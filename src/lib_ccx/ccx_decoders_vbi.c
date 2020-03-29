#include "ccx_decoders_vbi.h"
#include "ccx_decoders_common.h"
#include "ccx_common_constants.h"
#include "ccx_common_common.h"
#include "utility.h"
#include "stdlib.h"

void delete_decoder_vbi(struct ccx_decoder_vbi_ctx **arg)
{
	struct ccx_decoder_vbi_ctx *ctx = *arg;
	vbi_raw_decoder_destroy(&ctx->zvbi_decoder);

	freep(arg);
}
struct ccx_decoder_vbi_ctx *init_decoder_vbi(struct ccx_decoder_vbi_cfg *cfg)
{
	struct ccx_decoder_vbi_ctx *vbi;

	vbi = malloc(sizeof(*vbi));
	if (!vbi)
		return NULL;

#ifdef VBI_DEBUG
	vbi->vbi_debug_dump = fopen("dump_720.vbi", "w");
#endif
	vbi_raw_decoder_init(&vbi->zvbi_decoder);

	if (cfg == NULL)
	{
		/* Specify the image format. */
		vbi->zvbi_decoder.scanning = 525;

		/* The decoder ignores chroma data. */
		vbi->zvbi_decoder.sampling_format = VBI_PIXFMT_YUV420;

		/* You may have to adjust this. */
		vbi->zvbi_decoder.sampling_rate = 13.5e6; /* Hz */
		vbi->zvbi_decoder.bytes_per_line = 720;

		/* Sampling starts 9.7 µs from the front edge of the
		hor. sync pulse. You may have to adjust this. */
		vbi->zvbi_decoder.offset = 9.7e-6 * 13.5e6;

		/* Which lines were captured from the first field.
			You may have to adjust this. */
		vbi->zvbi_decoder.start[0] = 21;
		vbi->zvbi_decoder.count[0] = 1;

		/* Second field. */
		vbi->zvbi_decoder.start[1] = 284;
		vbi->zvbi_decoder.count[1] = 1;

		/* The image contains all lines of the first field,
			followed by all lines of the second field. */
		vbi->zvbi_decoder.interlaced = CCX_TRUE;

		/* The first field is always first in memory. */
		vbi->zvbi_decoder.synchronous = CCX_TRUE;

		/* Specify the services you want. */
		vbi_raw_decoder_add_services(&vbi->zvbi_decoder, VBI_SLICED_CAPTION_525, /* strict */ 0);
	}
	return vbi;
}

int decode_vbi(struct lib_cc_decode *dec_ctx, uint8_t field, unsigned char *buffer, size_t len, struct cc_subtitle *sub)
{
	int i = 0;
	unsigned int n_lines;
	vbi_sliced sliced[52];
	if (dec_ctx->vbi_decoder == NULL)
	{
		dec_ctx->vbi_decoder = init_decoder_vbi(NULL);
	}

	len -= 720;

	n_lines = vbi_raw_decode(&dec_ctx->vbi_decoder->zvbi_decoder, buffer, sliced);
	//n_lines = vbi3_raw_decoder_decode (&dec_ctx->vbi_decoder->zvbi_decoder, sliced, 2, buffer);
	if (n_lines > 0)
	{
		for (i = 0; i < n_lines; ++i)
		{
			int index = 0;
			//for(index = 0; index < 56; index += 2)
			{
				unsigned char data[3];
				if (field == 1)
					data[0] = 0x04;
				else
					data[0] = 0x05;
				data[1] = sliced[i].data[index];
				data[2] = sliced[i].data[index + 1];
				do_cb(dec_ctx, data, sub);
			}
		}
	}
	return CCX_OK;
}
