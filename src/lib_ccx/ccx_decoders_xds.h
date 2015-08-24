#ifndef CCX_DECODER_XDS_H
#define CCX_DECODER_XDS_H

#include "ccx_decoders_common.h"

struct ccx_decoders_xds_context;
void process_xds_bytes (struct ccx_decoders_xds_context *ctx, const unsigned char hi, int lo);
void do_end_of_xds (struct cc_subtitle *sub, struct ccx_decoders_xds_context *ctx, unsigned char expected_checksum);

struct ccx_decoders_xds_context *ccx_decoders_xds_init_library(struct ccx_common_timing_ctx *timing);

void xds_cea608_test();
#endif
