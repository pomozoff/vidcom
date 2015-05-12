#include "videocontainer.h"

#include <QDebug>
#include <sstream>

extern "C" {
#include "libavcodec/avcodec.h"
}

VideoContainer::VideoContainer(const QString& filePath)
	: _filePath(filePath)
	, _context(createContext(filePath))
	, _indexOfVideoStream(findIndexOfFirstVideoStream(filePath))
{
}
VideoContainer::~VideoContainer() {
	freeContext();
}

const KeyFramesList VideoContainer::listOfKeyFrames(const int indexOfVideoStream) const {
	auto codecContextOriginal = _context->streams[indexOfVideoStream]->codec;

	auto videoCodec = avcodec_find_decoder(codecContextOriginal->codec_id);
	if (!videoCodec) {
		QString errorText = "Кодек не поддерживается: " + (QString)codecContextOriginal->codec_id;
		throw std::runtime_error(errorText.toStdString());
	}

	auto codecContext = avcodec_alloc_context3(videoCodec);
	if (avcodec_copy_context(codecContext, codecContextOriginal) != 0) {
		QString errorText = "Не могу скопировать контекст кодека";
		throw std::runtime_error(errorText.toStdString());
	}

	if (avcodec_open2(codecContext, videoCodec, 0) != 0) {
		QString errorText = "Не могу открыть кодек";
		throw std::runtime_error(errorText.toStdString());
	}

	AVPacket packet;
	if (av_read_frame(_context, &packet) < 0) {
		QString errorText = "Ошибка чтения первого фрейма";
		throw std::runtime_error(errorText.toStdString());
	}

	auto frame = av_frame_alloc();
    auto keyFrames = std::vector<FractionalSecond>();

	while (av_seek_frame(_context, indexOfVideoStream, packet.dts + 1, 0) >= 0) {
        if (av_read_frame(_context, &packet) < 0) {
			qDebug() << "Error packet reading, dts:" << packet.dts;
            continue;
		} else if (packet.stream_index != indexOfVideoStream) {
            continue;
		}

		int gotPicture = 0;
		auto bytesDecoded = avcodec_decode_video2(codecContext, frame, &gotPicture, &packet);

		if (bytesDecoded < 0 || !gotPicture) {
			qDebug() << "Error packet decoding, dts:" << packet.dts;
			av_frame_unref(frame);
			av_free_packet(&packet);
			continue;
		}
		qDebug() << "Frame successfully decoded, dts:" << packet.dts;
		av_frame_unref(frame);
		av_free_packet(&packet);
		//keyFrames.push_back(packet.dts);
	}
	av_frame_free(&frame);

	avcodec_close(codecContext);
	avcodec_close(codecContextOriginal);

	return keyFrames;
}
int VideoContainer::indexOfFirstVideoStream(void) const {
	return _indexOfVideoStream;
}

void VideoContainer::freeContext(void) {
	if (_context) {
		avformat_close_input(&_context);
		_context = NULL;
	}
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
		freeContext();
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
		freeContext();
		QString errorText = "Не могу найти видео-поток в файле: " + filePath;
		throw std::runtime_error(errorText.toStdString());
	}
	av_dump_format(_context, 0, filePath.toUtf8().constData(), 0);
	return indexOfVideoStream;
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
