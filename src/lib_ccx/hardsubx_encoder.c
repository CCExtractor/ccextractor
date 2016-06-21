#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_OCR
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "allheaders.h"
#include "hardsubx.h"

int _hardsubx_write_subtitle_as_srt()
{

}

int _hardsubx_write_subtitle_as_transcript()
{
	
}

int _hardsubx_write_subtitle_as_sami()
{
	
}

int _hardsubx_write_subtitle_as_ass()
{
	
}

int hardsubx_write_subtitle(struct lib_hardsubx_ctx *ctx)
{
	switch(ctx->write_format) // Replace with timer context write_format?
	{
		case CCX_OF_NULL:
			break;
		case CCX_OF_SRT:
			break;
		case CCX_OF_TRANSCRIPT:
			break;
		case CCX_OF_SAMI:
			break;
		default:
			break;
	}
}

#endif