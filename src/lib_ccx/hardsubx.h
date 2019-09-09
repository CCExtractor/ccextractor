#ifndef HARDSUBX_H
#define HARDSUBX_H

#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_HARDSUBX
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>

enum hardsubx_color_type
{
	HARDSUBX_COLOR_WHITE = 0,
	HARDSUBX_COLOR_YELLOW = 1,
	HARDSUBX_COLOR_GREEN = 2,
	HARDSUBX_COLOR_CYAN = 3,
	HARDSUBX_COLOR_BLUE = 4,
	HARDSUBX_COLOR_MAGENTA = 5,
	HARDSUBX_COLOR_RED = 6,
	HARDSUBX_COLOR_CUSTOM = 7,
};

enum hardsubx_ocr_mode
{
	HARDSUBX_OCRMODE_FRAME = 0,
	HARDSUBX_OCRMODE_WORD = 1,
	HARDSUBX_OCRMODE_LETTER = 2,
};

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
	float cur_conf;
	float prev_conf;

	// Subtitle text parameters
	int tickertext;
	struct cc_subtitle *dec_sub;
	int ocr_mode;
	int subcolor;
	float min_sub_duration;
	int detect_italics;
	float conf_thresh;
	float hue;
	float lum_thresh;
};

struct lib_hardsubx_ctx* _init_hardsubx(struct ccx_s_options *options);
void _hardsubx_params_dump(struct ccx_s_options *options, struct lib_hardsubx_ctx *ctx);
void hardsubx(struct ccx_s_options *options);
void _dinit_hardsubx(struct lib_hardsubx_ctx **ctx);
int hardsubx_process_data(struct lib_hardsubx_ctx *ctx);

//hardsubx_decoder.c
void hardsubx_process_frames_linear(struct lib_hardsubx_ctx *ctx, struct encoder_ctx *enc_ctx);
int hardsubx_process_frames_tickertext(struct lib_hardsubx_ctx *ctx, struct encoder_ctx *enc_ctx);
void hardsubx_process_frames_binary(struct lib_hardsubx_ctx *ctx);
char* _process_frame_white_basic(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int index);
char *_process_frame_color_basic(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int index);
void _display_frame(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int timestamp);
char* _process_frame_tickertext(struct lib_hardsubx_ctx *ctx, AVFrame *frame, int width, int height, int index);

//hardsubx_imgops.c
void rgb_to_hsv(float R, float G, float B,float *H, float *S, float *V);
void rgb_to_lab(float R, float G, float B,float *L, float *a, float *b);

//hardsubx_classifier.c
char *get_ocr_text_simple(struct lib_hardsubx_ctx *ctx, PIX *image);
char *get_ocr_text_wordwise(struct lib_hardsubx_ctx *ctx, PIX *image);
char *get_ocr_text_letterwise(struct lib_hardsubx_ctx *ctx, PIX *image);
char *get_ocr_text_simple_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold);
char *get_ocr_text_wordwise_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold);
char *get_ocr_text_letterwise_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold);

//hardsubx_utility.c
int edit_distance(char * word1, char * word2, int len1, int len2);
int64_t convert_pts_to_ms(int64_t pts, AVRational time_base);
int64_t convert_pts_to_ns(int64_t pts, AVRational time_base);
int64_t convert_pts_to_s(int64_t pts, AVRational time_base);
int is_valid_trailing_char(char c);
char *prune_string(char *s);

#endif

#endif
