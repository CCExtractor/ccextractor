/*
 * DVB subtitle decoding
 * Copyright (c) 2005 Ian Caulfield
 * Copyright (c) 2004 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2014 Anshul Maheshwari <er.anshul.maheshwari@gmail.com>
 * License: GPL 2.0
 *
 * You should have received a copy of the GNU General Public
 * License along with CCExtractor; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */
/**
 * @file dvbsub.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#include "dvb_subtitle_decoder.h"
#include "utility.h"
#include "ccx_decoders_common.h"
#include "ocr.h"
#include "dvb_dedup.h"
#include "ccx_common_option.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

// Debug stub for dump_rect_and_log - used in OCR debugging but not critical
static void dump_rect_and_log(const char *label, const uint8_t *data, int w, int h, int linesize, int mode, int pid, int idx)
{
	// Intentionally empty - enable for debugging bitmap data if needed
	(void)label;
	(void)data;
	(void)w;
	(void)h;
	(void)linesize;
	(void)mode;
	(void)pid;
	(void)idx;
}

#define DVBSUB_PAGE_SEGMENT 0x10
#define DVBSUB_REGION_SEGMENT 0x11
#define DVBSUB_CLUT_SEGMENT 0x12
#define DVBSUB_OBJECT_SEGMENT 0x13
#define DVBSUB_DISPLAYDEFINITION_SEGMENT 0x14
#define DVBSUB_DISPLAY_SEGMENT 0x80

// Maximum reasonable subtitle duration in milliseconds.
// DVB page timeouts can be very long (e.g., 65 seconds), but actual subtitles
// rarely exceed 10 seconds. This cap prevents corrupt timestamps from page timeouts.
#define DVB_MAX_SUBTITLE_DURATION_MS 10000

#define SCALEBITS 10
#define ONE_HALF (1 << (SCALEBITS - 1))
#define FIX(x) ((int)((x) * (1 << SCALEBITS) + 0.5))

#define YUV_TO_RGB1_CCIR(cb1, cr1)                                                               \
	{                                                                                        \
		cb = (cb1)-128;                                                                  \
		cr = (cr1)-128;                                                                  \
		r_add = FIX(1.40200 * 255.0 / 224.0) * cr + ONE_HALF;                            \
		g_add = -FIX(0.34414 * 255.0 / 224.0) * cb - FIX(0.71414 * 255.0 / 224.0) * cr + \
			ONE_HALF;                                                                \
		b_add = FIX(1.77200 * 255.0 / 224.0) * cb + ONE_HALF;                            \
	}

#define YUV_TO_RGB2_CCIR(r, g, b, y1)               \
	{                                           \
		y = ((y1)-16) * FIX(255.0 / 219.0); \
		r = cm[(y + r_add) >> SCALEBITS];   \
		g = cm[(y + g_add) >> SCALEBITS];   \
		b = cm[(y + b_add) >> SCALEBITS];   \
	}

#define times4(x) x, x, x, x
#define times256(x) times4(times4(times4(times4(times4(x)))))

#define MAX_NEG_CROP 1024
const uint8_t crop_tab[256 + 2 * MAX_NEG_CROP] = {times256(0x00), 0x00, 0x01,
						  0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
						  0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
						  0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
						  0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31,
						  0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,
						  0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
						  0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
						  0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61,
						  0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D,
						  0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
						  0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85,
						  0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91,
						  0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
						  0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9,
						  0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5,
						  0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1,
						  0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD,
						  0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9,
						  0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5,
						  0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1,
						  0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD,
						  0xFE, 0xFF, times256(0xFF)};

#define cm (crop_tab + MAX_NEG_CROP)

#define RGBA(r, g, b, a) (((unsigned)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))

typedef struct DVBSubCLUT
{
	int id;
	int version;

	uint32_t clut4[4];
	uint32_t clut16[16];
	uint32_t clut256[256];
	uint8_t ilut4[4];
	uint8_t ilut16[16];
	uint8_t ilut256[256];

	struct DVBSubCLUT *next;
} DVBSubCLUT;

static DVBSubCLUT default_clut;

typedef struct DVBSubObjectDisplay
{
	int object_id;
	int region_id;

	int x_pos;
	int y_pos;

	int fgcolor;
	int bgcolor;

	struct DVBSubObjectDisplay *region_list_next;
	struct DVBSubObjectDisplay *object_list_next;
} DVBSubObjectDisplay;

typedef struct DVBSubObject
{
	int id;
	int version;

	int type;

	DVBSubObjectDisplay *display_list;

	struct DVBSubObject *next;
} DVBSubObject;

typedef struct DVBSubRegionDisplay
{
	int region_id;

	int x_pos;
	int y_pos;

	struct DVBSubRegionDisplay *next;
} DVBSubRegionDisplay;

typedef struct DVBSubRegion
{
	int id;
	int version;

	int width;
	int height;
	int depth;

	int clut;
	int bgcolor;

	uint8_t *pbuf;
	int buf_size;
	int dirty;

	DVBSubObjectDisplay *display_list;

	struct DVBSubRegion *next;
} DVBSubRegion;

typedef struct DVBSubDisplayDefinition
{
	int version;

	int x;
	int y;
	int width;
	int height;
} DVBSubDisplayDefinition;

typedef struct DVBSubContext
{
	int composition_id;
	int ancillary_id;
	int lang_index;
	int version;
	/* Store time in ms */
	LLONG time_out;
	uint32_t prev_bitmap_hash;
#ifdef ENABLE_OCR
	void *ocr_ctx;
	int ocr_initialized; // Flag to track if OCR has been lazily initialized
#endif
	DVBSubRegion *region_list;
	DVBSubCLUT *clut_list;
	DVBSubObject *object_list;

	DVBSubRegionDisplay *display_list;
	DVBSubDisplayDefinition *display_definition;

	struct dvb_dedup_ring dedup_ring; // Deduplication ring buffer
} DVBSubContext;

size_t dvbsub_get_context_size(void)
{
	return sizeof(DVBSubContext);
}

void dvbsub_copy_context(void *dst, void *src)
{
	if (dst && src)
	{
		DVBSubContext *d = (DVBSubContext *)dst;
		DVBSubContext *s = (DVBSubContext *)src;

		// Copy scalar values only - DO NOT copy pointers to avoid aliasing
		// The linked lists (region_list, clut_list, object_list, display_list)
		// are owned by the source context and must not be shared
		d->composition_id = s->composition_id;
		d->ancillary_id = s->ancillary_id;
		d->lang_index = s->lang_index;
		d->version = s->version;
		d->time_out = s->time_out;

		// Initialize pointers to NULL to avoid use-after-free
		d->region_list = NULL;
		d->clut_list = NULL;
		d->object_list = NULL;
		d->display_list = NULL;
		d->display_definition = NULL;

#ifdef ENABLE_OCR
		// OCR context is shared, just copy the pointer (it's managed externally)
		d->ocr_ctx = s->ocr_ctx;
#endif
	}
}

static __inline unsigned int bytestream_get_byte(const uint8_t **b)
{
	(*b) += 1;
	return ((const uint8_t *)(*b - 1))[0];
}

static __inline unsigned int bytestream_get_be16(const uint8_t **b)
{
	(*b) += 2;
	return RB16(*b - 2);
}
typedef struct GetBitContext
{
	const uint8_t *buffer, *buffer_end;
	int index;
	int size_in_bits;
	int size_in_bits_plus8;
} GetBitContext;

/**
 * Initialize GetBitContext.
 * @param buffer bitstream buffer, must be FF_INPUT_BUFFER_PADDING_SIZE bytes
 *        larger than the actual read bits because some optimized bitstream
 *        readers read 32 or 64 bit at once and could read over the end
 * @param bit_size the size of the buffer in bits
 * @return 0 on success, AVERROR_INVALIDDATA if the buffer_size would overflow.
 */
static __inline int init_get_bits(GetBitContext *s, const uint8_t *buffer,
				  int bit_size)
{
	int buffer_size;
	int ret = 0;

	if (bit_size >= INT_MAX - 7 || bit_size < 0 || !buffer)
	{
		buffer_size = bit_size = 0;
		buffer = NULL;
		ret = -1;
		errno = EINVAL;
	}

	buffer_size = (bit_size + 7) >> 3;

	s->buffer = buffer;
	s->size_in_bits = bit_size;
	s->size_in_bits_plus8 = bit_size + 8;
	s->buffer_end = buffer + buffer_size;
	s->index = 0;

	return ret;
}

static __inline int get_bits_count(const GetBitContext *s)
{
	return s->index;
}

