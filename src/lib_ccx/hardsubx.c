#ifdef ENABLE_HARDSUBX
#include <leptonica/allheaders.h>
#include "hardsubx.h"
#include "ocr.h"
#include "utility.h"

//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

int hardsubx_process_data(struct lib_hardsubx_ctx *ctx)
{
	// Get the required media attributes and initialize structures
	av_register_all();

	if (avformat_open_input(&ctx->format_ctx, ctx->inputfile[0], NULL, NULL) != 0)
	{
		fatal(EXIT_READ_ERROR, "Error reading input file!\n");
	}

	if (avformat_find_stream_info(ctx->format_ctx, NULL) < 0)
	{
		fatal(EXIT_READ_ERROR, "Error reading input stream!\n");
	}

	// Important call in order to determine media information using ffmpeg
	// TODO: Handle multiple inputs
	av_dump_format(ctx->format_ctx, 0, ctx->inputfile[0], 0);

	ctx->video_stream_id = -1;
	for (int i = 0; i < ctx->format_ctx->nb_streams; i++)
	{
		if (ctx->format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			ctx->video_stream_id = i;
			break;
		}
	}
	if (ctx->video_stream_id == -1)
	{
		fatal(EXIT_READ_ERROR, "Video Stream not found!\n");
	}

	ctx->codec_ctx = ctx->format_ctx->streams[ctx->video_stream_id]->codec;
	ctx->codec = avcodec_find_decoder(ctx->codec_ctx->codec_id);
	if (ctx->codec == NULL)
	{
		fatal(EXIT_READ_ERROR, "Input codec is not supported!\n");
	}

	if (avcodec_open2(ctx->codec_ctx, ctx->codec, &ctx->options_dict) < 0)
	{
		fatal(EXIT_READ_ERROR, "Error opening input codec!\n");
	}

	ctx->frame = av_frame_alloc();
	ctx->rgb_frame = av_frame_alloc();
	if (!ctx->frame || !ctx->rgb_frame)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to initialize frame!");
	}

	int frame_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, ctx->codec_ctx->width, ctx->codec_ctx->height, 16);
	ctx->rgb_buffer = (uint8_t *)av_malloc(frame_bytes * sizeof(uint8_t));

	ctx->sws_ctx = sws_getContext(
	    ctx->codec_ctx->width,
	    ctx->codec_ctx->height,
	    ctx->codec_ctx->pix_fmt,
	    ctx->codec_ctx->width,
	    ctx->codec_ctx->height,
	    AV_PIX_FMT_RGB24,
	    SWS_BILINEAR,
	    NULL, NULL, NULL);

	av_image_fill_arrays(ctx->rgb_frame->data, ctx->rgb_frame->linesize, ctx->rgb_buffer, AV_PIX_FMT_RGB24, ctx->codec_ctx->width, ctx->codec_ctx->height, 1);

	// int frame_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, 1280, 720, 16);
	// ctx->rgb_buffer = (uint8_t *)av_malloc(frame_bytes*sizeof(uint8_t));

	// ctx->sws_ctx = sws_getContext(
	// 		ctx->codec_ctx->width,
	// 		ctx->codec_ctx->height,
	// 		ctx->codec_ctx->pix_fmt,
	// 		1280,
	// 		720,
	// 		AV_PIX_FMT_RGB24,
	// 		SWS_BILINEAR,
	// 		NULL,NULL,NULL
	// 	);
	// avpicture_fill((AVPicture*)ctx->rgb_frame, ctx->rgb_buffer, AV_PIX_FMT_RGB24, 1280, 720);
	// av_image_fill_arrays(ctx->rgb_frame->data, ctx->rgb_frame->linesize, ctx->rgb_buffer, AV_PIX_FMT_RGB24, 1280, 720, 1);

	// Pass on the processing context to the appropriate functions
	struct encoder_ctx *enc_ctx;
	enc_ctx = init_encoder(&ccx_options.enc_cfg);

	mprint("Beginning burned-in subtitle detection...\n");

	if (ctx->tickertext)
		hardsubx_process_frames_tickertext(ctx, enc_ctx);
	else
		hardsubx_process_frames_linear(ctx, enc_ctx);

	dinit_encoder(&enc_ctx, 0); //TODO: Replace 0 with end timestamp

	// Free the allocated memory for frame processing
	av_free(ctx->rgb_buffer);
	if (ctx->frame)
		av_frame_free(&ctx->frame);
	if (ctx->rgb_frame)
		av_frame_free(&ctx->rgb_frame);
	avcodec_close(ctx->codec_ctx);
	avformat_close_input(&ctx->format_ctx);
}

