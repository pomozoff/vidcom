#ifndef VIDEOCONTAINER_H
#define VIDEOCONTAINER_H

#include <memory>
#include <string>
#include <vector>

extern "C" {
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
}

struct AVFormatContext;
class VideoContainer;

using VideoContainerPtr = std::shared_ptr<VideoContainer>;
using FractionalSecond = int64_t;
using KeyFramesList = std::vector<FractionalSecond>;

class VideoContainer {
  public:
	VideoContainer(const QString& filePath);
	~VideoContainer();

	const KeyFramesList listOfKeyFrames(const int indexOfVideoStream) const;
	int indexOfFirstVideoStream(void) const;

  private:
	const QString _filePath;
	AVFormatContext* _context = NULL;
	const int _indexOfVideoStream = -1;

	AVFormatContext* createContext(const QString& filePath) const;
	int findIndexOfFirstVideoStream(const QString& filePath);
	void freeContext(void);

	FractionalSecond startTime(void) const;
	FractionalSecond duration(void) const;
	int64_t realSeconds(const FractionalSecond value) const;
};

#endif // VIDEOCONTAINER_H
