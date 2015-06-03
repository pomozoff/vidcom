#include "videocontainer.h"

#include <sstream>

#include <QDebug>

extern "C" {
#include "libavcodec/avcodec.h"
#include <libswscale/swscale.h>
}

VideoContainer::VideoContainer(const QString& filePath)
	: _filePath(filePath)
	, _context(createContext(filePath))
	, _indexOfVideoStream(findIndexOfFirstVideoStream(filePath))
	, _codecContext(getCodecContext(_indexOfVideoStream))
{
	_lastReadPacket.buf = nullptr;
	readPacket();
	av_copy_packet(&_firstPacket, &_lastReadPacket);
}
VideoContainer::~VideoContainer() {
	freeContexts();

	av_free_packet(&_lastReadPacket);
	av_free_packet(&_firstPacket);
}

/*
const KeyFramesList VideoContainer::listOfKeyFrames(const int indexOfVideoStream) const {
	auto keyFrame = allocateFrame();
	auto keyFrames = std::vector<FractionalSecond>();

	int64_t dts = _firstPacket.dts;
	while (true) {
		auto keyFrame = hasNextKeyFrame(indexOfVideoStream, dts);
		if (!keyFrame) {
			break;
		}
		keyFrames.push_back(_lastReadPacket.dts);
		if (_codecContext->refcounted_frames == 1) {
			av_frame_unref(keyFrame);
		}
		dts = _lastReadPacket.dts;
		qDebug() << "Last read packet, dts:" << _lastReadPacket.dts;
	}
	av_frame_free(&keyFrame);

	return keyFrames;
}
*/
int VideoContainer::indexOfFirstVideoStream(void) const {
	return _indexOfVideoStream;
}
const QImagePtr VideoContainer::thumbnailForPositionMilliseconds(const int64_t positionMilliseconds) const {
	auto frame = getKeyFrameAtPositionMillisecond(_indexOfVideoStream, positionMilliseconds);
	if (!frame) {
		throw std::runtime_error("No frame found");
	}

	auto frameRGB = allocateFrame();
	auto numBytes = avpicture_get_size(PIX_FMT_RGB24,
									   _codecContext->width,
									   _codecContext->height);
	auto buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	avpicture_fill((AVPicture*)frameRGB,
				   buffer,
				   PIX_FMT_RGB24,
				   _codecContext->width,
				   _codecContext->height);
	auto sws_ctx = sws_getContext(_codecContext->width,
								  _codecContext->height,
								  _codecContext->pix_fmt,
								  _codecContext->width,
								  _codecContext->height,
								  PIX_FMT_RGB24,
								  SWS_BILINEAR,
								  NULL,
								  NULL,
								  NULL);
	sws_scale(sws_ctx,
			  (uint8_t const * const*)frame->data,
			  frame->linesize,
			  0,
			  _codecContext->height,
			  frameRGB->data,
			  frameRGB->linesize);

	if (_codecContext->refcounted_frames == 1) {
		av_frame_unref(frame);
	}
	av_frame_free(&frame);

	QImage image(_codecContext->width
				 , _codecContext->height
				 , QImage::Format_RGB32);
	uint8_t* src = (uint8_t*)(frameRGB->data[0]);

	for (int y = 0; y < _codecContext->height; ++y) {
		QRgb* scanLine = (QRgb*) image.scanLine(y);

		for (int x = 0; x < _codecContext->width; x += 1) {
			scanLine[x] = qRgb(src[3 * x + 0]
							 , src[3 * x + 1]
							 , src[3 * x + 2]);
		}

		src += frameRGB->linesize[0];
	}
	av_frame_free(&frameRGB);

	return std::make_shared<const QImage>(image);
}

int64_t VideoContainer::milliSecondsFromDts(const FractionalSecond contextDts) const {
	auto timeBase = _context->streams[_indexOfVideoStream]->time_base;
	timeBase.den *= 1000;
	auto milliSeconds = av_rescale_q(contextDts
								   , timeBase
								   , AV_TIME_BASE_Q);
	return milliSeconds;
}
FractionalSecond VideoContainer::dtsFromMilliSeconds(const int64_t milliSeconds) const {
	auto timeBase = _context->streams[_indexOfVideoStream]->time_base;
	timeBase.den *= 1000;
	auto contextDts = av_rescale_q(milliSeconds
								 , AV_TIME_BASE_Q
								 , timeBase);
	return contextDts;
}

int64_t VideoContainer::startTime(void) const {
	return _context->start_time / AV_TIME_BASE;
}
FractionalSecond VideoContainer::startTimeMilliseconds(void) const {
	return _context->start_time / (AV_TIME_BASE / 1000);
}
int64_t VideoContainer::durationMilliseconds(void) const {
	return milliSecondsFromDts(_context->streams[_indexOfVideoStream]->duration);
}
int64_t VideoContainer::lastPositionMilliseconds(void) const {
	return milliSecondsFromDts(_lastReadPacket.dts);
}