static __inline unsigned int get_bits(GetBitContext *s, int n)
{
	register int tmp;
	unsigned int re_index = s->index;
	unsigned int re_cache = 0;
	unsigned int re_size_plus8 = s->size_in_bits_plus8;

	if (n <= 0 || n > 25)
		return -1;
	re_cache = RB32(s->buffer + (re_index >> 3)) << (re_index & 7);

	tmp = ((uint32_t)re_cache) >> (32 - n);

	re_index = ((re_size_plus8 < re_index + (n)) ? (re_size_plus8) : (re_index + (n)));

	s->index = re_index;
	return tmp;
}
static __inline unsigned int get_bits1(GetBitContext *s)
{
	unsigned int index = s->index;
	uint8_t result = s->buffer[index >> 3];

	result <<= index & 7;
	result >>= 8 - 1;

	if (s->index < s->size_in_bits_plus8)
		index++;
	s->index = index;

	return result;
}

static DVBSubObject *get_object(DVBSubContext *ctx, int object_id)
{
	DVBSubObject *ptr = ctx->object_list;

	while (ptr && ptr->id != object_id)
		ptr = ptr->next;

	return ptr;
}

static DVBSubCLUT *get_clut(DVBSubContext *ctx, int clut_id)
{
	DVBSubCLUT *ptr = ctx->clut_list;

	while (ptr && ptr->id != clut_id)
		ptr = ptr->next;

	return ptr;
}

static DVBSubRegion *get_region(DVBSubContext *ctx, int region_id)
{
	DVBSubRegion *ptr = ctx->region_list;

	while (ptr && ptr->id != region_id)
		ptr = ptr->next;

	return ptr;
}

static void delete_region_display_list(DVBSubContext *ctx, DVBSubRegion *region)
{
	DVBSubObject *object, *obj2, **obj2_ptr;
	DVBSubObjectDisplay *display, *obj_disp, **obj_disp_ptr;

	while (region->display_list)
	{
		display = region->display_list;

		object = get_object(ctx, display->object_id);

		if (object)
		{
			obj_disp_ptr = &object->display_list;
			obj_disp = *obj_disp_ptr;

			while (obj_disp && obj_disp != display)
			{
				obj_disp_ptr = &obj_disp->object_list_next;
				obj_disp = *obj_disp_ptr;
			}

			if (obj_disp)
			{
				*obj_disp_ptr = obj_disp->object_list_next;

				if (!object->display_list)
				{
					obj2_ptr = &ctx->object_list;
					obj2 = *obj2_ptr;

					while (obj2 != object)
					{
						assert(obj2);
						obj2_ptr = &obj2->next;
						obj2 = *obj2_ptr;
					}

					*obj2_ptr = obj2->next;

					free(obj2);
				}
			}
		}

		region->display_list = display->region_list_next;

		free(display);
	}
}

static void delete_cluts(DVBSubContext *ctx)
{
	DVBSubCLUT *clut;

	while (ctx->clut_list)
	{
		clut = ctx->clut_list;

		ctx->clut_list = clut->next;

		free(clut);
	}
}

static void delete_objects(DVBSubContext *ctx)
{
	DVBSubObject *object;

	while (ctx->object_list)
	{
		object = ctx->object_list;

		ctx->object_list = object->next;

		free(object);
	}
}

static void delete_regions(DVBSubContext *ctx)
{
	DVBSubRegion *region;

	while (ctx->region_list)
	{
		region = ctx->region_list;

		ctx->region_list = region->next;

		delete_region_display_list(ctx, region);

		free(region->pbuf);
		free(region);
	}
}

/**
 * @param composition_id composition-page_id found in Subtitle descriptors
 *                       associated with     subtitle stream in the    PMT
 *                       it could be -1 if not found in PMT.
 * @param ancillary_id ancillary-page_id found in Subtitle descriptors
 *                     associated with     subtitle stream in the    PMT.
 *                       it could be -1 if not found in PMT.
 *
 * @return DVB context kept as void* for abstraction
 *
 */
void *dvbsub_init_decoder(struct dvb_config *cfg)
{
	int i, r, g, b, a = 0;
	DVBSubContext *ctx = (DVBSubContext *)malloc(sizeof(DVBSubContext));
	if (!ctx)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In dvbsub_init_decoder: Out of memory.");
	}
	memset(ctx, 0, sizeof(DVBSubContext));

	// Initialize deduplication ring buffer
	dvb_dedup_init(&ctx->dedup_ring);

	if (cfg)
	{
		ctx->composition_id = cfg->composition_id[0];
		ctx->ancillary_id = cfg->ancillary_id[0];
		ctx->lang_index = cfg->lang_index[0];
	}
	else
	{
		ctx->composition_id = 1;
		ctx->ancillary_id = 1;
		ctx->lang_index = 1;
	}

#ifdef ENABLE_OCR
	// Lazy OCR initialization: don't init here, wait until a bitmap actually needs OCR
	// This avoids ~10 second Tesseract startup overhead for files that have DVB streams
	// but don't actually produce any bitmap subtitles (e.g., files with CEA-608 captions)
	ctx->ocr_ctx = NULL;
	ctx->ocr_initialized = 0;
#endif
	ctx->version = -1;
	ctx->prev_bitmap_hash = 0;

	default_clut.id = -1;
	default_clut.next = NULL;

	default_clut.clut4[0] = RGBA(0, 0, 0, 0);
	default_clut.clut4[1] = RGBA(255, 255, 255, 255);
	default_clut.clut4[2] = RGBA(0, 0, 0, 255);
	default_clut.clut4[3] = RGBA(127, 127, 127, 255);
	default_clut.ilut4[0] = 0;
	default_clut.ilut4[1] = 255;
	default_clut.ilut4[2] = 0;
	default_clut.ilut4[3] = 127;

	default_clut.clut16[0] = RGBA(0, 0, 0, 0);
	default_clut.ilut16[0] = 0;
	for (i = 1; i < 16; i++)
	{
		if (i < 8)
		{
			r = (i & 1) ? 255 : 0;
			g = (i & 2) ? 255 : 0;
			b = (i & 4) ? 255 : 0;
		}
		else
		{
			r = (i & 1) ? 127 : 0;
			g = (i & 2) ? 127 : 0;
			b = (i & 4) ? 127 : 0;
		}
		default_clut.clut16[i] = RGBA(r, g, b, 255);
		default_clut.ilut16[i] = 0;
	}

	default_clut.clut256[0] = RGBA(0, 0, 0, 0);
	default_clut.ilut256[0] = 0;
	for (i = 1; i < 256; i++)
	{
		if (i < 8)
		{
			r = (i & 1) ? 255 : 0;
			g = (i & 2) ? 255 : 0;
			b = (i & 4) ? 255 : 0;
			a = 63;
		}
		else
		{
			switch (i & 0x88)
			{
				case 0x00:
					r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
					g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
					b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
					a = 255;
					break;
				case 0x08:
					r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
					g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
					b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
					a = 127;
					break;
				case 0x80:
					r = 127 + ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
					g = 127 + ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
					b = 127 + ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
					a = 255;
					break;
				case 0x88:
					r = ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
					g = ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
					b = ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
					a = 255;
					break;
			}
		}
		default_clut.ilut256[i] = 0;
		default_clut.clut256[i] = RGBA(r, g, b, a);
	}

	return (void *)ctx;
}
int dvbsub_close_decoder(void **dvb_ctx)
{
	DVBSubContext *ctx;
	DVBSubRegionDisplay *display;

	if (!dvb_ctx || !*dvb_ctx)
		return 0;

	ctx = (DVBSubContext *)*dvb_ctx;

	delete_regions(ctx);

	delete_objects(ctx);

	delete_cluts(ctx);

	freep(&ctx->display_definition);

	while (ctx->display_list)
	{
		display = ctx->display_list;
		ctx->display_list = display->next;
		free(display);
	}

#ifdef ENABLE_OCR
	if (ctx->ocr_ctx)
		delete_ocr(&ctx->ocr_ctx);
#endif
	freep(dvb_ctx);
	return 0;
}

