#ifndef _CC_DECODER_COMMON
#define _CC_DECODER_COMMON

#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_common_structs.h"
#include "ccx_decoders_structs.h"
#include "ccx_common_option.h"
#include "ccx_encoders_common.h"

extern uint64_t utc_refvalue; // UTC referential value

// Declarations
LLONG get_visible_start (struct ccx_common_timing_ctx *ctx, int current_field);
LLONG get_visible_end (struct ccx_common_timing_ctx *ctx, int current_field);

unsigned int get_decoder_str_basic(unsigned char *buffer, unsigned char *line, int trim_subs, enum ccx_encoding_type encoding);

void ccx_decoders_common_settings_init(LLONG subs_delay, enum ccx_output_format output_format);

int validate_cc_data_pair (unsigned char *cc_data_pair);
int process_cc_data (struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, unsigned char *cc_data, int cc_count, struct cc_subtitle *sub);
int do_cb (struct lib_cc_decode *ctx, unsigned char *cc_block, struct cc_subtitle *sub);
void printdata (struct lib_cc_decode *ctx, const unsigned char *data1, int length1,
                const unsigned char *data2, int length2, struct cc_subtitle *sub);
struct lib_cc_decode* init_cc_decode (struct ccx_decoders_common_settings_t *setting);
void dinit_cc_decode(struct lib_cc_decode **ctx);
void flush_cc_decode(struct lib_cc_decode *ctx, struct cc_subtitle *sub);
struct encoder_ctx* copy_encoder_context(struct encoder_ctx *ctx);
struct lib_cc_decode* copy_decoder_context(struct lib_cc_decode *ctx);
struct cc_subtitle* copy_subtitle(struct cc_subtitle *sub);
void free_encoder_context(struct encoder_ctx *ctx);
void free_decoder_context(struct lib_cc_decode *ctx);
void free_subtitle(struct cc_subtitle* sub);
#endif
