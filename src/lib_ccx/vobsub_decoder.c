/**
 * VOBSUB decoder with OCR support
 *
 * Decodes VOBSUB (DVD bitmap) subtitles from MKV, MP4, or standalone idx/sub files
 * and optionally performs OCR to convert to text.
 *
 * SPU (SubPicture Unit) format:
 * - 2 bytes: total SPU size
 * - 2 bytes: offset to control sequence
 * - RLE-encoded pixel data (interlaced)
 * - Control sequence with timing, colors, coordinates
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "lib_ccx.h"
#include "vobsub_decoder.h"
#include "ccx_common_common.h"
#include "ccx_decoders_structs.h"
#include "ccx_common_constants.h"

#ifdef ENABLE_OCR
#include "ocr.h"
#endif

#define RGBA(r, g, b, a) (((unsigned)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))

/* Control sequence structure */
struct vobsub_ctrl_seq
{
	uint8_t color[4];      /* Color indices */
	uint8_t alpha[4];      /* Alpha values */
	uint16_t coord[4];     /* x1, x2, y1, y2 */
	uint16_t pixoffset[2]; /* Offset to 1st and 2nd graphic line */
	uint16_t start_time;
	uint16_t stop_time;
};

struct vobsub_ctx
{
	uint32_t palette[16]; /* RGBA palette from idx header */
	int palette_parsed;   /* 1 if palette has been parsed */
	struct vobsub_ctrl_seq ctrl;
	unsigned char *bitmap; /* Decoded bitmap */
#ifdef ENABLE_OCR
	void *ocr_ctx; /* OCR context */
#endif
};

/* Get 4 bits from buffer for RLE decoding */
static int vobsub_get_bits(unsigned char *buffer, uint8_t *nextbyte, int *pos, int *m)
{
	int ret;
	ret = (*nextbyte & 0xf0) >> 4;
	if (*m == 0)
		*pos += 1;
	*nextbyte = (*nextbyte << 4) | ((*m) ? (buffer[*pos] & 0x0f) : ((buffer[*pos] & 0xf0) >> 4));
	*m = (*m + 1) % 2;
	return ret;
}

/* RLE decode to get run length and color */
static int vobsub_rle_decode(unsigned char *buffer, int *color, uint8_t *nextbyte, int *pos, int *m)
{
	int val = 4;
	uint16_t rlen = vobsub_get_bits(buffer, nextbyte, pos, m);
	while (rlen < val && val <= 0x40)
	{
		rlen = (rlen << 4) | vobsub_get_bits(buffer, nextbyte, pos, m);
		val = val << 2;
	}
	*color = rlen & 0x3;
	rlen = rlen >> 2;
	return rlen;
}

/* Decode bitmap from RLE-encoded SPU data */
static void vobsub_get_bitmap(struct vobsub_ctx *ctx, unsigned char *buffer, size_t buf_size)
{
	int w, h, x, lineno;
	int pos, color, m;
	int len;
	uint8_t nextbyte;
	unsigned char *buffp;

	w = (ctx->ctrl.coord[1] - ctx->ctrl.coord[0]) + 1;
	h = (ctx->ctrl.coord[3] - ctx->ctrl.coord[2]) + 1;

	if (w <= 0 || h <= 0 || w > 4096 || h > 4096)
	{
		dbg_print(CCX_DMT_VERBOSE, "VOBSUB: Invalid dimensions w=%d h=%d\n", w, h);
		return;
	}

	pos = ctx->ctrl.pixoffset[0];
	if (pos >= (int)buf_size)
	{
		dbg_print(CCX_DMT_VERBOSE, "VOBSUB: Pixel offset out of bounds\n");
		return;
	}

	m = 0;
	nextbyte = buffer[pos];

	ctx->bitmap = malloc(w * h);
	if (!ctx->bitmap)
		return;
	memset(ctx->bitmap, 0, w * h);

	buffp = ctx->bitmap;
	x = 0;
	lineno = 0;

	/* Decode first field (odd lines in interlaced) */
	while (lineno < (h + 1) / 2 && pos < (int)buf_size)
	{
		len = vobsub_rle_decode(buffer, &color, &nextbyte, &pos, &m);
		if (len > (w - x) || len == 0)
			len = w - x;

		memset(buffp + x, color, len);
		x += len;
		if (x >= w)
		{
			x = 0;
			++lineno;
			buffp += (2 * w); /* Skip 1 line due to interlacing */
			if ((m == 1))
			{
				vobsub_get_bits(buffer, &nextbyte, &pos, &m);
			}
		}
	}

	/* Decode second field (even lines) */
	if (pos > ctx->ctrl.pixoffset[1])
	{
		dbg_print(CCX_DMT_VERBOSE, "VOBSUB: Error creating bitmap - overlapping fields\n");
		return;
	}

	pos = ctx->ctrl.pixoffset[1];
	if (pos >= (int)buf_size)
	{
		dbg_print(CCX_DMT_VERBOSE, "VOBSUB: Second field offset out of bounds\n");
		return;
	}

	buffp = ctx->bitmap + w;
	x = 0;
	lineno = 0;
	m = 0;
	nextbyte = buffer[pos];

	while (lineno < h / 2 && pos < (int)buf_size)
	{
		len = vobsub_rle_decode(buffer, &color, &nextbyte, &pos, &m);
		if (len > (w - x) || len == 0)
			len = w - x;

		memset(buffp + x, color, len);
		x += len;
		if (x >= w)
		{
			x = 0;
			++lineno;
			buffp += (2 * w);
			if ((m == 1))
			{
				vobsub_get_bits(buffer, &nextbyte, &pos, &m);
			}
		}
	}
}