static int dvbsub_read_2bit_string(uint8_t *destbuf, int dbuf_len,
				   const uint8_t **srcbuf, int buf_size, int non_mod, uint8_t *map_table,
				   int x_pos)
{
	GetBitContext gb;

	int bits;
	int run_length;
	int pixels_read = x_pos;

	init_get_bits(&gb, *srcbuf, buf_size << 3);

	destbuf += x_pos;

	while (get_bits_count(&gb) < buf_size << 3 && pixels_read < dbuf_len)
	{
		bits = get_bits(&gb, 2);

		if (bits)
		{
			if (non_mod != 1 || bits != 1)
			{
				if (map_table)
					*destbuf++ = map_table[bits];
				else
					*destbuf++ = bits;
			}
			pixels_read++;
		}
		else
		{
			bits = get_bits1(&gb);
			if (bits == 1)
			{
				run_length = get_bits(&gb, 3) + 3;
				bits = get_bits(&gb, 2);

				if (non_mod == 1 && bits == 1)
					pixels_read += run_length;
				else
				{
					if (map_table)
						bits = map_table[bits];
					while (run_length-- > 0 && pixels_read < dbuf_len)
					{
						*destbuf++ = bits;
						pixels_read++;
					}
				}
			}
			else
			{
				bits = get_bits1(&gb);
				if (bits == 0)
				{
					bits = get_bits(&gb, 2);
					if (bits == 2)
					{
						run_length = get_bits(&gb, 4) + 12;
						bits = get_bits(&gb, 2);

						if (non_mod == 1 && bits == 1)
							pixels_read += run_length;
						else
						{
							if (map_table)
								bits = map_table[bits];
							while (run_length-- > 0 && pixels_read < dbuf_len)
							{
								*destbuf++ = bits;
								pixels_read++;
							}
						}
					}
					else if (bits == 3)
					{
						run_length = get_bits(&gb, 8) + 29;
						bits = get_bits(&gb, 2);

						if (non_mod == 1 && bits == 1)
							pixels_read += run_length;
						else
						{
							if (map_table)
								bits = map_table[bits];
							while (run_length-- > 0 && pixels_read < dbuf_len)
							{
								*destbuf++ = bits;
								pixels_read++;
							}
						}
					}
					else if (bits == 1)
					{
						if (map_table)
							bits = map_table[0];
						else
							bits = 0;
						run_length = 2;
						while (run_length-- > 0 && pixels_read < dbuf_len)
						{
							*destbuf++ = bits;
							pixels_read++;
						}
					}
					else
					{
						(*srcbuf) += (get_bits_count(&gb) + 7) >> 3;
						return pixels_read;
					}
				}
				else
				{
					if (map_table)
						bits = map_table[0];
					else
						bits = 0;
					*destbuf++ = bits;
					pixels_read++;
				}
			}
		}
	}

	if (get_bits(&gb, 6))
	{
		mprint("DVBSub error: line overflow at dvbsub_read_2bit_string()\n");
		return -1;
	}

	(*srcbuf) += (get_bits_count(&gb) + 7) >> 3;

	return pixels_read;
}

static int dvbsub_read_4bit_string(uint8_t *destbuf, int dbuf_len,
				   const uint8_t **srcbuf, int buf_size, int non_mod, uint8_t *map_table,
				   int x_pos)
{
	GetBitContext gb;

	int bits;
	int run_length;
	int pixels_read = x_pos;

	init_get_bits(&gb, *srcbuf, buf_size << 3);

	destbuf += x_pos;

	while (get_bits_count(&gb) < buf_size << 3 && pixels_read < dbuf_len)
	{
		bits = get_bits(&gb, 4);

		if (bits)
		{
			if (non_mod != 1 || bits != 1)
			{
				if (map_table)
					*destbuf++ = map_table[bits];
				else
					*destbuf++ = bits;
			}
			pixels_read++;
		}
		else
		{
			bits = get_bits1(&gb);
			if (bits == 0)
			{
				run_length = get_bits(&gb, 3);

				if (run_length == 0)
				{
					(*srcbuf) += (get_bits_count(&gb) + 7) >> 3;
					return pixels_read;
				}

				run_length += 2;

				if (map_table)
					bits = map_table[0];
				else
					bits = 0;

				while (run_length-- > 0 && pixels_read < dbuf_len)
				{
					*destbuf++ = bits;
					pixels_read++;
				}
			}
			else
			{
				bits = get_bits1(&gb);
				if (bits == 0)
				{
					run_length = get_bits(&gb, 2) + 4;
					bits = get_bits(&gb, 4);

					if (non_mod == 1 && bits == 1)
						pixels_read += run_length;
					else
					{
						if (map_table)
							bits = map_table[bits];
						while (run_length-- > 0 && pixels_read < dbuf_len)
						{
							*destbuf++ = bits;
							pixels_read++;
						}
					}
				}
				else
				{
					bits = get_bits(&gb, 2);
					if (bits == 2)
					{
						run_length = get_bits(&gb, 4) + 9;
						bits = get_bits(&gb, 4);

						if (non_mod == 1 && bits == 1)
							pixels_read += run_length;
						else
						{
							if (map_table)
								bits = map_table[bits];
							while (run_length-- > 0 && pixels_read < dbuf_len)
							{
								*destbuf++ = bits;
								pixels_read++;
							}
						}
					}
					else if (bits == 3)
					{
						run_length = get_bits(&gb, 8) + 25;
						bits = get_bits(&gb, 4);

						if (non_mod == 1 && bits == 1)
							pixels_read += run_length;
						else
						{
							if (map_table)
								bits = map_table[bits];
							while (run_length-- > 0 && pixels_read < dbuf_len)
							{
								*destbuf++ = bits;
								pixels_read++;
							}
						}
					}
					else if (bits == 1)
					{
						if (map_table)
							bits = map_table[0];
						else
							bits = 0;
						run_length = 2;
						while (run_length-- > 0 && pixels_read < dbuf_len)
						{
							*destbuf++ = bits;
							pixels_read++;
						}
					}
					else
					{
						if (map_table)
							bits = map_table[0];
						else
							bits = 0;
						*destbuf++ = bits;
						pixels_read++;
					}
				}
			}
		}
	}

	if (get_bits(&gb, 8))
	{
		mprint("DVBSub error: line overflow at dvbsub_read_4bit_string()\n");
		return -1;
	}

	(*srcbuf) += (get_bits_count(&gb) + 7) >> 3;

	return pixels_read;
}

static int dvbsub_read_8bit_string(uint8_t *destbuf, int dbuf_len,
				   const uint8_t **srcbuf, int buf_size, int non_mod, uint8_t *map_table,
				   int x_pos)
{
	const uint8_t *sbuf_end = (*srcbuf) + buf_size;
	int bits;
	int run_length;
	int pixels_read = x_pos;

	destbuf += x_pos;

	while (*srcbuf < sbuf_end && pixels_read < dbuf_len)
	{
		bits = *(*srcbuf)++;

		if (bits)
		{
			if (non_mod != 1 || bits != 1)
			{
				if (map_table)
					*destbuf++ = map_table[bits];
				else
					*destbuf++ = bits;
			}
			pixels_read++;
		}
		else
		{
			bits = *(*srcbuf)++;
			run_length = bits & 0x7f;
			if ((bits & 0x80) == 0)
			{
				if (run_length == 0)
				{
					return pixels_read;
				}

				if (map_table)
					bits = map_table[0];
				else
					bits = 0;
				while (run_length-- > 0 && pixels_read < dbuf_len)
				{
					*destbuf++ = bits;
					pixels_read++;
				}
			}
			else
			{
				bits = *(*srcbuf)++;

				if (non_mod == 1 && bits == 1)
					pixels_read += run_length;
				if (map_table)
					bits = map_table[bits];
				else
					while (run_length-- > 0 && pixels_read < dbuf_len)
					{
						*destbuf++ = bits;
						pixels_read++;
					}
			}
		}
	}

	if (*(*srcbuf)++)
	{
		mprint("DVBSub error: line overflow at dvbsub_read_8bit_string()\n");
		return -1;
	}

	return pixels_read;
}