void _hardsubx_params_dump(struct ccx_s_options *options, struct lib_hardsubx_ctx *ctx)
{
	// Print the relevant passed parameters onto the screen
	mprint("Input : %s\n", ctx->inputfile[0]);

	switch (ctx->subcolor)
	{
		case HARDSUBX_COLOR_WHITE:
			mprint("Subtitle Color : White\n");
			break;
		case HARDSUBX_COLOR_YELLOW:
			mprint("Subtitle Color : Yellow\n");
			break;
		case HARDSUBX_COLOR_GREEN:
			mprint("Subtitle Color : Green\n");
			break;
		case HARDSUBX_COLOR_CYAN:
			mprint("Subtitle Color : Cyan\n");
			break;
		case HARDSUBX_COLOR_BLUE:
			mprint("Subtitle Color : Blue\n");
			break;
		case HARDSUBX_COLOR_MAGENTA:
			mprint("Subtitle Color : Magenta\n");
			break;
		case HARDSUBX_COLOR_RED:
			mprint("Subtitle Color : Red\n");
			break;
		case HARDSUBX_COLOR_CUSTOM:
			mprint("Subtitle Color : Custom Hue - %0.2f\n", ctx->hue);
			break;
		default:
			fatal(EXIT_MALFORMED_PARAMETER, "Invalid Subtitle Color");
	}

	switch (ctx->ocr_mode)
	{
		case HARDSUBX_OCRMODE_WORD:
			mprint("OCR Mode : Word-wise\n");
			break;
		case HARDSUBX_OCRMODE_LETTER:
			mprint("OCR Mode : Letter-wise\n");
			break;
		case HARDSUBX_OCRMODE_FRAME:
			mprint("OCR Mode : Frame-wise (simple)\n");
			break;
		default:
			fatal(EXIT_MALFORMED_PARAMETER, "Invalid OCR Mode");
	}

	if (ctx->conf_thresh > 0)
	{
		mprint("OCR Confidence Threshold : %.2f\n", ctx->conf_thresh);
	}
	else
	{
		mprint("OCR Confidence Threshold : %.2f (Default)\n", ctx->conf_thresh);
	}

	if (ctx->subcolor == HARDSUBX_COLOR_WHITE)
	{
		if (ctx->lum_thresh != 95.0)
		{
			mprint("OCR Luminance Threshold : %.2f\n", ctx->lum_thresh);
		}
		else
		{
			mprint("OCR Luminance Threshold : %.2f (Default)\n", ctx->lum_thresh);
		}
	}

	if (ctx->detect_italics == 1)
	{
		mprint("OCR Italic Detection : On\n");
	}
	else
	{
		mprint("OCR Italic Detection : Off\n");
	}

	if (ctx->min_sub_duration == 0.5)
	{
		mprint("Minimum subtitle duration : 0.5 seconds (Default)\n");
	}
	else
	{
		mprint("Minimum subtitle duration : %0.2f seconds\n", ctx->min_sub_duration);
	}

	mprint("FFMpeg Media Information:-\n");
}

struct lib_hardsubx_ctx *_init_hardsubx(struct ccx_s_options *options)
{
	// Initialize HardsubX data structures
	struct lib_hardsubx_ctx *ctx = (struct lib_hardsubx_ctx *)malloc(sizeof(struct lib_hardsubx_ctx));
	if (!ctx)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory for HardsubX data structures.");
	memset(ctx, 0, sizeof(struct lib_hardsubx_ctx));