/* Parse control sequence from SPU data */
static void vobsub_decode_control(struct vobsub_ctx *ctx, unsigned char *buffer, size_t buf_size, uint16_t ctrl_offset)
{
	int pos = ctrl_offset;
	int pack_end = 0;
	uint16_t date, next_ctrl;

	memset(&ctx->ctrl, 0, sizeof(ctx->ctrl));

	while (pos + 4 <= (int)buf_size && pack_end == 0)
	{
		date = (buffer[pos] << 8) | buffer[pos + 1];
		next_ctrl = (buffer[pos + 2] << 8) | buffer[pos + 3];
		if (next_ctrl == pos)
			pack_end = 1;
		pos += 4;

		int seq_end = 0;
		while (seq_end == 0 && pos < (int)buf_size)
		{
			int command = buffer[pos++];
			switch (command)
			{
				case 0x01: /* Start display */
					ctx->ctrl.start_time = (date << 10) / 90;
					break;
				case 0x02: /* Stop display */
					ctx->ctrl.stop_time = (date << 10) / 90;
					break;
				case 0x03: /* SET_COLOR */
					if (pos + 2 > (int)buf_size)
						break;
					ctx->ctrl.color[3] = (buffer[pos] & 0xf0) >> 4;
					ctx->ctrl.color[2] = buffer[pos] & 0x0f;
					ctx->ctrl.color[1] = (buffer[pos + 1] & 0xf0) >> 4;
					ctx->ctrl.color[0] = buffer[pos + 1] & 0x0f;
					pos += 2;
					break;
				case 0x04: /* SET_CONTR (alpha) */
					if (pos + 2 > (int)buf_size)
						break;
					ctx->ctrl.alpha[3] = (buffer[pos] & 0xf0) >> 4;
					ctx->ctrl.alpha[2] = buffer[pos] & 0x0f;
					ctx->ctrl.alpha[1] = (buffer[pos + 1] & 0xf0) >> 4;
					ctx->ctrl.alpha[0] = buffer[pos + 1] & 0x0f;
					pos += 2;
					break;
				case 0x05: /* SET_DAREA (coordinates) */
					if (pos + 6 > (int)buf_size)
						break;
					ctx->ctrl.coord[0] = ((buffer[pos] << 8) | (buffer[pos + 1] & 0xf0)) >> 4;
					ctx->ctrl.coord[1] = ((buffer[pos + 1] & 0x0f) << 8) | buffer[pos + 2];
					ctx->ctrl.coord[2] = ((buffer[pos + 3] << 8) | (buffer[pos + 4] & 0xf0)) >> 4;
					ctx->ctrl.coord[3] = ((buffer[pos + 4] & 0x0f) << 8) | buffer[pos + 5];
					pos += 6;
					break;
				case 0x06: /* SET_DSPXA (pixel offset) */
					if (pos + 4 > (int)buf_size)
						break;
					ctx->ctrl.pixoffset[0] = (buffer[pos] << 8) | buffer[pos + 1];
					ctx->ctrl.pixoffset[1] = (buffer[pos + 2] << 8) | buffer[pos + 3];
					pos += 4;
					break;
				case 0x07: /* Extended command */
					if (pos + 2 > (int)buf_size)
						break;
					{
						uint16_t skip = (buffer[pos] << 8) | buffer[pos + 1];
						pos += skip;
					}
					break;
				case 0xff: /* End of control sequence */
					seq_end = 1;
					break;
				default:
					dbg_print(CCX_DMT_VERBOSE, "VOBSUB: Unknown control command 0x%02x\n", command);
					break;
			}
		}
	}
}

