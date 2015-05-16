#ifndef _CC_ENCODER_COMMON_H
#define _CC_ENCODER_COMMON_H

#include "ccx_common_structs.h"
#include "ccx_decoders_structs.h"
#include "ccx_encoders_structs.h"
#include "ccx_encoders_helpers.h"
#include "ccx_common_option.h"

#define REQUEST_BUFFER_CAPACITY(ctx,length) if (length>ctx->capacity) \
{ctx->capacity = length * 2; ctx->buffer = (unsigned char*)realloc(ctx->buffer, ctx->capacity); \
if (ctx->buffer == NULL) { fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory, bailing out\n"); } \
}

extern ccx_encoders_transcript_format ccx_encoders_default_transcript_settings;

/**
 * Context of encoder, This structure gives single interface
 * to all encoder
 */
struct encoder_ctx
{
	/* common buffer used by all encoder */
	unsigned char *buffer;
	/* capacity of buffer */
	unsigned int capacity;
	/* keep count of srt subtitle*/
	unsigned int srt_counter;
	/* output context */
	struct ccx_s_write *out;
	/* start time of previous sub */
	LLONG prev_start;

	LLONG subs_delay;
	LLONG last_displayed_subs_ms;
	int startcredits_displayed;
	enum ccx_encoding_type encoding;
	enum ccx_output_date_format date_format;
	char millis_separator;
	int sentence_cap ; // FIX CASE? = Fix case?
	int trim_subs; // "    Remove spaces at sides?    "
	int autodash; // Add dashes (-) before each speaker automatically?
	enum ccx_output_format write_format; // 0=Raw, 1=srt, 2=SMI
	/* Credit stuff */
	char *start_credits_text;
	char *end_credits_text;
	struct ccx_encoders_transcript_format *transcript_settings; // Keeps the settings for generating transcript output files.
	struct ccx_boundary_time startcreditsnotbefore, startcreditsnotafter; // Where to insert start credits, if possible
	struct ccx_boundary_time startcreditsforatleast, startcreditsforatmost; // How long to display them?
	struct ccx_boundary_time endcreditsforatleast, endcreditsforatmost;
	unsigned int teletext_mode; // 0=Disabled, 1 = Not found, 2=Found
	unsigned int send_to_srv;
	int gui_mode_reports; // If 1, output in stderr progress updates so the GUI can grab them
};

#define INITIAL_ENC_BUFFER_CAPACITY	2048
/**
 * Inialize encoder context with output context
 * allocate initial memory to buffer of context
 * write subtitle header to file refrenced by
 * output context
 *
 * @param ctx preallocated encoder ctx
 * @param out output context
 * @param opt Option to initilaize encoder cfg params
 *
 * @return 0 on SUCESS, -1 on failure
 */
int init_encoder(struct encoder_ctx *ctx, struct ccx_s_write *out, struct ccx_s_options *opt);

/**
 * try to add end credits in subtitle file and then write subtitle
 * footer
 *
 * deallocate encoder ctx, so before using encoder_ctx again
 * after deallocating user need to allocate encoder ctx again
 *
 * @oaram ctx Initialized encoder ctx using init_encoder
 */
void dinit_encoder(struct encoder_ctx *ctx);

/**
 * @param ctx encoder context
 * @param sub subtitle context returned by decoder
 */
int encode_sub(struct encoder_ctx *ctx,struct cc_subtitle *sub);

int write_cc_buffer_as_srt(struct eia608_screen *data, struct encoder_ctx *context);
void write_stringz_as_srt(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);

int write_cc_buffer_as_sami(struct eia608_screen *data, struct encoder_ctx *context);
void write_stringz_as_sami(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);

int write_cc_buffer_as_smptett(struct eia608_screen *data, struct encoder_ctx *context);
void write_stringz_as_smptett(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);
void write_cc_buffer_to_gui(struct eia608_screen *data, struct encoder_ctx *context);

int write_cc_bitmap_as_spupng(struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_as_srt(struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_as_sami(struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_as_smptett(struct cc_subtitle *sub, struct encoder_ctx *context);


void set_encoder_last_displayed_subs_ms(struct encoder_ctx *ctx, LLONG last_displayed_subs_ms);
void set_encoder_subs_delay(struct encoder_ctx *ctx, LLONG subs_delay);
void set_encoder_startcredits_displayed(struct encoder_ctx *ctx, int startcredits_displayed);
#endif
