#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_OCR
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "allheaders.h"
#include "hardsubx.h"

int detect_italics()
{
	//TODO: Get orientation of the detected subtitles
}

int detect_subtitle_color()
{
	//TODO: Detect the color of detected subtitle line
}

#endif