static int dvbsub_parse_pixel_data_block(void *dvb_ctx,
					 DVBSubObjectDisplay *display, const uint8_t *buf, int buf_size,
					 int top_bottom, int non_mod)
{
	DVBSubContext *ctx = (DVBSubContext *)dvb_ctx;

	DVBSubRegion *region = get_region(ctx, display->region_id);
	const uint8_t *buf_end = buf + buf_size;
	uint8_t *pbuf;
	int x_pos, y_pos;
	int i;
	int parseerror = 0;

	uint8_t map2to4[] = {0x0, 0x7, 0x8, 0xf};
	uint8_t map2to8[] = {0x00, 0x77, 0x88, 0xff};
	uint8_t map4to8[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
			     0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	uint8_t *map_table;

	if (region == 0)
		return 0;

	pbuf = region->pbuf;
	region->dirty = 1;

	x_pos = display->x_pos;
	y_pos = display->y_pos;

	y_pos += top_bottom;

	while (!parseerror && buf < buf_end)
	{
		if ((*buf != 0xf0 && x_pos >= region->width) || y_pos >= region->height)
		{
			mprint("In dvbsub_parse_pixel_data_block(): Invalid object location. %d-%d %d-%d %02x\n", x_pos,
			       region->width, y_pos, region->height, *buf);
			parseerror = 1;
			break;
		}

		switch (*buf++)
		{
			case 0x10:
				if (region->depth == 8)
					map_table = map2to8;
				else if (region->depth == 4)
					map_table = map2to4;
				else
					map_table = NULL;

				x_pos = dvbsub_read_2bit_string(pbuf + (y_pos * region->width),
								region->width, &buf, buf_end - buf, non_mod, map_table,
								x_pos);
				if (x_pos < 0)
				{
					mprint("In dvbsub_parse_pixel_data_block(): dvbsub_read_2bit_string() returned error.\n");
					parseerror = 1;
					goto exitfunc;
				}
				break;
			case 0x11:
				if (region->depth < 4)
				{
					mprint("In dvbsub_parse_pixel_data_block(): 4-bit pixel string in %d-bit region.\n", region->depth);
					parseerror = 1;
					goto exitfunc;
				}

				if (region->depth == 8)
					map_table = map4to8;
				else
					map_table = NULL;

				x_pos = dvbsub_read_4bit_string(pbuf + (y_pos * region->width),
								region->width, &buf, buf_end - buf, non_mod, map_table,
								x_pos);
				if (x_pos < 0)
				{
					mprint("In dvbsub_parse_pixel_data_block(): dvbsub_read_4bit_string() returned error.\n");
					parseerror = 1;
					goto exitfunc;
				}
				break;
			case 0x12:
				if (region->depth < 8)
				{
					mprint("In dvbsub_parse_pixel_data_block(): 8-bit pixel string in %d-bit region.\n", region->depth);
					return -1;
				}

				x_pos = dvbsub_read_8bit_string(pbuf + (y_pos * region->width),
								region->width, &buf, buf_end - buf, non_mod, NULL, x_pos);
				if (x_pos < 0)
				{
					mprint("In dvbsub_parse_pixel_data_block(): dvbsub_read_8bit_string() returned error.\n");
					parseerror = 1;
					goto exitfunc;
				}
				break;

			case 0x20:
				map2to4[0] = (*buf) >> 4;
				map2to4[1] = (*buf++) & 0xf;
				map2to4[2] = (*buf) >> 4;
				map2to4[3] = (*buf++) & 0xf;
				break;
			case 0x21:
				for (i = 0; i < 4; i++)
					map2to8[i] = *buf++;
				break;
			case 0x22:
				for (i = 0; i < 16; i++)
					map4to8[i] = *buf++;
				break;

			case 0xf0:
				x_pos = display->x_pos;
				y_pos += 2;
				break;
			default:
				mprint("In dvbsub_parse_pixel_data_block(): Unknown/unsupported pixel block 0x%x\n", *(buf - 1));
				/* no break */
		}
	}
exitfunc:
	return parseerror ? -1 : 0;
}

static int dvbsub_parse_object_segment(void *dvb_ctx, const uint8_t *buf,
				       int buf_size)
{
	DVBSubContext *ctx = (DVBSubContext *)dvb_ctx;

	const uint8_t *buf_end = buf + buf_size;
	int object_id;
	DVBSubObject *object;
	DVBSubObjectDisplay *display;
	int top_field_len, bottom_field_len;

	int coding_method, non_modifying_color;

	object_id = RB16(buf);
	buf += 2;

	object = get_object(ctx, object_id);

	if (!object)
		return 0;

	int version = ((*buf) >> 4) & 15;
	coding_method = ((*buf) >> 2) & 3;
	non_modifying_color = ((*buf++) >> 1) & 1;

	if (object->version == version)
		return 0;

	object->version = version;

	if (coding_method == 0)
	{
		top_field_len = RB16(buf);
		buf += 2;
		bottom_field_len = RB16(buf);
		buf += 2;
		if (buf + top_field_len + bottom_field_len > buf_end)
		{
			mprint("dvbsub_parse_object_segment(): Field data size too large\n");
			return -1;
		}

		for (display = object->display_list; display;
		     display = display->object_list_next)
		{
			const uint8_t *block = buf;
			int bfl = bottom_field_len;

			if (dvbsub_parse_pixel_data_block(dvb_ctx, display, block,
							  top_field_len, 0, non_modifying_color))
			{
				mprint("dvbsub_parse_object_segment(): Something went wrong. Giving up on block (1).\n");
				// Something went wrong, get out and hope we can
				// recover
				return -1;
			}

			if (bottom_field_len > 0)
				block = buf + top_field_len;
			else
				bfl = top_field_len;

			if (dvbsub_parse_pixel_data_block(dvb_ctx, display, block, bfl, 1,
							  non_modifying_color))
			{
				mprint("dvbsub_parse_object_segment(): Something went wrong. Giving up on block (2).\n");
				return -1;
			}
		}
	}
	else if (coding_method == 1)
	{
		mprint("FIXME support for string coding standard\n");
	}
	else
	{
		mprint("Unknown object coding %d\n", coding_method);
	}
	return 0;
}

static int dvbsub_parse_clut_segment(void *dvb_ctx, const uint8_t *buf,
				     int buf_size)
{
	DVBSubContext *ctx = (DVBSubContext *)dvb_ctx;

	const uint8_t *buf_end = buf + buf_size;
	int clut_id;
	int version;
	DVBSubCLUT *clut;
	int entry_id, depth, full_range;
	int y, cr, cb, alpha;
	int r, g, b, r_add, g_add, b_add;

	clut_id = *buf++;
	version = ((*buf) >> 4) & 15;
	buf += 1;

	clut = get_clut(ctx, clut_id);

	if (!clut)
	{
		clut = (DVBSubCLUT *)malloc(sizeof(DVBSubCLUT));
		if (!clut)
		{
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In dvbsub_parse_clut_segment: Out of memory.");
		}

		memcpy(clut, &default_clut, sizeof(DVBSubCLUT));

		clut->id = clut_id;
		clut->version = -1;

		clut->next = ctx->clut_list;
		ctx->clut_list = clut;
	}

	if (clut->version != version)
	{

		clut->version = version;

		while (buf + 4 < buf_end)
		{
			entry_id = *buf++;

			depth = (*buf) & 0xe0;

			if (depth == 0)
			{
				mprint("dvbsub_parse_clut_segment(): Invalid clut depth 0x%x.\n", *buf);
				return -1;
			}

			full_range = (*buf++) & 1;

			if (full_range)
			{
				y = *buf++;
				cr = *buf++;
				cb = *buf++;
				alpha = *buf++;
			}
			else
			{
				y = buf[0] & 0xfc;
				cr = (((buf[0] & 3) << 2) | ((buf[1] >> 6) & 3)) << 4;
				cb = (buf[1] << 2) & 0xf0;
				alpha = (buf[1] << 6) & 0xc0;

				buf += 2;
			}

			if (y == 0)
				alpha = 0xff;

			YUV_TO_RGB1_CCIR(cb, cr);
			YUV_TO_RGB2_CCIR(r, g, b, y);

			if (!!(depth & 0x80) + !!(depth & 0x40) + !!(depth & 0x20) > 1)
			{
				mprint("dvbsub_parse_clut_segment(): More than one bit level marked: %x\n", depth);
			}

			if (depth & 0x80)
			{
				clut->clut4[entry_id] = RGBA(r, g, b, 255 - alpha);
				clut->ilut4[entry_id] = y;
			}
			else if (depth & 0x40)
			{
				clut->clut16[entry_id] = RGBA(r, g, b, 255 - alpha);
				clut->ilut16[entry_id] = y;
			}
			else if (depth & 0x20)
			{
				clut->clut256[entry_id] = RGBA(r, g, b, 255 - alpha);
				clut->ilut256[entry_id] = y;
			}
		}
	}
	return 0;
}

static void dvbsub_parse_region_segment(void *dvb_ctx, const uint8_t *buf,
					int buf_size)
{
	DVBSubContext *ctx = (DVBSubContext *)dvb_ctx;

	const uint8_t *buf_end = buf + buf_size;
	int region_id, object_id;
	int version;
	DVBSubRegion *region;
	DVBSubObject *object;
	DVBSubObjectDisplay *display;
	int fill;

	if (buf_size < 10)
	{
		mprint(" dvbsub_parse_region_segment(): [buf_size < 10, leaving region] ");
		return;
	}

	region_id = *buf++;
	version = ((*buf) >> 4) & 15;

	region = get_region(ctx, region_id);

	if (!region)
	{
		dbg_print(CCX_DMT_DVB, " [new region allocated] ");
		region = (struct DVBSubRegion *)malloc(sizeof(struct DVBSubRegion));
		if (!region)
		{
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In dvbsub_parse_region_segment: Out of memory allocating region.");
		}
		memset(region, 0, sizeof(struct DVBSubRegion));

		region->id = region_id;
		region->version = version;

		region->next = ctx->region_list;
		ctx->region_list = region;
	}
	else if (version == region->version)
	{
		dbg_print(CCX_DMT_DVB, " [already had this region and version] ");
		return;
	}
	fill = ((*buf++) >> 3) & 1;

	region->width = RB16(buf);
	buf += 2;
	region->height = RB16(buf);
	buf += 2;

	dbg_print(CCX_DMT_DVB, ", REGION %d WIDTH: %d, ", region_id, region->width);
	dbg_print(CCX_DMT_DVB, "REGION %d HEIGHT: %d", region_id, region->height);

	if (region->width * region->height != region->buf_size)
	{
		freep(&region->pbuf);
		region->buf_size = region->width * region->height;
		region->pbuf = (uint8_t *)malloc(region->buf_size);
		if (!region->pbuf)
		{
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In dvbsub_parse_region_segment: Out of memory allocating pbuf.");
		}
		fill = 1;
		region->dirty = 0;
	}

	region->depth = 1 << (((*buf++) >> 2) & 7);
	if (region->depth < 2 || region->depth > 8)
	{
		mprint("dvbsub_parse_region_segment(): region depth %d is invalid\n", region->depth);
		region->depth = 4;
	}
	region->clut = *buf++;

	if (region->depth == 8)
	{
		region->bgcolor = *buf++;
		buf += 1;
	}
	else
	{
		buf += 1;
		if (region->depth == 4)
			region->bgcolor = (((*buf++) >> 4) & 15);
		else
			region->bgcolor = (((*buf++) >> 2) & 3);
	}

	if (fill)
		memset(region->pbuf, region->bgcolor, region->buf_size);

	delete_region_display_list(ctx, region);

	while (buf + 5 < buf_end)
	{
		object_id = RB16(buf);
		buf += 2;

		object = get_object(ctx, object_id);

		if (!object)
		{
			object = (struct DVBSubObject *)malloc(sizeof(struct DVBSubObject));
			if (!object)
			{
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In dvbsub_parse_region_segment: Out of memory allocating object.");
			}
			memset(object, 0, sizeof(struct DVBSubObject));
			object->version = -1;

			object->id = object_id;
			object->next = ctx->object_list;
			ctx->object_list = object;
		}

		object->type = (*buf) >> 6;

		display = (struct DVBSubObjectDisplay *)malloc(
		    sizeof(struct DVBSubObjectDisplay));
		if (!display)
		{
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In dvbsub_parse_region_segment: Out of memory allocating display.");
		}
		memset(display, 0, sizeof(struct DVBSubObjectDisplay));

		display->object_id = object_id;
		display->region_id = region_id;

		display->x_pos = RB16(buf) & 0xfff;
		buf += 2;
		display->y_pos = RB16(buf) & 0xfff;
		buf += 2;

		if ((object->type == 1 || object->type == 2) && buf + 1 < buf_end)
		{
			display->fgcolor = *buf++;
			display->bgcolor = *buf++;
		}

		display->region_list_next = region->display_list;
		region->display_list = display;

		display->object_list_next = object->display_list;
		object->display_list = display;
	}
	assert(buf <= buf_end);
}
/*
 * xxx loose last frame
 */
static void dvbsub_parse_page_segment(void *dvb_ctx, const uint8_t *buf,
				      int buf_size)
{
	DVBSubContext *ctx = (DVBSubContext *)dvb_ctx;
	DVBSubRegionDisplay *display;
	DVBSubRegionDisplay *tmp_display_list, **tmp_ptr;

	const uint8_t *buf_end = buf + buf_size;
	int region_id;
	int page_state;
	int timeout;
	int version;
	int has_region_definitions; // Declaration moved here for C89 compliance

	if (buf_size < 2)
		return;

	timeout = *buf++;
	version = ((*buf) >> 4) & 15;
	page_state = ((*buf++) >> 2) & 3;

	// Version check removed to always allow page state check (Fix for Arte stream)

	/* Convert time from second to ms */
	ctx->time_out = timeout * 1000;
	ctx->version = version;

	//
	// Issue 5: Spec-compliant Page Segment handling
	// KEY FIX: Only rebuild display_list if new regions are defined
	has_region_definitions = (buf + 6 <= buf_end); // Need at least 6 bytes for one region

	if (page_state == 1 || page_state == 2)
	{
		// Mode change (1) or Acquisition point (2): Always clear display list
		delete_regions(ctx);
		delete_objects(ctx);
		delete_cluts(ctx);

		tmp_display_list = ctx->display_list;
		ctx->display_list = NULL;

		// If regions are provided, parsing loop below will rebuild ctx->display_list
		// If no regions provided, ctx->display_list remains NULL (empty)
	}
	else if (has_region_definitions)
	{
		// Normal case (0) WITH new region definitions: clear and rebuild
		tmp_display_list = ctx->display_list;
		ctx->display_list = NULL;
	}
	else
	{
		// Normal case (0) WITHOUT region definitions: keep existing display_list (Arte fix)
		// Do not clear ctx->display_list
		tmp_display_list = NULL; // Nothing to free from old list
	}

	// Rebuild display list if we have regions to parse
	if (has_region_definitions)
	{
		while (buf + 6 <= buf_end)
		{
			region_id = *buf++;
			buf += 1;
			display = tmp_display_list;
			tmp_ptr = &tmp_display_list;

			while (display && display->region_id != region_id)
			{
				tmp_ptr = &display->next;
				display = display->next;
			}

			if (!display)
			{
				display = (struct DVBSubRegionDisplay *)malloc(
				    sizeof(struct DVBSubRegionDisplay));
				if (!display)
				{
					fatal(EXIT_NOT_ENOUGH_MEMORY, "In dvbsub_parse_page_segment: Out of memory allocating display.");
				}
				memset(display, 0, sizeof(struct DVBSubRegionDisplay));
			}

			display->region_id = region_id;

			display->x_pos = RB16(buf);
			buf += 2;
			display->y_pos = RB16(buf);
			buf += 2;

			*tmp_ptr = display->next;

			display->next = ctx->display_list;
			ctx->display_list = display;
		}
	}

	// Free any leftover regions that weren't reused
	while (tmp_display_list)
	{
		display = tmp_display_list;
		tmp_display_list = display->next;
		free(display);
	}

	assert(buf <= buf_end);
}

static void dvbsub_parse_display_definition_segment(void *dvb_ctx,
						    const uint8_t *buf, int buf_size)
{
	DVBSubContext *ctx = (DVBSubContext *)dvb_ctx;
	DVBSubDisplayDefinition *display_def = ctx->display_definition;
	int dds_version, info_byte;

	if (buf_size < 5)
		return;

	info_byte = bytestream_get_byte(&buf);
	dds_version = info_byte >> 4;
	if (display_def && display_def->version == dds_version)
		return; // already have this display definition version

	if (!display_def)
	{
		display_def = (struct DVBSubDisplayDefinition *)malloc(
		    sizeof(*display_def));
		if (!display_def)
			return;
		memset(display_def, 0, sizeof(*display_def));
		ctx->display_definition = display_def;
	}

	display_def->version = dds_version;
	display_def->x = 0;
	display_def->y = 0;
	display_def->width = bytestream_get_be16(&buf) + 1;
	display_def->height = bytestream_get_be16(&buf) + 1;

	if (buf_size < 13)
		return;

	if (info_byte & 1 << 3)
	{ // display_window_flag
		display_def->x = bytestream_get_be16(&buf);
		display_def->width = bytestream_get_be16(&buf) - display_def->x + 1;
		display_def->y = bytestream_get_be16(&buf);
		display_def->height = bytestream_get_be16(&buf) - display_def->y + 1;
	}
}

static uint32_t fnv1a_32(const uint8_t *data, size_t len)
{
	uint32_t hash = 2166136261u;
	for (size_t i = 0; i < len; i++)
	{
		hash ^= data[i];
		hash *= 16777619u;
	}
	return hash;
}

/**
 * Write Subtitle in cc_subtitle structure in CC_BITMAP format
 * when OCR subsystem is present then it also write recognised text in
 * cc_bitmap ocr_text variable.
 */
static int write_dvb_sub(struct lib_cc_decode *dec_ctx, struct cc_subtitle *sub)
{
	DVBSubContext *ctx;
	DVBSubRegion *region;
	DVBSubRegionDisplay *display;
	DVBSubCLUT *clut;
	DVBSubDisplayDefinition *display_def;
	struct cc_bitmap *rect = NULL;
	uint32_t *clut_table;
	int offset_x = 0, offset_y = 0;
	int x_pos = -1, y_pos = -1, width = 0, height = 0;

	ctx = (DVBSubContext *)dec_ctx->private_data;

	// Safety check: Context may be NULL after PAT change
	if (!ctx)
		return -1;

	// Validate we have something to display
	if (!ctx->display_list)
	{
		if (ctx->region_list)
		{
			// Heuristic Fix for Arte stream: Valid regions exist but Page Segment was empty.
			// We auto-populate the display list assuming (0,0) coordinates.
			DVBSubRegion *r = ctx->region_list;
			while (r)
			{
				// Only add regions with actual pixel data (non-empty pbuf)
				int has_content = 0;
				if (r->pbuf && r->buf_size > 0)
				{
					for (int i = 0; i < r->buf_size; i++)
					{
						if (r->pbuf[i] != 0)
						{
							has_content = 1;
							break;
						}
					}
				}

				if (has_content)
				{
					DVBSubRegionDisplay *d = (DVBSubRegionDisplay *)malloc(sizeof(struct DVBSubRegionDisplay));
					if (d)
					{
						memset(d, 0, sizeof(*d));
						d->region_id = r->id;
						d->x_pos = 0;
						d->y_pos = 0;
						d->next = ctx->display_list;
						ctx->display_list = d;
						// Force dirty so this region gets rendered
						// r->dirty = 1;
					}
				}
				else
				{
				}
				r = r->next;
			}
		}
		else
		{
			return 0;
		}
	}

	display_def = ctx->display_definition;
	sub->type = CC_BITMAP;
	sub->lang_index = ctx->lang_index;

	if (display_def)
	{
		offset_x = display_def->x;
		offset_y = display_def->y;
	}

	// Initialize nb_data before counting dirty regions to avoid stale values
	// from copy_subtitle() carrying over between calls
	sub->nb_data = 0;

	for (display = ctx->display_list; display; display = display->next)
	{
		region = get_region(ctx, display->region_id);
		if (region && region->dirty)
		{
			sub->nb_data++;
		}
	}
	if (sub->nb_data <= 0)
	{
		return 0;
	}

	rect = malloc(sizeof(struct cc_bitmap));
	if (!rect)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In write_dvb_sub: Out of memory allocating rect.");
	}
	rect->data0 = NULL;
	rect->data1 = NULL;

	// The first loop, to determine the size of the whole subtitle (made up of different display/regions)

	for (display = ctx->display_list; display; display = display->next)
	{
		region = get_region(ctx, display->region_id);

		if (!region)
			continue;

		if (!region->dirty)
			continue;

		if (x_pos == -1)
		{
			x_pos = display->x_pos;
			y_pos = display->y_pos;
			width = region->width;
			height = region->height;
		}
		else
		{
			if (x_pos > display->x_pos)
			{
				width += (x_pos - display->x_pos);
				x_pos = display->x_pos;
			}

			if (display->y_pos < y_pos)
			{
				height += (y_pos - display->y_pos);
				y_pos = display->y_pos;
			}

			if (display->x_pos + region->width > x_pos + width)
			{
				width = display->x_pos + region->width - x_pos;
			}

			if (display->y_pos + region->height > y_pos + height)
			{
				height = display->y_pos + region->height - y_pos;
			}
		}
		rect->nb_colors = (1 << region->depth);

		// Process CLUT
		clut = get_clut(ctx, region->clut);

		if (!clut)
			clut = &default_clut;

		switch (region->depth)
		{
			case 2:
				clut_table = clut->clut4;
				break;
			case 8:
				clut_table = clut->clut256;
				break;
			case 4:
			default:
				clut_table = clut->clut16;
				break;
		}

		if (rect->data1 == NULL)
		{
			rect->data1 = malloc(1024);
			if (!rect->data1)
			{
				free(rect);
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In write_dvb_sub: Out of memory allocating data1.");
			}
		}
		memset(rect->data1, 0, 1024);
		memcpy(rect->data1, clut_table, (1 << region->depth) * sizeof(uint32_t));
		rect->nb_colors = (1 << region->depth); // CRITICAL FIX: OCR needs this to know palette size

		// User Quick Test
		assert(((1 << region->depth) * sizeof(uint32_t)) <= 1024);
	}

	rect->x = x_pos + offset_x;
	rect->y = y_pos + offset_y;
	rect->w = width;
	rect->h = height;
	rect->linesize0 = width;

	// The second loop, to generate the merged image

	dbg_print(CCX_DMT_DVB, "\nCreating a data[0] of %d bytes (%d x %d)\n", width * height, width, height);
	rect->data0 = (uint8_t *)malloc(width * height);
	if (!rect->data0)
	{
		free(rect->data1);
		free(rect);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In write_dvb_sub: Out of memory allocating data0 (%d * %d = %d bytes).", width, height, width * height);
	}
	memset(rect->data0, 0x0, width * height);

	for (display = ctx->display_list; display; display = display->next)
	{
		region = get_region(ctx, display->region_id);
		if (!region)
			continue;

		int x_off = display->x_pos - x_pos;
		int y_off = display->y_pos - y_pos;
		static int oob_warning_printed = 0;
		int oob_count = 0;
		for (int y = 0; y < region->height; y++)
		{
			for (int x = 0; x < region->width; x++)
			{
				int offset = ((y + y_off) * width) + x_off + x;
				if (offset >= (width * height) || offset < 0)
				{
					oob_count++;
					if (!oob_warning_printed)
					{
						mprint("write_dvb_sub(): Out of bounds pixels detected (showing first occurrence only)\n");
						mprint("  Formula: offset=((y + y_off) * width) + x_off + x\n");
						mprint("  y=%d, y_off=%d, width=%d, x_off=%d, x=%d, offset=%d\n",
						       y, y_off, width, x_off, x, offset);
						oob_warning_printed = 1;
					}
				}
				else
				{
					uint8_t c = (uint8_t)region->pbuf[y * region->width + x];
					rect->data0[offset] = c;
				}
			}
		}
		region->dirty = 0;
	}

	if (rect->data0 && rect->data1)
	{
		if (!region)
		{
			free(rect->data0);
			free(rect->data1);
			free(rect);
			sub->nb_data = 0;
			sub->got_output = 0;
			return 0;
		}
		uint32_t current_hash = 2166136261u;

		// Hash the geometry
		current_hash ^= (uint32_t)rect->x;
		current_hash *= 16777619u;
		current_hash ^= (uint32_t)rect->y;
		current_hash *= 16777619u;
		current_hash ^= (uint32_t)rect->w;
		current_hash *= 16777619u;
		current_hash ^= (uint32_t)rect->h;
		current_hash *= 16777619u;

		// Hash the pixels
		current_hash ^= fnv1a_32(rect->data0, width * height);
		current_hash *= 16777619u;

		// Hash the palette (color table)
		// We use the last region's depth/nb_colors as that's what determined the palette size
		current_hash ^= fnv1a_32(rect->data1, (1 << region->depth) * sizeof(uint32_t));
		current_hash *= 16777619u;

		if (ctx->prev_bitmap_hash == current_hash)
		{
			dbg_print(CCX_DMT_DVB, "Duplicate DVB subtitle frame detected (Hash: %08X). Skipping.\n", current_hash);
			// Free the rect we just allocated since we aren't using it
			free(rect->data0);
			free(rect->data1);
			free(rect);
			sub->nb_data = 0;    // Ensure nb_data is 0 so the encoder doesn't try to access null data
			sub->got_output = 0; // CRITICAL: Mark as no output to prevent duplicate encoding
			return 0;	     // Return 0 to indicate no new subtitle produced
		}

		ctx->prev_bitmap_hash = current_hash;
	}

	sub->flags |= SUB_EOD_MARKER;
	sub->got_output = 1;
	sub->data = rect;
	sub->datatype = CC_DATATYPE_DVB;

	// DEBUG: Verify nonzero count manually
	int nz_count = 0;
	for (int k = 0; k < width * height; k++)
		if (rect->data0[k])
			nz_count++;
	sub->nb_data = 1; // Set nb_data to 1 since we have merged the images into one image.

	// Perform OCR
#ifdef ENABLE_OCR
	char *ocr_str = NULL;
	// Lazy OCR initialization: only init when we actually have a bitmap to process
	if (!ctx->ocr_initialized)
	{
		ctx->ocr_ctx = init_ocr(ctx->lang_index);
		ctx->ocr_initialized = 1; // Mark as initialized even if init_ocr returns NULL
	}
	if (ctx->ocr_ctx && region)
	{
		// DEBUG: Dump before OCR
		// dump_rect_and_log("before_ocr", rect->data0, rect->w, rect->h, rect->linesize0, 1, 0, 0);

		int ret = ocr_rect(ctx->ocr_ctx, rect, &ocr_str, region->bgcolor, dec_ctx->ocr_quantmode);
		if (ret >= 0 && ocr_str)
		{
			rect->ocr_text = ocr_str;
		}
		else
		{
			rect->ocr_text = NULL;
		}

		// DEBUG: Dump after OCR (if modified)
		// dump_rect_and_log("after_ocr", rect->data0, rect->w, rect->h, rect->linesize0, ctx->display_definition ? 3 : 1, 0, 0);
	}
	else
	{
		rect->ocr_text = NULL;
	}
#endif

	return 0;
}

