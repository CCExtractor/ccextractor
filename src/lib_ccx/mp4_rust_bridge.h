#ifndef MP4_RUST_BRIDGE_H
#define MP4_RUST_BRIDGE_H

#include "lib_ccx.h"
#include "ccx_encoders_common.h"

/* Bridge functions exposing internal mp4.c processing to Rust */
int ccx_mp4_process_avc_sample(struct lib_ccx_ctx *ctx, uint32_t timescale,
			       uint8_t nal_unit_size, const uint8_t *data,
			       uint32_t data_len, uint64_t dts,
			       uint32_t cts_offset, struct cc_subtitle *sub);

int ccx_mp4_process_hevc_sample(struct lib_ccx_ctx *ctx, uint32_t timescale,
				uint8_t nal_unit_size, const uint8_t *data,
				uint32_t data_len, uint64_t dts,
				uint32_t cts_offset, struct cc_subtitle *sub);

/* CC track types for ccx_mp4_process_cc_packet */
#define CCX_MP4_TRACK_C608 0
#define CCX_MP4_TRACK_C708 1
#define CCX_MP4_TRACK_TX3G 2

int ccx_mp4_process_cc_packet(struct lib_ccx_ctx *ctx, int track_type,
			      uint32_t timescale, const uint8_t *data,
			      uint32_t data_len, uint64_t dts,
			      struct cc_subtitle *sub);

void ccx_mp4_flush_tx3g(struct lib_ccx_ctx *ctx, struct cc_subtitle *sub);

#endif /* MP4_RUST_BRIDGE_H */