/* Generate RGBA palette from color/alpha indices using parsed palette */
static void vobsub_generate_rgba_palette(struct vobsub_ctx *ctx, uint32_t *rgba_palette)
{
	for (int i = 0; i < 4; i++)
	{
		if (ctx->ctrl.alpha[i] == 0)
		{
			rgba_palette[i] = 0; /* Fully transparent */
		}
		else if (ctx->palette_parsed)
		{
			/* Use parsed palette from idx header */
			uint32_t color = ctx->palette[ctx->ctrl.color[i] & 0x0f];
			uint8_t r = (color >> 16) & 0xff;
			uint8_t g = (color >> 8) & 0xff;
			uint8_t b = color & 0xff;
			uint8_t a = ctx->ctrl.alpha[i] * 17; /* Scale 0-15 to 0-255 */
			rgba_palette[i] = RGBA(r, g, b, a);
		}
		else
		{
			/* Fallback: guess palette (grayscale levels) */
			static const uint8_t level_map[4][4] = {
			    {0xff},
			    {0x00, 0xff},
			    {0x00, 0x80, 0xff},
			    {0x00, 0x55, 0xaa, 0xff},
			};

			/* Count opaque colors */
			int nb_opaque = 0;
			for (int j = 0; j < 4; j++)
				if (ctx->ctrl.alpha[j] != 0)
					nb_opaque++;

			if (nb_opaque == 0)
				nb_opaque = 1;
			if (nb_opaque > 4)
				nb_opaque = 4;

			int level = level_map[nb_opaque - 1][i < nb_opaque ? i : nb_opaque - 1];
			uint8_t a = ctx->ctrl.alpha[i] * 17;
			rgba_palette[i] = RGBA(level, level, level, a);
		}
	}
}

struct vobsub_ctx *init_vobsub_decoder(void)
{
	struct vobsub_ctx *ctx = malloc(sizeof(struct vobsub_ctx));
	if (!ctx)
		return NULL;

	memset(ctx, 0, sizeof(struct vobsub_ctx));

#ifdef ENABLE_OCR
	ctx->ocr_ctx = init_ocr(1); /* 1 = default language index (English) */
	if (!ctx->ocr_ctx)
	{
		mprint("VOBSUB: Warning - OCR initialization failed\n");
		/* Continue anyway - OCR will just not work */
	}
#endif

	return ctx;
}

int vobsub_parse_palette(struct vobsub_ctx *ctx, const char *idx_header)
{
	if (!ctx || !idx_header)
		return -1;

	/* Find "palette:" line */
	const char *palette_line = strstr(idx_header, "palette:");
	if (!palette_line)
	{
		dbg_print(CCX_DMT_VERBOSE, "VOBSUB: No palette line found in idx header\n");
		return -1;
	}

	palette_line += 8; /* Skip "palette:" */

	/* Skip whitespace */
	while (*palette_line == ' ' || *palette_line == '\t')
		palette_line++;

	/* Parse 16 hex RGB colors */
	for (int i = 0; i < 16; i++)
	{
		unsigned int color;
		if (sscanf(palette_line, "%x", &color) != 1)
		{
			dbg_print(CCX_DMT_VERBOSE, "VOBSUB: Failed to parse palette color %d\n", i);
			break;
		}
		ctx->palette[i] = color;

		/* Skip to next color (past comma and whitespace) */
		while (*palette_line && *palette_line != ',' && *palette_line != '\n')
			palette_line++;
		if (*palette_line == ',')
			palette_line++;
		while (*palette_line == ' ' || *palette_line == '\t')
			palette_line++;
	}

	ctx->palette_parsed = 1;
	dbg_print(CCX_DMT_VERBOSE, "VOBSUB: Parsed palette from idx header\n");
	return 0;
}

