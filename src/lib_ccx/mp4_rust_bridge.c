#ifdef ENABLE_FFMPEG_MP4

#include "mp4_rust_bridge.h"
#include "ccx_gpac_types.h"
#include "lib_ccx.h"
#include "ccx_encoders_common.h"

/* Forward declare the actual functions from mp4.c */
int process_avc_sample(struct lib_ccx_ctx *ctx, uint32_t timescale,
		       GF_AVCConfig *c, GF_ISOSample *s,
		       struct cc_subtitle *sub);

int process_hevc_sample(struct lib_ccx_ctx *ctx, uint32_t timescale,
			GF_HEVCConfig *c, GF_ISOSample *s,
			struct cc_subtitle *sub);

int ccx_mp4_process_avc_sample(struct lib_ccx_ctx *ctx, uint32_t timescale,
			       uint8_t nal_unit_size, const uint8_t *data,
			       uint32_t data_len, uint64_t dts,
			       uint32_t cts_offset, struct cc_subtitle *sub)
{
	GF_AVCConfig cfg = {nal_unit_size};
	GF_ISOSample s = {
	    .data = (char *)data,
	    .dataLength = data_len,
	    .DTS = dts,
	    .CTS_Offset = cts_offset,
	};
	return process_avc_sample(ctx, timescale, &cfg, &s, sub);
}

int ccx_mp4_process_hevc_sample(struct lib_ccx_ctx *ctx, uint32_t timescale,
				uint8_t nal_unit_size, const uint8_t *data,
				uint32_t data_len, uint64_t dts,
				uint32_t cts_offset, struct cc_subtitle *sub)
{
	GF_HEVCConfig cfg = {nal_unit_size};
	GF_ISOSample s = {
	    .data = (char *)data,
	    .dataLength = data_len,
	    .DTS = dts,
	    .CTS_Offset = cts_offset,
	};
	return process_hevc_sample(ctx, timescale, &cfg, &s, sub);
}

#endif /* ENABLE_FFMPEG_MP4 */
