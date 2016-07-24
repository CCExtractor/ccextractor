#ifndef HARDSUBX_H
#define HARDSUBX_H

#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_OCR
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "allheaders.h"
#include "capi.h"


struct lib_hardsubx_ctx
{
	// The main context for hard subtitle extraction

	// Attributes common to the lib_ccx context
	int cc_to_stdout;
	LLONG subs_delay;
	LLONG last_displayed_subs_ms;
	char *basefilename;
	const char *extension;
	int current_file;
	char **inputfile;
	int num_input_files; 
	LLONG system_start_time;
	enum ccx_output_format write_format;

	// Media file context
	AVFormatContext *format_ctx;
	AVCodecContext *codec_ctx;
	AVCodec *codec;
	AVFrame *frame;
	AVFrame *rgb_frame;
	AVPacket packet;
	AVDictionary *options_dict;
	struct SwsContext *sws_ctx;
	uint8_t *rgb_buffer;
	int video_stream_id;

	// Leptonica Image and Tesseract Context
	PIX *im;
	TessBaseAPI *tess_handle;

	// Classifier parameters

	// Subtitle text parameters
	struct cc_subtitle *dec_sub;
	float min_sub_duration;
};

struct lib_hardsubx_ctx* _init_hardsubx(struct ccx_s_options *options);
void _hardsubx_params_dump(struct ccx_s_options *options, struct lib_hardsubx_ctx *ctx);
void hardsubx(struct ccx_s_options *options);

//hardsubx_decoder.c
int hardsubx_process_frames_linear(struct lib_hardsubx_ctx *ctx, struct encoder_ctx *enc_ctx);
int hardsubx_process_frames_binary(struct lib_hardsubx_ctx *ctx);

//hardsubx_imgops.c
void rgb2hsv(float R, float G, float B,float *L, float *a, float *b);
void rgb2lab(float R, float G, float B,float *L, float *a, float *b);

//hardsubx_classifier.c
char *get_ocr_text_simple(struct lib_hardsubx_ctx *ctx, PIX *image);

//hardsubx_utility.c
int edit_distance(char * word1, char * word2, int len1, int len2);

#endif

#endif