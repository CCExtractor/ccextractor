#ifndef _FFMPEG_INTIGRATION
#define _FFMPEG_INTIGRATION

#ifdef ENABLE_FFMPEG
#include "libavutil/common.h"
#include "libavutil/error.h"
#endif
/**
 * @path this path could be relative or absolute path of static file
 * 	 this path could be path of device
 *
 * @return ctx Context of ffmpeg
 */
void *init_ffmpeg(char *path);

/**
 * @param ctx context of ffmpeg
 * @param data preallocated buffer where data will be recieved
 * @param maxlen length of buffer, where data will be copied
 * @return number of bytes recieved as data
 */
int ff_get_ccframe(void *arg, unsigned char*data, int maxlen);
#endif