	ctx->tess_handle = TessBaseAPICreate();
	char *pars_vec = strdup("debug_file");
	char *pars_values = strdup("/dev/null");
	char *tessdata_path = NULL;

	char *lang = options->ocrlang;
	if (!lang)
		lang = "eng"; // English is default language

	tessdata_path = probe_tessdata_location(lang);
	if (!tessdata_path)
	{
		if (strcmp(lang, "eng") == 0)
		{
			mprint("eng.traineddata not found! No Switching Possible\n");
			return NULL;
		}
		mprint("%s.traineddata not found! Switching to English\n", lang);
		lang = "eng";
		tessdata_path = probe_tessdata_location("eng");
		if (!tessdata_path)
		{
			mprint("eng.traineddata not found! No Switching Possible\n");
			return NULL;
		}
	}

	int ret = -1;

	if (!strncmp("4.", TessVersion(), 2))
	{
		char tess_path[1024];
		if (ccx_options.ocr_oem < 0)
			ccx_options.ocr_oem = 1;
		snprintf(tess_path, 1024, "%s%s%s", tessdata_path, "/", "tessdata");
		ret = TessBaseAPIInit4(ctx->tess_handle, tess_path, lang, ccx_options.ocr_oem, NULL, 0, &pars_vec,
				       &pars_values, 1, false);
	}
	else
	{
		if (ccx_options.ocr_oem < 0)
			ccx_options.ocr_oem = 0;
		ret = TessBaseAPIInit4(ctx->tess_handle, tessdata_path, lang, ccx_options.ocr_oem, NULL, 0, &pars_vec,
				       &pars_values, 1, false);
	}

	free(pars_vec);
	free(pars_values);
	if (ret != 0)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to initialize Tesseract");
	}

	//Initialize attributes common to lib_ccx context
	ctx->basefilename = get_basename(options->output_filename); //TODO: Check validity, add stdin, network
	ctx->current_file = -1;
	ctx->inputfile = options->inputfile;
	ctx->num_input_files = options->num_input_files;
	ctx->extension = get_file_extension(options->write_format);
	ctx->write_format = options->write_format;
	ctx->subs_delay = options->subs_delay;
	ctx->cc_to_stdout = options->cc_to_stdout;

	//Initialize subtitle text parameters
	ctx->tickertext = options->tickertext;
	ctx->cur_conf = 0.0;
	ctx->prev_conf = 0.0;
	ctx->ocr_mode = options->hardsubx_ocr_mode;
	ctx->subcolor = options->hardsubx_subcolor;
	ctx->min_sub_duration = options->hardsubx_min_sub_duration;
	ctx->detect_italics = options->hardsubx_detect_italics;
	ctx->conf_thresh = options->hardsubx_conf_thresh;
	ctx->hue = options->hardsubx_hue;
	ctx->lum_thresh = options->hardsubx_lum_thresh;

	//Initialize subtitle structure memory
	ctx->dec_sub = (struct cc_subtitle *)malloc(sizeof(struct cc_subtitle));
	memset(ctx->dec_sub, 0, sizeof(struct cc_subtitle));

	return ctx;
}

void _dinit_hardsubx(struct lib_hardsubx_ctx **ctx)
{
	struct lib_hardsubx_ctx *lctx = *ctx;
	// Free all memory allocated to everything in the context

	// Free OCR
	TessBaseAPIEnd(lctx->tess_handle);
	TessBaseAPIDelete(lctx->tess_handle);

	//Free subtitle
	freep(lctx->dec_sub);
	freep(ctx);
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

	// Configure output settings

	// Data processing loop
	time_t start, end;
	time(&start);
	hardsubx_process_data(ctx);

	// Show statistics (time taken, frames processed, mode etc)
	time(&end);
	long processing_time = (long)(end - start);
	mprint("\rDone, processing time = %ld seconds\n", processing_time);

	// Free all allocated memory for the data structures
	_dinit_hardsubx(&ctx);
}

#endif