int vobsub_decode_spu(struct vobsub_ctx *ctx,
		      unsigned char *spu_data, size_t spu_size,
		      long long start_time, long long end_time,
		      struct cc_subtitle *sub)
{
	if (!ctx || !spu_data || spu_size < 4 || !sub)
		return -1;

	/* Parse SPU header */
	uint16_t size_spu = (spu_data[0] << 8) | spu_data[1];
	uint16_t ctrl_offset = (spu_data[2] << 8) | spu_data[3];

	if (ctrl_offset > spu_size || size_spu > spu_size)
	{
		dbg_print(CCX_DMT_VERBOSE, "VOBSUB: Invalid SPU header (size=%u, ctrl=%u, buf=%zu)\n",
			  size_spu, ctrl_offset, spu_size);
		return -1;
	}

	/* Parse control sequence */
	vobsub_decode_control(ctx, spu_data, spu_size, ctrl_offset);

	/* Free any previous bitmap */
	if (ctx->bitmap)
	{
		free(ctx->bitmap);
		ctx->bitmap = NULL;
	}

	/* Decode bitmap */
	vobsub_get_bitmap(ctx, spu_data, spu_size);
	if (!ctx->bitmap)
	{
		dbg_print(CCX_DMT_VERBOSE, "VOBSUB: Failed to decode bitmap\n");
		return -1;
	}

	/* Build cc_subtitle structure */
	int w = (ctx->ctrl.coord[1] - ctx->ctrl.coord[0]) + 1;
	int h = (ctx->ctrl.coord[3] - ctx->ctrl.coord[2]) + 1;

	if (w <= 0 || h <= 0)
	{
		dbg_print(CCX_DMT_VERBOSE, "VOBSUB: Invalid bitmap dimensions\n");
		free(ctx->bitmap);
		ctx->bitmap = NULL;
		return -1;
	}

	sub->type = CC_BITMAP;
	sub->nb_data = 1;
	sub->got_output = 1;

	struct cc_bitmap *rect = malloc(sizeof(struct cc_bitmap));
	if (!rect)
	{
		free(ctx->bitmap);
		ctx->bitmap = NULL;
		return -1;
	}
	memset(rect, 0, sizeof(struct cc_bitmap));

	sub->data = rect;
	sub->datatype = CC_DATATYPE_GENERIC;
	sub->start_time = start_time;
	sub->end_time = end_time > 0 ? end_time : start_time + ctx->ctrl.stop_time;

	/* Copy bitmap data */
	rect->data0 = malloc(w * h);
	if (!rect->data0)
	{
		free(rect);
		sub->data = NULL;
		free(ctx->bitmap);
		ctx->bitmap = NULL;
		return -1;
	}
	memcpy(rect->data0, ctx->bitmap, w * h);

	/* Generate RGBA palette */
	rect->data1 = malloc(1024); /* Space for 256 colors */
	if (!rect->data1)
	{
		free(rect->data0);
		free(rect);
		sub->data = NULL;
		free(ctx->bitmap);
		ctx->bitmap = NULL;
		return -1;
	}
	memset(rect->data1, 0, 1024);
	vobsub_generate_rgba_palette(ctx, (uint32_t *)rect->data1);

	rect->nb_colors = 4;
	rect->x = ctx->ctrl.coord[0];
	rect->y = ctx->ctrl.coord[2];
	rect->w = w;
	rect->h = h;
	rect->linesize0 = w;

#ifdef ENABLE_OCR
	/* Run OCR if available */
	if (ctx->ocr_ctx)
	{
		char *ocr_str = NULL;
		int ret = ocr_rect(ctx->ocr_ctx, rect, &ocr_str, 0, 1); /* quantmode=1 */
		if (ret >= 0 && ocr_str)
		{
			rect->ocr_text = ocr_str;
		}
	}
#endif

	free(ctx->bitmap);
	ctx->bitmap = NULL;

	return 0;
}

int vobsub_ocr_available(void)
{
#ifdef ENABLE_OCR
	return 1;
#else
	return 0;
#endif
}

void delete_vobsub_decoder(struct vobsub_ctx **ctx)
{
	if (!ctx || !*ctx)
		return;

	struct vobsub_ctx *c = *ctx;

#ifdef ENABLE_OCR
	if (c->ocr_ctx)
		delete_ocr(&c->ocr_ctx);
#endif

	if (c->bitmap)
		free(c->bitmap);

	free(c);
	*ctx = NULL;
}
