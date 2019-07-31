#ifndef _FFMPEG_INTIGRATION
#define _FFMPEG_INTIGRATION

#include "lib_ccx.h"
#ifdef ENABLE_FFMPEG
#include "libavutil/common.h"
#include "libavutil/error.h"
#endif
#include "ccx_demuxer.h"
/**
 * @path this path could be relative or absolute path of static file
 * 	 this path could be path of device
 *
 * @return ctx Context of ffmpeg
 */
void *init_ffmpeg(const char *path);

int ffmpeg_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata);
#endif
