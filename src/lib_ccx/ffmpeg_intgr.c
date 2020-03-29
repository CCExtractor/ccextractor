#include "ffmpeg_intgr.h"

#ifdef ENABLE_FFMPEG
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
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
 * call back function to be registered for avlog
 */
static void log_cb(void *ptr, int level, const char *fmt, va_list vl)
{
	if (level > av_log_get_level())
		return;

	const char *name = NULL;
	FILE *flog;

	if (ccx_options.messages_target == CCX_MESSAGES_STDOUT)
		flog = stdout;
	else
		flog = stderr;

	if (level == AV_LOG_PANIC)
		fprintf(flog, "[panic][%s] ", name);
	else if (level == AV_LOG_FATAL)
		fprintf(flog, "[fatal][%s] ", name);
	else if (level == AV_LOG_ERROR)
		fprintf(flog, "[error][%s] ", name);
	else if (level == AV_LOG_WARNING)
		fprintf(flog, "[warning][%s] ", name);
	else if (level == AV_LOG_INFO)
		fprintf(flog, "[info][%s] ", name);
	else if (level == AV_LOG_DEBUG)
		fprintf(flog, "[debug][%s] ", name);

	vfprintf(flog, fmt, vl);
}

/**
 * @path this path could be relative or absolute path of static file
 * 	 this path could be path of device
 *
 * @return ctx Context of ffmpeg
 */
void *init_ffmpeg(const char *path)
{
	int ret = 0;
	int stream_index = 0;
	struct ffmpeg_ctx *ctx;
	AVCodec *dec = NULL;
	avcodec_register_all();
	av_register_all();

	if (ccx_options.debug_mask & CCX_DMT_VERBOSE)
		av_log_set_level(AV_LOG_INFO);
	else if (ccx_options.messages_target == 0)
		av_log_set_level(AV_LOG_FATAL);

	av_log_set_callback(log_cb);

	ctx = av_mallocz(sizeof(*ctx));
	if (!ctx)
	{
		av_log(NULL, AV_LOG_ERROR, "Not enough memory\n");
		return NULL;
	}
	/**
	 * Initialize decoder according to the name of input
	 */
	ret = avformat_open_input(&ctx->ifmt, path, NULL, NULL);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "could not open input(%s) format\n", path);
		goto fail;
	}

	ret = avformat_find_stream_info(ctx->ifmt, NULL);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "could not find any stream\n");
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
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "unable to open codec\n");
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
int ff_get_ccframe(void *arg, unsigned char *data, int maxlen)
{
	struct ffmpeg_ctx *ctx = arg;
	int len = 0;
	int ret = 0;
	int got_frame;
	AVPacket packet;

	ret = av_read_frame(ctx->ifmt, &packet);
	if (ret == AVERROR_EOF)
	{
		return ret;
	}
	else if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "not able to read the packet\n");
		return ret;
	}
	else if (packet.stream_index != ctx->stream_index)
	{
		return AVERROR(EAGAIN);
	}

	ret = avcodec_decode_video2(ctx->dec_ctx, ctx->frame, &got_frame, &packet);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "unable to decode packet\n");
	}
	else if (!got_frame)
	{
		return AVERROR(EAGAIN);
	}
	//	current_pts = av_frame_get_best_effort_timestamp(ctx->frame);
	//	if(!pts_set)
	//		pts_set = 1;
	//	set_fts();
	for (int i = 0; i < ctx->frame->nb_side_data; i++)
	{
		if (ctx->frame->side_data[i]->type == AV_FRAME_DATA_A53_CC)
		{
			ctx->frame->pts = av_frame_get_best_effort_timestamp(ctx->frame);
			if (ctx->frame->side_data[i]->size > maxlen)
				av_log(NULL, AV_LOG_ERROR, "Please consider increasing length of data\n");
			else
			{
				memcpy(data, ctx->frame->side_data[i]->data, ctx->frame->side_data[i]->size);
				len = ctx->frame->side_data[i]->size;
			}
		}
	}

	if (ret < 0)
		return ret;

	return len;
}

int ffmpeg_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata)
{
	struct demuxer_data *data;
	int ret = 0;
	if (!*ppdata)
	{
		*ppdata = alloc_demuxer_data();
		if (!*ppdata)
			return -1;
		data = *ppdata;
		//TODO Set to dummy, find and set actual value
		data->program_number = 1;
		data->stream_pid = 1;
		data->codec = CCX_CODEC_ATSC_CC;
		data->bufferdatatype = CCX_RAW_TYPE;
	}
	else
	{
		data = *ppdata;
	}

	do
	{
		int len = ff_get_ccframe(ctx->demux_ctx->ffmpeg_ctx, data->buffer, BUFSIZE);
		if (len == AVERROR(EAGAIN))
		{
			continue;
		}
		else if (len == AVERROR_EOF)
		{
			ret = CCX_EOF;
			break;
		}
		else if (len == 0)
			continue;
		else if (len < 0)
		{
			mprint("Error extracting Frame\n");
			break;
		}
		else
		{
			data->len = len;
			break;
		}

	} while (1);

	return ret;
}
#endif