void dvbsub_handle_display_segment(struct encoder_ctx *enc_ctx,
				   struct lib_cc_decode *dec_ctx,
				   struct cc_subtitle *sub,
				   LLONG pre_fts_max)
{
	if (!enc_ctx || !dec_ctx || !dec_ctx->timing)
		return;

	DVBSubContext *ctx = (DVBSubContext *)dec_ctx->private_data;
	if (!ctx)
		return;

	LLONG current_pts = dec_ctx->timing->current_pts;
	// Deduplication check: Skip if this subtitle is a duplicate
	// We use composition_id + ancillary_id + PTS to uniquely identify a subtitle
	// PTS is converted from microseconds to milliseconds for consistency
	// Skip dedup if --no-dvb-dedup flag is set
	if (!ccx_options.no_dvb_dedup)
	{
		uint64_t pts_ms = (uint64_t)(current_pts / 1000);
		uint32_t pid = (uint32_t)(dec_ctx->pid ? dec_ctx->pid : dec_ctx->program_number);

		if (dvb_dedup_is_duplicate(&ctx->dedup_ring, pts_ms, pid,
					   (uint16_t)ctx->composition_id,
					   (uint16_t)ctx->ancillary_id))
		{
			dbg_print(CCX_DMT_DVB, "DVB: Skipping duplicate subtitle (PTS=%lld, comp_id=%d, anc_id=%d)\n",
				  current_pts, ctx->composition_id, ctx->ancillary_id);
			return;
		}

		// Add to dedup ring buffer
		dvb_dedup_add(&ctx->dedup_ring, pts_ms, pid,
			      (uint16_t)ctx->composition_id,
			      (uint16_t)ctx->ancillary_id);
	}

	if (enc_ctx->write_previous) // this condition is used for the first subtitle - write_previous will be 0 first so we don't encode a non-existing previous sub
	{
		enc_ctx->prev->last_str = NULL; // Reset last recognized sub text
		// Validate current_field before calling get_fts (valid: 1=field1, 2=field2, 3=CEA-708)
		int caption_field = dec_ctx->current_field;
		if (caption_field < 1 || caption_field > 3)
		{
			dbg_print(CCX_DMT_DVB, "DVB: invalid current_field %d, using default 1\n", caption_field);
			caption_field = 1;
		}
		// Get the current FTS, which will be the start_time of the new subtitle
		LLONG next_start_time = get_fts(dec_ctx->timing, caption_field);

		if (!sub->prev)
		{
			// Previous subtitle is missing or invalid, skipping write_previous
			enc_ctx->write_previous = 0;
			if (enc_ctx->prev)
			{
				free_encoder_context(enc_ctx->prev);
				enc_ctx->prev = NULL;
				enc_ctx->prev = copy_encoder_context(enc_ctx);
			}
		}
		else
		{

			// For DVB subtitles, a subtitle is displayed until the next one appears.
			// Use next_start_time as the end_time to ensure subtitle N ends when N+1 starts.
			// This prevents any overlap between consecutive subtitles.
			// Issue 7: Use FTS-based duration calculation always.
			// If pre_fts_max is available (captured before potential PTS jumps), use it
			// to accurately end the previous sub at the end of the previous timeline.
			LLONG fts_end_time = (pre_fts_max > 0) ? pre_fts_max : next_start_time;

			LLONG duration = fts_end_time - sub->prev->start_time;
			LLONG capped_timeout = (sub->prev->time_out > 0 && sub->prev->time_out < DVB_MAX_SUBTITLE_DURATION_MS)
						   ? sub->prev->time_out
						   : DVB_MAX_SUBTITLE_DURATION_MS;

			if (duration <= 0 || duration > capped_timeout)
			{
				fts_end_time = sub->prev->start_time + capped_timeout;
			}

			sub->prev->end_time = fts_end_time;

			// Sanity check: if end_time still <= start_time (e.g. due to resets), force 1ms
			if (sub->prev->end_time <= sub->prev->start_time)
			{
				dbg_print(CCX_DMT_DVB, "DVB timing: end <= start, using start+1\n");
				sub->prev->end_time = sub->prev->start_time + 1;
			}
			int timeok = 1;
			if (dec_ctx->extraction_start.set &&
			    sub->prev->start_time < dec_ctx->extraction_start.time_in_ms)
				timeok = 0;
			if (dec_ctx->extraction_end.set &&
			    sub->prev->end_time > dec_ctx->extraction_end.time_in_ms)
			{
				timeok = 0;
				dec_ctx->processed_enough = 1;
			}
			if (timeok)
			{
				encode_sub(enc_ctx->prev, sub->prev); // we encode it

				// Update last recognized string (used in Matroska)
				// Move ownership from prev to main context
				if (enc_ctx->prev)
				{
					enc_ctx->last_str = enc_ctx->prev->last_str;
					enc_ctx->prev->last_str = NULL;
				}

				enc_ctx->srt_counter = enc_ctx->prev->srt_counter; // for dvb subs we need to update the current srt counter because we always encode the previous subtitle (and the counter is increased for the previous context)
				enc_ctx->prev_start = enc_ctx->prev->prev_start;
				sub->prev->got_output = 0;
				if (enc_ctx->write_format == CCX_OF_WEBVTT)
				{ // we already wrote header, but since we encoded last sub, we must prevent multiple headers in future
					enc_ctx->wrote_webvtt_header = 1;
				}
			}
		}
	}
	/* copy previous encoder context*/

	free_encoder_context(enc_ctx->prev);

	enc_ctx->prev = NULL;
	enc_ctx->prev = copy_encoder_context(enc_ctx);

	/* copy previous decoder context */
	free_decoder_context(dec_ctx->prev);
	dec_ctx->prev = NULL;
	dec_ctx->prev = copy_decoder_context(dec_ctx);

	freep(&dec_ctx->prev->private_data);
	dec_ctx->prev->private_data = malloc(sizeof(struct DVBSubContext));
	if (!dec_ctx->prev->private_data)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In dvbsub_handle_display_segment: Out of memory allocating private_data.");
	}
	// Use safe copy that doesn't alias linked list pointers
	dvbsub_copy_context(dec_ctx->prev->private_data, dec_ctx->private_data);

	// Issue 6: Removed workaround. Version management should be handled by logic, not forcing -1.
	// Reference: ((DVBSubContext *)dec_ctx->prev->private_data)->version = -1;

	/* copy previous subtitle */
	free_subtitle(sub->prev);
	sub->time_out = ctx->time_out;
	sub->prev = NULL;
	sub->prev = copy_subtitle(sub);
	// Use get_fts() which properly handles PTS jumps and maintains monotonic timing
	// Validate current_field (valid: 1=field1, 2=field2, 3=CEA-708)
	int sub_caption_field = dec_ctx->current_field;
	if (sub_caption_field < 1 || sub_caption_field > 3)
		sub_caption_field = 1;
	sub->prev->start_time = get_fts(dec_ctx->timing, sub_caption_field);
	// Store the raw PTS for accurate duration calculation (not affected by PTS jump handling)
	sub->prev->start_pts = current_pts;

	// Use current dec_ctx (not prev) because we need valid region/object data for rendering
	// dec_ctx->prev has NULL pointers to avoid memory corruption from aliased linked lists
	write_dvb_sub(dec_ctx, sub->prev); // we write the current dvb sub to update decoder context
	enc_ctx->write_previous = 1;	   // we update our boolean value so next time the program reaches this block of code, it encodes the previous sub

