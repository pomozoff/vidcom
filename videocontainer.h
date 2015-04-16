#ifndef VIDEOCONTAINER_H
#define VIDEOCONTAINER_H

#include <memory>
#include <string>

extern "C" {
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
}

struct AVFormatContext;

class VideoContainer;
using VideoContainerPtr = std::shared_ptr<VideoContainer>;

using fractionalSecond = int64_t;

class VideoContainer {
  public:
	VideoContainer(const char* filePath);
	~VideoContainer();

  private:
	const char* _filePath;
	AVFormatContext* _context = NULL;
	const int _indexOfVideoStream = -1;

	AVFormatContext* createContext(void) const;
	int indexOfFirstVideoStream(void);
	void freeContext(void);

	fractionalSecond startTime(void) const;
	fractionalSecond duration(void) const;
	int64_t realSeconds(const fractionalSecond value) const;
};

#endif // VIDEOCONTAINER_H
