#ifndef _CC_DECODER_COMMON
#define _CC_DECODER_COMMON

#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_decoders_structs.h"

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
#endif
