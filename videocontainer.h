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
using KeyFramesList = std::vector<const FractionalSecond>;

class VideoContainer {
  public:
	VideoContainer(const char* filePath);
	~VideoContainer();

	KeyFramesList listOfKeyFrames(const int indexOfVideoStream) const;

  private:
	const std::string _filePath;
	AVFormatContext* _context = NULL;
	const int _indexOfVideoStream = -1;

	AVFormatContext* createContext(const char* filePath) const;
	int indexOfFirstVideoStream(const char* filePath);
	void freeContext(void);

	FractionalSecond startTime(void) const;
	FractionalSecond duration(void) const;
	int64_t realSeconds(const FractionalSecond value) const;
};

#endif // VIDEOCONTAINER_H
