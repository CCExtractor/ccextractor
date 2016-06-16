#ifdef ENABLE_OCR
#include "hardsubx.h"
#include "capi.h"
#include "allheaders.h"
#include "ocr.h"
#include "utility.h"

//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

int hardsubx_process_data(struct lib_hardsubx_ctx *ctx)
{
	// Get the required media attributes and initialize structures
	av_register_all();
	
	if(avformat_open_input(&ctx->format_ctx, ctx->inputfile[0], NULL, NULL)!=0)
	{
		//TODO: Give error about file not opened
		return -1;
	}

	if(avformat_find_stream_info(ctx->format_ctx, NULL)<0)
	{
		//TODO: Give error about not finding stream information
		return -1;
	}

	//TODO: Remove this and handle in params dump?
	// Need to calculate stuff too in that case
	av_dump_format(ctx->format_ctx, 0, ctx->inputfile[0], 0);
	

	ctx->video_stream_id = -1;
	for(int i = 0; i < ctx->format_ctx->nb_streams; i++)
	{
		if(ctx->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			ctx->video_stream_id = i;
			break;
		}
	}
	if(ctx->video_stream_id == -1)
	{
		//TODO: give error about not finding video stream
		return -1;
	}

	ctx->codec_ctx = ctx->format_ctx->streams[ctx->video_stream_id]->codec;
	ctx->codec = avcodec_find_decoder(ctx->codec_ctx->codec_id);
	if(ctx->codec == NULL)
	{
		//TODO: Give error about unsupported codec
		return -1;
	}

	if(avcodec_open2(ctx->codec_ctx, ctx->codec, &ctx->options_dict) < 0)
	{
		//TODO: Give error about not opening codec
		return -1;
	}

	ctx->frame = av_frame_alloc();
	ctx->rgb_frame = av_frame_alloc();
	if(!ctx->frame || !ctx->rgb_frame)
	{
		//TODO: Give error about not enough memory
		return -1;
	}

	int frame_bytes = avpicture_get_size(AV_PIX_FMT_RGB24, ctx->codec_ctx->width, ctx->codec_ctx->height);
	ctx->rgb_buffer = (uint8_t *)av_malloc(frame_bytes*sizeof(uint8_t));
	
	ctx->sws_ctx = sws_getContext(
			ctx->codec_ctx->width,
			ctx->codec_ctx->height,
			ctx->codec_ctx->pix_fmt,
			ctx->codec_ctx->width,
			ctx->codec_ctx->height,
			AV_PIX_FMT_RGB24,
			SWS_BILINEAR,
			NULL,NULL,NULL
		);

	avpicture_fill((AVPicture *)ctx->rgb_frame, ctx->rgb_buffer, AV_PIX_FMT_RGB24, ctx->codec_ctx->width, ctx->codec_ctx->height);

	// Pass on the processing context to the appropriate functions
	hardsubx_process_frames_linear(ctx);
	// TODO: Add binary search processing mode

	// Free the allocated memory for frame processing
	av_free(ctx->rgb_buffer);
	av_free(ctx->rgb_frame);
	av_free(ctx->frame);
	avcodec_close(ctx->codec_ctx);
	avformat_close_input(&ctx->format_ctx);
}

void _hardsubx_params_dump(struct ccx_s_options *options, struct lib_hardsubx_ctx *ctx)
{
	// Print the relevant passed parameters onto the screen
	mprint("Input : %s\n", ctx->inputfile[0]);
}

struct lib_hardsubx_ctx* _init_hardsubx(struct ccx_s_options *options)
{
	// Initialize HardsubX data structures
	struct lib_hardsubx_ctx *ctx = (struct lib_hardsubx_ctx *)malloc(sizeof(struct lib_hardsubx_ctx));
	if(!ctx)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "lib_hardsubx_ctx");
	memset(ctx, 0, sizeof(struct lib_hardsubx_ctx));

	//Initialize attributes common to lib_ccx context
	ctx->basefilename = get_basename(options->output_filename);//TODO: Check validity, add stdin, network
	ctx->current_file = -1;
	ctx->inputfile = options->inputfile;
	ctx->num_input_files = options->num_input_files;
	ctx->extension = get_file_extension(options->write_format);
	ctx->write_format = options->write_format;
	ctx->subs_delay = options->subs_delay;
	ctx->cc_to_stdout = options->cc_to_stdout;

	return ctx;
}

void _dinit_hardsubx(struct lib_hardsubx_ctx **ctx)
{
	// Free all memory allocated to everything in the context
}

void hardsubx(struct ccx_s_options *options)
{
	// This is similar to the 'main' function in ccextractor.c, but for hard subs
	mprint("HardsubX (Hard Subtitle Extractor) - Burned-in subtitle extraction subsystem\n");

	// Initialize HardsubX data structures
	struct lib_hardsubx_ctx *ctx;
	ctx = _init_hardsubx(options);

	// Dump parameters (Not using params_dump since completely different parameters)
	_hardsubx_params_dump(options, ctx);

	// Configure output settings (write format, capitalization etc)

	// Data processing loop
	time_t start, end;
	time(&start);
	hardsubx_process_data(ctx);

	// Show statistics (time taken, frames processed, mode etc)
	time(&end);
	long processing_time=(long) (end-start);
	mprint ("\rDone, processing time = %ld seconds\n", processing_time);

	// Free all allocated memory for the data structures
	_dinit_hardsubx(&ctx);
}

#endif