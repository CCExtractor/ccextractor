#ifndef _CC_ENCODER_COMMON_H
#define _CC_ENCODER_COMMON_H

/**
 * Context of encoder, This structure gives single interface 
 * to all encoder 
 */
struct encoder_ctx
{
	/** common buffer used by all encoder */
	unsigned char *buffer;
	/** capacity of buffer */
	unsigned int capacity;
	/* keep count of srt subtitle*/
	unsigned int srt_counter;
	/** output contet */
	struct ccx_s_write *out;
	/** start time of previous sub */
	LLONG prev_start;
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
 * 
 * @return 0 on SUCESS, -1 on failure
 */
int init_encoder(struct encoder_ctx *ctx,struct ccx_s_write *out);

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
#endif