#ifdef ENABLE_OCR
	if (sub->prev)
	{
		struct cc_bitmap *content_prev = sub->prev->data;
		dbg_print(CCX_DMT_DVB, "\nPrevious subtitle %x (%s)\nStart time: %lld; End time: %lld",
			  sub->prev, content_prev ? (content_prev->ocr_text ? content_prev->ocr_text : "NULL OCR") : "NULL DATA",
			  sub->prev->start_time, sub->prev->end_time);
	}
	struct cc_bitmap *content = sub->data;
	dbg_print(CCX_DMT_DVB, "\nCurrent subtitle %x (%s)\nStart time: %lld; End time: %lld\n",
		  sub, content ? (content->ocr_text ? content->ocr_text : "NULL OCR") : "NULL DATA",
		  sub->start_time, sub->end_time);

#endif
}

/**
 * @param dvb_ctx    PreInitialized DVB context using DVB
 * @param buf        buffer containing segment data, first sync byte need to 0x0f.
 *                   does not include data_identifier and subtitle_stream_id.
 * @param buf_size   size of buf buffer
 * @param sub        output subtitle data
 *
 * @return           -1 on error
 */
int dvbsub_decode(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, const unsigned char *buf, int buf_size, struct cc_subtitle *sub)
{
	DVBSubContext *ctx = (DVBSubContext *)dec_ctx->private_data;
	const uint8_t *p, *p_end;
	int segment_type;
	int page_id;
	int segment_length;
	int ret = 0;
	int got_segment = 0;

	// Safety check: Context may be NULL after PAT change
	if (!ctx)
		return -1;

	// Clear last recognized string to avoid leakage between calls
	if (enc_ctx)
		freep(&enc_ctx->last_str);

	// Sync loop: Advance buffer until we find the 0x0F sync byte
	while (buf_size > 0 && *buf != 0x0f)
	{
		buf++;
		buf_size--;
	}

	if (buf_size <= 6)
	{
		return -1;
	}

	p = buf;
	p_end = buf + buf_size;

	// Capture the max FTS before set_fts() processes any PTS jumps.
	// This will be used as the end time for the previous subtitle.
	LLONG pre_fts_max = get_fts_max(dec_ctx->timing);

	dec_ctx->timing->current_tref = 0;
	set_fts(dec_ctx->timing);

	while (p_end - p >= 6 && *p == 0x0f)
	{
		p += 1;
		segment_type = *p++;
		page_id = RB16(p);
		p += 2;
		segment_length = RB16(p);
		p += 2;

		if (p_end - p < segment_length)
		{
			mprint("dvbsub_decode: incomplete, broken or empty packet, remaining bytes=%d, segment_length=%d\n",
			       p_end - p, segment_length);
			return -1;
		}

		if (page_id == ctx->composition_id || page_id == ctx->ancillary_id || ctx->composition_id == -1 || ctx->ancillary_id == -1)
		{
			// debug traces
			// Unconditional trace for debugging

			dbg_print(CCX_DMT_DVB, "DVBSUB - PTS: %" PRId64 ", ", dec_ctx->timing->current_pts);
			dbg_print(CCX_DMT_DVB, "FTS: %d, ", dec_ctx->timing->fts_now);
			dbg_print(CCX_DMT_DVB, "SEGMENT TYPE: %2X, ", segment_type);
			switch (segment_type)
			{
				case DVBSUB_PAGE_SEGMENT:
					dbg_print(CCX_DMT_DVB, "(DVBSUB_PAGE_SEGMENT), SEGMENT LENGTH: %d", segment_length);
					dvbsub_parse_page_segment(ctx, p, segment_length);
					got_segment |= 1;
					break;
				case DVBSUB_REGION_SEGMENT:
					dbg_print(CCX_DMT_DVB, "(DVBSUB_REGION_SEGMENT), SEGMENT LENGTH: %d", segment_length);
					dvbsub_parse_region_segment(ctx, p, segment_length);
					got_segment |= 2;
					break;
				case DVBSUB_CLUT_SEGMENT:
					dbg_print(CCX_DMT_DVB, "(DVBSUB_CLUT_SEGMENT), SEGMENT LENGTH: %d", segment_length);
					ret = dvbsub_parse_clut_segment(ctx, p, segment_length);
					if (ret < 0)
						goto end;
					got_segment |= 4;
					break;
				case DVBSUB_OBJECT_SEGMENT:
					dbg_print(CCX_DMT_DVB, "(DVBSUB_OBJECT_SEGMENT), SEGMENT LENGTH: %d", segment_length);
					ret = dvbsub_parse_object_segment(ctx, p, segment_length);
					if (ret < 0)
						goto end;
					got_segment |= 8;
					break;
				case DVBSUB_DISPLAYDEFINITION_SEGMENT:
					dbg_print(CCX_DMT_DVB, "(DVBSUB_DISPLAYDEFINITION_SEGMENT), SEGMENT LENGTH: %d", segment_length);
					dvbsub_parse_display_definition_segment(ctx, p,
										segment_length);
					break;
				case DVBSUB_DISPLAY_SEGMENT:
					dvbsub_handle_display_segment(enc_ctx, dec_ctx, sub, pre_fts_max);

					got_segment |= 16;
					break;
				default:
					if (segment_type == 0) // Padding
					{
						p += segment_length;
						continue;
					}

					dbg_print(CCX_DMT_DVB, "Subtitling segment type 0x%x, page id %d, length %d\n",
						  segment_type, page_id, segment_length);
					break;
			}
		}
		dbg_print(CCX_DMT_DVB, "\n");
		p += segment_length;
	}
	// Some streams do not send a display segment but if we have all the other
	// segments then we need no further data.
	if (got_segment == 15)
	{
		dvbsub_handle_display_segment(enc_ctx, dec_ctx, sub, pre_fts_max);
		got_segment |= 16;
	}
end:
	if (ret >= 0)
		ret = p - buf;

	return ret;
}
/**
 * @func parse_dvb_description
 *
 * data pointer to this function should be after description length and description tag is parsed
 *
 * @param cfg preallocated dvb_config structure where parsed description will be stored,Not to be NULL
 *
 * @return return -1 if invalid data found other wise 0 if everything goes well
 * errno is set is to EINVAL if invalid data is found
 */
