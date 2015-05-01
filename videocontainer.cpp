#include "videocontainer.h"

#include <QDebug>
#include <sstream>

extern "C" {
#include "libavcodec/avcodec.h"
}

VideoContainer::VideoContainer(const char* filePath)
	: _filePath(filePath)
	, _context(createContext(filePath))
	, _indexOfVideoStream(findIndexOfFirstVideoStream(filePath))
{
}
VideoContainer::~VideoContainer() {
	freeContext();
}

KeyFramesList VideoContainer::listOfKeyFrames(const int indexOfVideoStream) const {
	qDebug() << "Чтение файла: " << _filePath.c_str() << ", индекс видеодорожки: " << indexOfVideoStream;

	auto codecContextOriginal = _context->streams[indexOfVideoStream]->codec;

	auto videoCodec = avcodec_find_decoder(codecContextOriginal->codec_id);
	if (!videoCodec) {
		std::stringstream errorTextStream;
		errorTextStream << "Кодек не поддерживается: " << codecContextOriginal->codec_id;
		throw std::runtime_error(errorTextStream.str());
	}

	auto codecContext = avcodec_alloc_context3(videoCodec);
	if (avcodec_copy_context(codecContext, codecContextOriginal) != 0) {
		std::stringstream errorTextStream;
		errorTextStream << "Не могу скопировать контекст кодека";
		throw std::runtime_error(errorTextStream.str());
	}

	if (avcodec_open2(codecContext, videoCodec, 0) != 0) {
		std::stringstream errorTextStream;
		errorTextStream << "Не могу открыть кодек";
		throw std::runtime_error(errorTextStream.str());
	}

	AVPacket packet;
	if (av_read_frame(_context, &packet) < 0) {
		std::stringstream errorTextStream;
		errorTextStream << "Ошибка чтения первого фрейма";
		throw std::runtime_error(errorTextStream.str());
	}

	auto frame = av_frame_alloc();
	auto keyFrames = std::vector<const FractionalSecond>();

	while (av_seek_frame(_context, indexOfVideoStream, packet.dts, 0) >= 0) {
		if (packet.stream_index != indexOfVideoStream) {
			continue;
		}

		int gotPicture = 0;
		auto bytesDecoded = avcodec_decode_video2(codecContext, frame, &gotPicture, &packet);

		if (bytesDecoded < 0 || !gotPicture) {
			qDebug() << "Ошибка декодирования пакета, время: " << packet.dts;
			av_frame_unref(frame);
			av_free_packet(&packet);
			continue;
		}
		qDebug() << "Фрейм успешно декодирован, время: " << packet.dts;
		av_frame_unref(frame);
		av_free_packet(&packet);
		//keyFrames.push_back(packet.dts);
	}
	av_frame_free(&frame);

	avcodec_close(codecContext);
	avcodec_close(codecContextOriginal);

	av_frame_free(&frame);

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
AVFormatContext* VideoContainer::createContext(const char* filePath) const {
	av_log_set_level(AV_LOG_DEBUG);
	av_register_all();
	avdevice_register_all();
	avcodec_register_all();

	AVFormatContext* context = avformat_alloc_context();
	if (avformat_open_input(&context, filePath, NULL, NULL) != 0) {
		std::stringstream errorTextStream;
		errorTextStream << "Не могу открыть файл: " << filePath;
		throw std::runtime_error(errorTextStream.str());
	}
	return context;
}
int VideoContainer::findIndexOfFirstVideoStream(const char* filePath) {
	if (avformat_find_stream_info(_context, NULL) < 0) {
		freeContext();
		std::stringstream errorTextStream;
		errorTextStream << "Не могу найти медиа-потоки в файле: " << filePath;
		throw std::runtime_error(errorTextStream.str());
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
		std::stringstream errorTextStream;
		errorTextStream << "Не могу найти видео-поток в файле: " << filePath;
		throw std::runtime_error(errorTextStream.str());
	}

	av_dump_format(_context, 0, filePath, 0);
	qDebug() << "Найдена видео-дорожка с индексом: " << indexOfVideoStream;

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
