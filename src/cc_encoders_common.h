#ifndef _CC_ENCODER_COMMON_H
#define _CC_ENCODER_COMMON_H

struct encoder_ctx
{
	unsigned char *buffer;
	unsigned int capacity;
	unsigned int srt_counter;
	struct ccx_s_write *out;
};

#define INITIAL_ENC_BUFFER_CAPACITY	2048
int init_encoder(struct encoder_ctx *ctx,struct ccx_s_write *out);
void dinit_encoder(struct encoder_ctx *ctx);
int encode_sub(struct encoder_ctx *ctx,struct cc_subtitle *sub);

int write_cc_buffer_as_srt(struct eia608_screen *data, struct encoder_ctx *context);
void write_stringz_as_srt(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);

int write_cc_buffer_as_sami(struct eia608_screen *data, struct encoder_ctx *context);
void write_stringz_as_sami(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);

int write_cc_buffer_as_smptett(struct eia608_screen *data, struct encoder_ctx *context);
void write_stringz_as_smptett(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);
#endif
