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
};

#endif // VIDEOCONTAINER_H
