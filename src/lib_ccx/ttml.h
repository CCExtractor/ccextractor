#ifndef TTML_H
#define TTML_H

#include "ccx_common_platform.h"
#include "ccx_encoders_common.h"

// TTML encoder context
struct ttml_ctx {
    struct encoder_ctx *parent;
    int wrote_header;
    int64_t last_start_time;
    int64_t last_end_time;
};

// Initialize TTML encoder
void ttml_init(struct ttml_ctx *ctx, struct encoder_ctx *parent);

// Write TTML header
int ttml_write_header(struct ttml_ctx *ctx);

// Write TTML subtitle entry
int ttml_write_subtitle(struct ttml_ctx *ctx, struct cc_subtitle *sub);

// Write TTML footer
int ttml_write_footer(struct ttml_ctx *ctx);

// Encode a single screen to TTML
int write_ttml(struct cc_subtitle *sub, struct encoder_ctx *context);

#endif // TTML_H
