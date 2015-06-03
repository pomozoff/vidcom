#ifndef VIDEOCONTAINER_H
#define VIDEOCONTAINER_H

#include <memory>
#include <string>
#include <vector>

#include <QImage>

extern "C" {
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
}

struct AVFormatContext;
class VideoContainer;

using QImagePtr = std::shared_ptr<const QImage>;
using VideoContainerPtr = std::shared_ptr<VideoContainer>;
using FractionalSecond = int64_t;
using KeyFramesList = std::vector<FractionalSecond>;

class VideoContainer {
  public:
	VideoContainer(const QString& filePath);
	~VideoContainer();

	//const KeyFramesList listOfKeyFrames(const int indexOfVideoStream) const;
	int indexOfFirstVideoStream(void) const;

	const QImagePtr thumbnailForPositionMilliseconds(const int64_t positionMilliseconds) const;

	int64_t milliSecondsFromDts(const FractionalSecond contextDts) const;
	FractionalSecond dtsFromMilliSeconds(const int64_t milliSeconds) const;

	int64_t startTimeMilliseconds(void) const;
	int64_t durationMilliseconds(void) const;

	int64_t lastPositionMilliseconds(void) const;
  private:
	const QString _filePath;
	AVFormatContext* _context = nullptr;
	const int _indexOfVideoStream = -1;
	AVCodecContext* _codecContext = nullptr;
	AVPacket _firstPacket;
	mutable AVPacket _lastReadPacket;

	AVFormatContext* createContext(const QString& filePath) const;
	int findIndexOfFirstVideoStream(const QString& filePath);
	void readPacket(void) const;
	AVFrame* decodeFrame(void) const;
	//AVFrame* hasNextKeyFrame(const int indexOfVideoStream, int64_t dts) const;
	void freeContexts(void);

	FractionalSecond startTime(void) const;
	FractionalSecond duration(void) const;

	AVFrame* allocateFrame(void) const;
	AVCodecContext* getCodecContext(const int indexOfVideoStream) const;
	AVFrame* getKeyFrameAtPositionMillisecond(const int indexOfVideoStream, const FractionalSecond positionMilliseconds) const;
};

#endif // VIDEOCONTAINER_H
