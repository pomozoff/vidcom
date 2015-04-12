#include "videocontainer.h"

#include <QDebug>
#include <sstream>

VideoContainer::VideoContainer(const char* filePath)
	: _filePath(filePath)
	, _context(createContext())
	, _indexOfVideoStream(indexOfFirstVideoStream())
{
}
VideoContainer::~VideoContainer() {
	freeContext();
}

void VideoContainer::freeContext(void) {
	if (_context) {
		avformat_close_input(&_context);
		_context = NULL;
	}
}
AVFormatContext* VideoContainer::createContext(void) const {
	av_log_set_level(AV_LOG_DEBUG);
	av_register_all();
	avdevice_register_all();
	avcodec_register_all();

	AVFormatContext* context = avformat_alloc_context();
	if (avformat_open_input(&context, _filePath, NULL, NULL) != 0) {
		std::stringstream errorTextStream;
		errorTextStream << "Не могу открыть файл: " << _filePath;
		throw std::logic_error(errorTextStream.str());
	}
	return context;
}
int VideoContainer::indexOfFirstVideoStream(void) {
	if (avformat_find_stream_info(_context, NULL) < 0) {
		freeContext();
		std::stringstream errorTextStream;
		errorTextStream << "Не могу найти медиа-потоки в файле: " << _filePath;
		throw std::logic_error(errorTextStream.str());
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
		errorTextStream << "Не могу найти видео-поток в файле: " << _filePath;
		throw std::logic_error(errorTextStream.str());
	}

	av_dump_format(_context, 0, _filePath, 0);
	qDebug() << "Найдена видео-дорожка с индексом: " << indexOfVideoStream;

	//auto codecContext = _context->streams[indexOfVideoStream]->codec;
	return indexOfVideoStream;
}
