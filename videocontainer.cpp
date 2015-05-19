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
	if (av_read_frame(_context, &_firstPacket) < 0) {
		QString errorText = "Ошибка чтения первого пакета";
		throw std::runtime_error(errorText.toStdString());
	}
}
VideoContainer::~VideoContainer() {
	freeContexts();
}

const KeyFramesList VideoContainer::listOfKeyFrames(const int indexOfVideoStream) const {
	AVPacket packet;
	av_copy_packet(&packet, &_firstPacket);

	auto keyFrame = allocateFrame();
	auto keyFrames = std::vector<FractionalSecond>();

	while (hasNextKeyFrame(indexOfVideoStream, packet.dts, packet, keyFrame)) {
		keyFrames.push_back(packet.dts);
		av_frame_unref(keyFrame);
		av_free_packet(&packet);
	}
	av_frame_free(&keyFrame);

	return keyFrames;
}
int VideoContainer::indexOfFirstVideoStream(void) const {
	return _indexOfVideoStream;
}
const QImage VideoContainer::firstKeyFrameImage(void) const {
	auto keyFrame = getKeyFrameAt(_indexOfVideoStream, _firstPacket.dts);
	if (!keyFrame) {
		throw std::runtime_error("No keyframe found");
	}

	auto frameRGB = allocateFrame();
	auto numBytes = avpicture_get_size(PIX_FMT_RGB24,
									   _codecContext->width,
									   _codecContext->height);
	auto buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
	avpicture_fill((AVPicture *)frameRGB,
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
			  (uint8_t const * const *)keyFrame->data,
			  keyFrame->linesize,
			  0,
			  _codecContext->height,
			  frameRGB->data,
			  frameRGB->linesize);

	QImage image(_codecContext->width
				 , _codecContext->height
				 , QImage::Format_RGB32);
	uint8_t *src = (uint8_t *)(frameRGB->data[0]);
	for (int y = 0; y < _codecContext->height; ++y) {
		QRgb *scanLine = (QRgb *) image.scanLine(y);
		for (int x = 0; x < _codecContext->width; x += 1) {
			scanLine[x] = qRgb(src[3 * x + 0]
						  ,    src[3 * x + 1]
						  ,    src[3 * x + 2]);
		}
		src += frameRGB->linesize[0];
	}
	return image;
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
bool VideoContainer::hasNextKeyFrame(const int indexOfVideoStream, const int64_t dts, AVPacket& packet, AVFrame* frame) const {
	while (av_seek_frame(_context, indexOfVideoStream, dts + 1, 0) >= 0) {
		if (av_read_frame(_context, &packet) < 0) {
			qDebug() << "Error packet reading, dts:" << packet.dts;
			continue;
		} else if (packet.stream_index != indexOfVideoStream) {
			continue;
		}

		int gotPicture = 0;
		auto bytesDecoded = avcodec_decode_video2(_codecContext, frame, &gotPicture, &packet);

		if (bytesDecoded < 0 || !gotPicture) {
			qDebug() << "Error packet decoding, dts:" << packet.dts;
			av_frame_unref(frame);
			av_free_packet(&packet);
			continue;
		}
		qDebug() << "Frame successfully decoded, dts:" << packet.dts;
		return true;
	}
	return false;
}
void VideoContainer::freeContexts(void) {
	if (_context) {
		avformat_close_input(&_context);
		_context = NULL;
	}
	if (_codecContext) {
		avcodec_close(_codecContext);
		_codecContext = NULL;
	}
}

FractionalSecond VideoContainer::startTime(void) const {
	return _context->start_time;
}
FractionalSecond VideoContainer::duration(void) const {
	return _context->duration;
}
int64_t VideoContainer::realSeconds(const FractionalSecond value) const {
	return value / AV_TIME_BASE;
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
AVFrame* VideoContainer::getKeyFrameAt(const int indexOfVideoStream, const int64_t dts) const {
	AVPacket packet;
	packet.dts = dts;

	auto keyFrame = allocateFrame();
	while (hasNextKeyFrame(indexOfVideoStream, packet.dts, packet, keyFrame)) {
		av_free_packet(&packet);
		return keyFrame;
	}
	av_frame_free(&keyFrame);

	return NULL;
}
