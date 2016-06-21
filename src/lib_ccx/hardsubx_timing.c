#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_OCR
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "allheaders.h"
#include "hardsubx.h"

int bsearch_frame_neighbourhood(struct lib_hardsubx_ctx *ctx)
{
	// Searches the neighbourhood of the frame with the detected sub to get exact timing

}

#endif