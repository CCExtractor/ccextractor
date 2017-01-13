#ifndef CCX_DECODER_VBI
#define CCX_DECODER_VBI

#include <zvbi_decoder.h>
#define VBI_DEBUG

#include "ccx_decoders_structs.h"
#include "ccx_decoders_common.h"

struct ccx_decoder_vbi_cfg
{
#ifdef VBI_DEBUG
	char *debug_file_name;
#endif
};

struct ccx_decoder_vbi_ctx
{
	int vbi_decoder_inited;
	vbi_raw_decoder zvbi_decoder;
	//vbi3_raw_decoder zvbi_decoder;
#ifdef VBI_DEBUG
	FILE *vbi_debug_dump;
#endif
};


int decode_vbi(struct lib_cc_decode *dec_ctx, uint8_t field, unsigned char *buffer, size_t len,  struct cc_subtitle *sub);
#endif
