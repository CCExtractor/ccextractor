#ifndef _CC_DECODER_COMMON
#define _CC_DECODER_COMMON

#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_common_structs.h"
#include "ccx_decoders_structs.h"
#include "ccx_common_option.h"

extern unsigned char encoded_crlf[16]; // We keep it encoded here so we don't have to do it many times
extern unsigned int encoded_crlf_length;
extern unsigned char encoded_br[16];
extern unsigned int encoded_br_length;

extern LLONG minimum_fts; // No screen should start before this FTS

extern uint64_t utc_refvalue; // UTC referential value

// Declarations
LLONG get_visible_start(void);
LLONG get_visible_end(void);

void find_limit_characters(unsigned char *line, int *first_non_blank, int *last_non_blank);
unsigned get_decoder_line_basic(unsigned char *buffer, int line_num, struct eia608_screen *data, int trim_subs, enum ccx_encoding_type encoding);

void ccx_decoders_common_settings_init(LLONG subs_delay, enum ccx_output_format output_format);

int validate_cc_data_pair (unsigned char *cc_data_pair);
int process_cc_data (struct lib_cc_decode *ctx, unsigned char *cc_data, int cc_count, struct cc_subtitle *sub);
int do_cb (struct lib_cc_decode *ctx, unsigned char *cc_block, struct cc_subtitle *sub);
void printdata (struct lib_cc_decode *ctx, const unsigned char *data1, int length1,
                const unsigned char *data2, int length2, struct cc_subtitle *sub);
struct lib_cc_decode* init_cc_decode (struct ccx_decoders_common_settings_t *setting);
void dinit_cc_decode(struct lib_cc_decode **ctx);
#endif
