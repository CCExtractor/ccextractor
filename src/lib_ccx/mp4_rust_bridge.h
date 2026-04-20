/*
 * C bridge functions for the Rust FFmpeg MP4 demuxer.
 * These expose existing C processing functions with flat, FFI-safe signatures
 * that Rust can call via bindgen.
 */
#ifndef MP4_RUST_BRIDGE_H
#define MP4_RUST_BRIDGE_H

#ifdef ENABLE_FFMPEG_MP4

#include "lib_ccx.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/*
	 * Process an AVC (H.264) or HEVC (H.265) sample through do_NAL().
	 * Iterates the AVCC/HVCC length-prefixed NAL units in `data`.
	 *
	 * @param ctx           CCExtractor library context
	 * @param timescale     Media timescale for timestamp conversion
	 * @param nal_unit_size NAL unit length field size (1, 2, or 4 bytes)
	 * @param is_hevc       Non-zero for HEVC, zero for AVC
	 * @param data          Sample data buffer
	 * @param data_length   Length of the sample data
	 * @param dts           Decoding timestamp
	 * @param cts_offset    Composition time offset
	 * @param sub           Output subtitle structure
	 * @return 0 on success, -1 on unexpected nal_unit_size
	 */
	int ccx_mp4_process_nal_sample(struct lib_ccx_ctx *ctx, unsigned int timescale,
				       unsigned char nal_unit_size, int is_hevc,
				       unsigned char *data, unsigned int data_length,
				       long long dts, int cts_offset,
				       struct cc_subtitle *sub);

	/*
	 * Process a closed caption packet (CEA-608/708).
	 * Wraps the existing process_clcp() atom-level logic with flat arguments.
	 *
	 * @param ctx           CCExtractor library context
	 * @param sub_type_c608 1 if CEA-608, 0 if CEA-708
	 * @param data          CC atom data (starting from the atom header)
	 * @param data_length   Length of the data
	 * @param dts           Decoding timestamp
	 * @param cts_offset    Composition time offset
	 * @param timescale     Media timescale
	 * @param sub           Output subtitle structure
	 * @return 0 on success, non-zero on error
	 */
	int ccx_mp4_process_cc_packet(struct lib_ccx_ctx *ctx, int sub_type_c608,
				      unsigned char *data, unsigned int data_length,
				      long long dts, int cts_offset,
				      unsigned int timescale,
				      struct cc_subtitle *sub);

	/*
	 * Process a tx3g (3GPP timed text) subtitle sample.
	 *
	 * @param ctx           CCExtractor library context
	 * @param data          tx3g sample data
	 * @param data_length   Length of the data
	 * @param dts           Decoding timestamp
	 * @param cts_offset    Composition time offset
	 * @param timescale     Media timescale
	 * @param sub           Output subtitle structure
	 * @return 0 on success, non-zero on error
	 */
	int ccx_mp4_process_tx3g_packet(struct lib_ccx_ctx *ctx,
					unsigned char *data, unsigned int data_length,
					long long dts, int cts_offset,
					unsigned int timescale,
					struct cc_subtitle *sub);

	/*
	 * Flush pending tx3g subtitle (encode the last one).
	 *
	 * @param ctx  CCExtractor library context
	 * @param sub  Output subtitle structure
	 */
	void ccx_mp4_flush_tx3g(struct lib_ccx_ctx *ctx, struct cc_subtitle *sub);

	/*
	 * Report progress for the MP4 demuxer.
	 *
	 * @param ctx      CCExtractor library context
	 * @param cur      Current sample index
	 * @param total    Total sample count
	 */
	void ccx_mp4_report_progress(struct lib_ccx_ctx *ctx, unsigned int cur, unsigned int total);

	/*
	 * VobSub (DVD subtitle) bridge.
	 * Wraps vobsub_decoder.c for use from the Rust FFmpeg path.
	 */
	void *ccx_mp4_vobsub_init(void);
	int ccx_mp4_vobsub_process(void *vob_ctx, struct lib_ccx_ctx *ctx,
				   unsigned char *data, unsigned int data_length,
				   long long start_ms, long long end_ms,
				   struct cc_subtitle *sub);
	void ccx_mp4_vobsub_free(void *vob_ctx);

#ifdef __cplusplus
}
#endif

#endif /* ENABLE_FFMPEG_MP4 */
#endif /* MP4_RUST_BRIDGE_H */