AVFormatContext* VideoContainer::createContext(const QString& filePath) const {
	av_log_set_level(AV_LOG_DEBUG);
	av_register_all();
	avdevice_register_all();
	avcodec_register_all();

	AVFormatContext* context = avformat_alloc_context();

	if (avformat_open_input(&context, filePath.toUtf8().constData(), NULL, NULL) != 0) {
		QString errorText = "Не могу открыть файл: " + filePath;
		throw std::runtime_error(errorText.toStdString());
	}

	return context;
}
int VideoContainer::findIndexOfFirstVideoStream(const QString& filePath) {
	if (avformat_find_stream_info(_context, NULL) < 0) {
		freeContexts();
		QString errorText = "Не могу найти медиа-потоки в файле: " + filePath;
		throw std::runtime_error(errorText.toStdString());
	}

	int indexOfVideoStream = -1;

	for (unsigned int i = 0; i < _context->nb_streams; ++i) {
		if (_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			indexOfVideoStream = i;
			break;
		}
	}

	if (indexOfVideoStream < 0) {
		freeContexts();
		QString errorText = "Не могу найти видео-поток в файле: " + filePath;
		throw std::runtime_error(errorText.toStdString());
	}

	av_dump_format(_context, 0, filePath.toUtf8().constData(), 0);
	return indexOfVideoStream;
}
void VideoContainer::readPacket(void) const {
	if (_lastReadPacket.buf != nullptr) {
		av_free_packet(&_lastReadPacket);
	}
	if (av_read_frame(_context, &_lastReadPacket) < 0) {
		QString errorText = "Error packet reading, dts:";
		errorText.append(QString::number(_lastReadPacket.dts));
		throw std::runtime_error(errorText.toStdString());
	}
	qDebug() << "Read packet, dts:" << _lastReadPacket.dts;
}
AVFrame* VideoContainer::decodeFrame(void) const {
	AVFrame* frame = allocateFrame();
	int gotPicture = 0;
	auto bytesDecoded = avcodec_decode_video2(_codecContext, frame, &gotPicture, &_lastReadPacket);

	if (bytesDecoded < 0 || !gotPicture) {
		qDebug() << "Error decoding frame, dts:" << _lastReadPacket.dts;
		if (_codecContext->refcounted_frames == 1) {
			av_frame_unref(frame);
		}
		av_frame_free(&frame);
		return nullptr;
	}
	qDebug() << "Frame successfully decoded, dts:" << _lastReadPacket.dts;
	return frame;
}
void VideoContainer::freeContexts(void) {
	if (_context) {
		avformat_close_input(&_context);
		_context = nullptr;
	}
	if (_codecContext) {
		avcodec_close(_codecContext);
		_codecContext = nullptr;
	}
}

FractionalSecond VideoContainer::duration(void) const {
	return _context->duration;
}

AVFrame* VideoContainer::allocateFrame(void) const {
	auto frame = av_frame_alloc();

	if (!frame) {
		QString errorText = "Ошибка выделения памяти для фрейма";
		throw std::runtime_error(errorText.toStdString());
	}

	return frame;
}
AVCodecContext* VideoContainer::getCodecContext(const int indexOfVideoStream) const {
	auto codecContextOriginal = _context->streams[indexOfVideoStream]->codec;

	auto videoCodec = avcodec_find_decoder(codecContextOriginal->codec_id);

	if (!videoCodec) {
		QString errorText = "Кодек не поддерживается: " + QString::number(codecContextOriginal->codec_id);
		throw std::runtime_error(errorText.toStdString());
	}

	auto _codecContext = avcodec_alloc_context3(videoCodec);

	if (avcodec_copy_context(_codecContext, codecContextOriginal) != 0) {
		QString errorText = "Не могу скопировать контекст кодека";
		throw std::runtime_error(errorText.toStdString());
	}

	if (avcodec_open2(_codecContext, videoCodec, 0) != 0) {
		QString errorText = "Не могу открыть кодек";
		throw std::runtime_error(errorText.toStdString());
	}

	return _codecContext;
}
AVFrame* VideoContainer::getKeyFrameAtPositionMillisecond(const int indexOfVideoStream, const FractionalSecond positionMilliseconds) const {
	auto duration = _context->streams[indexOfVideoStream]->duration;
	auto timeBase = _context->streams[indexOfVideoStream]->time_base;
	auto position = dtsFromMilliSeconds(positionMilliseconds);

	auto flags = position < _lastReadPacket.dts ? AVSEEK_FLAG_BACKWARD : 0;

	qDebug() << (_lastReadPacket.dts > position ? "BACKWARD" : "FORWARD");
	avcodec_flush_buffers(_codecContext);

	while (position < duration) {
		if (av_seek_frame(_context
						, indexOfVideoStream
						, position
						, flags) < 0)
		{
			qDebug() << "Error seeking frame, milliseconds:" << positionMilliseconds;
			return nullptr;
		}

		try {
			readPacket();
		} catch (std::runtime_error error) {
			qDebug() << "Error reading paket - " << error.what();
			position += timeBase.den;
			continue;
		}
		if (_lastReadPacket.stream_index != indexOfVideoStream) {
			qDebug() << "Packet contains frame with wrong stream index: "
					 << _lastReadPacket.stream_index
					 << ", should be: "
					 << indexOfVideoStream;
			position += timeBase.den;
			continue;
		}
		auto frame = decodeFrame();
		if (frame != nullptr) {
			return frame;
		}
	}
	return nullptr;
}
