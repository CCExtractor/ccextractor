#include "ffmpeg_intgr.h"

#ifdef ENABLE_FFMPEG
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avstring.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/log.h>
#include <libavutil/error.h>

struct ffmpeg_ctx
{
	AVFormatContext *ifmt;
	AVCodecContext *dec_ctx;
	AVFrame *frame;
	int stream_index;
};
/**
 * @path this path could be relative or absolute path of static file
 * 	 this path could be path of device
 *
 * @return ctx Context of ffmpeg
 */
void *init_ffmpeg(char *path)
{
	int ret = 0;
	int stream_index = 0;
	struct ffmpeg_ctx *ctx;
	AVCodec *dec = NULL;
	avcodec_register_all();
	av_register_all();

	ctx = av_malloc(sizeof(*ctx));
	if(!ctx)
	{
		av_log(NULL,AV_LOG_ERROR,"Not enough memory\n");
		return NULL;
	}
	/**
	 * Initialize decoder according to the name of input
	 */
	ret = avformat_open_input(&ctx->ifmt, path, NULL, NULL);
	if(ret < 0)
	{
		av_log(NULL,AV_LOG_ERROR,"could not open input(%s) format\n",path);
		goto fail;
	}

	ret = avformat_find_stream_info(ctx->ifmt,NULL);
	if(ret < 0)
	{
		av_log(NULL,AV_LOG_ERROR,"could not find any stream\n");
		goto fail;
	}

	/* first search in strean if not found search the video stream */
	ret = av_find_best_stream(ctx->ifmt, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "no suitable subtitle or caption\n");
		goto fail;
	}
	stream_index = ret;
	ctx->dec_ctx = ctx->ifmt->streams[stream_index]->codec;
	ctx->stream_index = stream_index;
	ret = avcodec_open2(ctx->dec_ctx, dec, NULL);
	if(ret < 0)
	{
		av_log(NULL,AV_LOG_ERROR,"unable to open codec\n");
		goto fail;
	}

	//Initialize frame where input frame will be stored
	ctx->frame = av_frame_alloc();

fail:
	return ctx;
}
/**
 * @param ctx context of ffmpeg
 * @param data preallocated buffer where data will be recieved
 * @param maxlen length of buffer, where data will be copied
 * @return number of bytes recieved as data
 */
int ff_get_ccframe(void *arg,char*data,int maxlen)
{
	struct ffmpeg_ctx *ctx = arg;
	int len = 0;
	int ret = 0;
	int got_frame;
	AVPacket packet;

	ret = av_read_frame(ctx->ifmt, &packet);
	if(ret == AVERROR_EOF)
	{
		return ret;
	}
	else if(ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "not able to read the packet\n");
		return ret;
	}
	else if(packet.stream_index != ctx->stream_index)
	{
		return AVERROR(EAGAIN);
	}

	ret = avcodec_decode_video2(ctx->dec_ctx,ctx->frame, &got_frame,&packet);
	if(ret < 0)
	{
		av_log(NULL,AV_LOG_ERROR,"unable to decode packet\n");
	}
	else if (!got_frame)
	{
		return AVERROR(EAGAIN);
	}
	for(int i = 0;i< ctx->frame->nb_side_data;i++)
	{
		if(ctx->frame->side_data[i]->type == AV_FRAME_DATA_A53_CC)
		{
			if(ctx->frame->side_data[i]->size > maxlen)
				av_log(NULL,AV_LOG_ERROR,"Please consider increaing length of data\n");
			else
			{
				memcpy(data,ctx->frame->side_data[i]->data,ctx->frame->side_data[i]->size);
				len = ctx->frame->side_data[i]->size;

			}
		}
	}


	if(ret < 0)
		return ret;

	return len;
}

#endif