int parse_dvb_description(struct dvb_config *cfg, unsigned char *data,
			  unsigned int len)
{
	/* 8 bytes per DVB subtitle substream d2:
	 * ISO_639_language_code (3 bytes),
	 * subtitling_type (1 byte),
	 * composition_page_id (2 bytes),
	 * ancillary_page_id (2 bytes)
	 */

	cfg->n_language = len / 8;

	if (len > 0 && len % 8 != 0)
	{
		errno = EINVAL;
		return -1;
	}

	if (cfg->n_language > MAX_LANGUAGE_PER_DESC)
	{
		mprint("Warning: more than %d languages in DVB descriptor, only parsing first %d\n", MAX_LANGUAGE_PER_DESC, MAX_LANGUAGE_PER_DESC);
		cfg->n_language = MAX_LANGUAGE_PER_DESC;
	}

	if (cfg->n_language > 1)
	{
		mprint("DVB subtitles with multiple languages\n");
	}

	for (int i = 0; i < cfg->n_language; i++)
	{
		unsigned char *ptr = data + (i * 8);
		char lang_name[4];
		lang_name[0] = ptr[0];
		lang_name[1] = ptr[1];
		lang_name[2] = ptr[2];
		lang_name[3] = '\0';

		for (int j = 0; language[j] != NULL; j++)
		{
			if (!strncmp(lang_name, language[j], 3))
			{
				cfg->lang_index[i] = j;
				break;
			}
		}
		cfg->sub_type[i] = ptr[3];
		cfg->composition_id[i] = RB16(ptr + 4);
		cfg->ancillary_id[i] = RB16(ptr + 6);
	}

	if (ccx_options.dvblang)
	{
		if (strcmp(ccx_options.dvblang, language[cfg->lang_index[0]]) && strncmp(ccx_options.dvblang, (const char *)data, 3))
		{
			mprint("Ignoring stream language index %d not equal to dvblang '%s'\n",
			       cfg->lang_index[0], ccx_options.dvblang);
			return -1;
		}
	}

	return 0;
}
