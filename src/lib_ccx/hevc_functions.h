#ifndef HEVC_FUNCTIONS_H
#define HEVC_FUNCTIONS_H

#include "ccx_common_constants.h"  // This includes the enum definition
#include "lib_ccx.h"

struct hevc_ctx;

// Public function declarations only
struct hevc_ctx *init_hevc(void);
void dinit_hevc(struct hevc_ctx **ctx);
size_t process_hevc(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx,
                   unsigned char *hevc_buf, size_t hevc_buflen, struct cc_subtitle *sub);
void do_hevc_nal(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, 
                 unsigned char *nal_start, LLONG nal_length, struct cc_subtitle *sub);

#endif /* HEVC_FUNCTIONS_H */
