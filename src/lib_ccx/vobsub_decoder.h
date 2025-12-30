#ifndef VOBSUB_DECODER_H
#define VOBSUB_DECODER_H

#include "ccx_decoders_structs.h"

/**
 * VOBSUB decoder context - opaque structure
 */
struct vobsub_ctx;

/**
 * Initialize VOBSUB decoder context
 * @return Pointer to context, or NULL on failure
 */
struct vobsub_ctx *init_vobsub_decoder(void);

/**
 * Parse palette from idx header string (e.g., from MKV CodecPrivate)
 * Looks for "palette:" line and parses 16 hex RGB colors
 * @param ctx VOBSUB decoder context
 * @param idx_header The idx header string containing palette info
 * @return 0 on success, -1 on failure
 */
int vobsub_parse_palette(struct vobsub_ctx *ctx, const char *idx_header);

/**
 * Decode single SPU packet and optionally perform OCR
 * @param ctx VOBSUB decoder context
 * @param spu_data Raw SPU data (starting with 2-byte size)
 * @param spu_size Size of SPU data
 * @param start_time Start time in milliseconds
 * @param end_time End time in milliseconds (0 if unknown)
 * @param sub Output subtitle structure
 * @return 0 on success, -1 on error
 */
int vobsub_decode_spu(struct vobsub_ctx *ctx,
		      unsigned char *spu_data, size_t spu_size,
		      long long start_time, long long end_time,
		      struct cc_subtitle *sub);

/**
 * Check if VOBSUB OCR is available (compiled with OCR support)
 * @return 1 if OCR available, 0 otherwise
 */
int vobsub_ocr_available(void);

/**
 * Free VOBSUB decoder context and resources
 * @param ctx Pointer to context pointer (will be set to NULL)
 */
void delete_vobsub_decoder(struct vobsub_ctx **ctx);

#endif /* VOBSUB_DECODER_H */
