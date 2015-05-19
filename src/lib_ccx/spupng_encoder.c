#include "ccfont2.xbm" // CC font from libzvbi
#include "spupng_encoder.h"
#include "ccx_common_common.h"
#include "ccx_encoders_common.h"
#include <assert.h>
#ifdef ENABLE_OCR
#include "ocr.h"
#undef OCR_DEBUG
#endif

#define CCPL (ccfont2_width / CCW * ccfont2_height / CCH)

static int initialized = 0;

void spupng_init_font()
{
	uint8_t *t, *p;
	int i, j;

	/* de-interleave font image (puts all chars in row 0) */
	if (!(t = (uint8_t*)malloc(ccfont2_width * ccfont2_height / 8)))
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "Memory allocation failed");

	for (p = t, i = 0; i < CCH; i++)
		for (j = 0; j < ccfont2_height; p += ccfont2_width / 8, j += CCH)
			memcpy(p, ccfont2_bits + (j + i) * ccfont2_width / 8,
					ccfont2_width / 8);

	memcpy(ccfont2_bits, t, ccfont2_width * ccfont2_height / 8);
	free(t);
}

struct spupng_t *spunpg_init(struct ccx_s_write *out)
{
	struct spupng_t *sp = (struct spupng_t *) malloc(sizeof(struct spupng_t));
	if (NULL == sp)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "Memory allocation failed");

	if (!initialized)
	{
		initialized = 1;
		spupng_init_font();
	}

	if ((sp->fpxml = fdopen(out->fh, "w")) == NULL)
	{
		ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Cannot open %s: %s\n",
				out->filename, strerror(errno));
	}
	sp->dirname = (char *) malloc(
			sizeof(char) * (strlen(out->filename) + 3));
	if (NULL == sp->dirname)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "Memory allocation failed");

	strcpy(sp->dirname, out->filename);
	char* p = strrchr(sp->dirname, '.');
	if (NULL == p)
		p = sp->dirname + strlen(sp->dirname);
	*p = '\0';
	strcat(sp->dirname, ".d");
	if (0 != mkdir(sp->dirname, 0777))
	{
		if (errno != EEXIST)
		{
			ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Cannot create %s: %s\n",
					sp->dirname, strerror(errno));
		}
		// If dirname isn't a directory or if we don't have write permission,
		// the first attempt to create a .png file will fail and we'll XXxit.
	}

	// enough to append /subNNNN.png
	sp->pngfile = (char *) malloc(sizeof(char) * (strlen(sp->dirname) + 13));
	if (NULL == sp->pngfile)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "Memory allocation failed");
	sp->fileIndex = 0;
	sprintf(sp->pngfile, "%s/sub%04d.png", sp->dirname, sp->fileIndex);

	// For NTSC closed captions and 720x480 DVD subtitle resolution:
	// Each character is 16x26.
	// 15 rows by 32 columns, plus 2 columns for left & right padding
	// So each .png image will be 16*34 wide and 26*15 high, or 544x390
	// To center image in 720x480 DVD screen, offset image by 88 and 45
	// Need to keep yOffset even to prevent flicker on interlaced displays
	// Would need to do something different for PAL format and teletext.
	sp->xOffset = 88;
	sp->yOffset = 46;

	return sp;
}
void spunpg_free(struct spupng_t *sp)
{
	free(sp->dirname);
	free(sp->pngfile);
	free(sp);
}

void spupng_write_header(struct spupng_t *sp,int multiple_files,char *first_input_file)
{
	fprintf(sp->fpxml, "<subpictures>\n<stream>\n");
	if (multiple_files)
		fprintf(sp->fpxml, "<!-- %s -->\n", first_input_file);
}

void spupng_write_footer(struct spupng_t *sp)
{
	fprintf(sp->fpxml, "</stream>\n</subpictures>\n");
	fflush(sp->fpxml);
	fclose(sp->fpxml);
}

void write_spumux_header(struct ccx_s_write *out)
{
	if (0 == out->spupng_data)
		out->spupng_data = spunpg_init(out);

	spupng_write_header((struct spupng_t*)out->spupng_data,out->multiple_files,out->first_input_file);
}

void write_spumux_footer(struct ccx_s_write *out)
{
	if (0 != out->spupng_data)
	{
		struct spupng_t *sp = (struct spupng_t *) out->spupng_data;

		spupng_write_footer(sp);
		spunpg_free(sp);

		out->spupng_data = 0;
		out->fh = -1;
	}
}

/**
 * @internal
 * @param p Plane of @a canvas_type char, short, int.
 * @param i Index.
 *
 * @return
 * Pixel @a i in plane @a p.
 */
#define peek(p, i)							\
((canvas_type == sizeof(uint8_t)) ? ((uint8_t *)(p))[i] :		\
    ((canvas_type == sizeof(uint16_t)) ? ((uint16_t *)(p))[i] :		\
	((uint32_t *)(p))[i]))

/**
 * @internal
 * @param p Plane of @a canvas_type char, short, int.
 * @param i Index.
 * @param v Value.
 *
 * Set pixel @a i in plane @a p to value @a v.
 */
#define poke(p, i, v)							\
((canvas_type == sizeof(uint8_t)) ? (((uint8_t *)(p))[i] = (v)) :	\
    ((canvas_type == sizeof(uint16_t)) ? (((uint16_t *)(p))[i] = (v)) :	\
	(((uint32_t *)(p))[i] = (v))))

/**
 * @internal
 * @param c Unicode.
 * @param italic @c TRUE to switch to slanted character set.
 *
 * Translate Unicode character to glyph number in ccfont2 image.
 *
 * @return
 * Glyph number.
 */
static unsigned int unicode_ccfont2(unsigned int c, int italic)
{
	static const unsigned short specials[] = {
		0x00E1, 0x00E9,
		0x00ED, 0x00F3, 0x00FA, 0x00E7, 0x00F7, 0x00D1, 0x00F1, 0x25A0,
		0x00AE, 0x00B0, 0x00BD, 0x00BF, 0x2122, 0x00A2, 0x00A3, 0x266A,
		0x00E0, 0x0020, 0x00E8, 0x00E2, 0x00EA, 0x00EE, 0x00F4, 0x00FB };
	unsigned int i;

	if (c < 0x0020)
		c = 15; /* invalid */
	else if (c < 0x0080)
		/*c = c */;
	else
	{
		for (i = 0; i < sizeof(specials) / sizeof(specials[0]); i++)
		{
			if (specials[i] == c)
			{
				c = i + 6;
				goto slant;
			}
		}
		c = 15; /* invalid */
	}

slant:
	if (italic)
		c += 4 * 32;

	return c;
}

/**
 * @internal
 * @param canvas_type sizeof(char, short, int).
 * @param canvas Pointer to image plane where the character is to be drawn.
 * @param rowstride @a canvas <em>byte</em> distance from line to line.
 * @param color Color value of @a canvas_type.
 * @param cw Character width in pixels.
 * @param ch Character height in pixels.
 *
 * Draw blank character.
static void
draw_blank(int canvas_type, uint8_t *canvas, unsigned int rowstride,
	   unsigned int color, int cw, int ch)
{
    int x, y;

    for (y = 0; y < ch; y++) {
        for (x = 0; x < cw; x++)
            poke(canvas, x, color);

        canvas += rowstride;
    }
}
*/

/**
 * @internal
 * @param canvas_type sizeof(char, short, int).
 * @param canvas Pointer to image plane where the character is to be drawn.
 * @param rowstride @a canvas <em>byte</em> distance from line to line.
 * @param pen Pointer to color palette of @a canvas_type (index 0 background
 *   pixels, index 1 foreground pixels).
 * @param font Pointer to font image with width @a cpl x @a cw pixels, height
 *   @a ch pixels, depth one bit, bit '1' is foreground.
 * @param cpl Chars per line (number of characters in @a font image).
 * @param cw Character cell width in pixels.
 * @param ch Character cell height in pixels.
 * @param glyph Glyph number in font image, 0 ... @a cpl - 1.
 * @param underline Bit mask of character rows. For each bit
 *   1 << (n = 0 ... @a ch - 1) set all of character row n to
 *   foreground color.
 * @param size Size of character, either NORMAL, DOUBLE_WIDTH (draws left
 *   and right half), DOUBLE_HEIGHT (draws upper half only),
 *   DOUBLE_SIZE (left and right upper half), DOUBLE_HEIGHT2
 *   (lower half), DOUBLE_SIZE2 (left and right lower half).
 *
 * Draw one character (function template - define a static version with
 * constant @a canvas_type, @a font, @a cpl, @a cw, @a ch).
 */
static void draw_char(int canvas_type, uint8_t *canvas, int rowstride,
	  uint8_t *pen, uint8_t *font, int cpl, int cw, int ch,
	  int glyph, unsigned int underline)
{
	uint8_t *src;
	int shift, x, y;

	assert(cw >= 8 && cw <= 16);
	assert(ch >= 1 && ch <= 31);

	x = glyph * cw;
	shift = x & 7;
	src = font + (x >> 3);

	for (y = 0; y < ch; underline >>= 1, y++)
	{
		int bits = ~0;

		if (!(underline & 1))
		{
#ifdef __i386__
			bits = (*((uint16_t *) src) >> shift);
#else
			/* unaligned/little endian */
			bits = ((src[1] * 256 + src[0]) >> shift);
#endif
		}

		for (x = 0; x < cw; bits >>= 1, x++)
			poke(canvas, x, peek(pen, bits & 1));

		canvas += rowstride;

		src += cpl * cw / 8;
	}
}

/*
 * PNG and XPM drawing functions (palette-based)
 */
void draw_char_indexed(uint8_t * canvas, int rowstride,  uint8_t * pen,
		     int unicode, int italic, int underline)
{
	draw_char(sizeof(*canvas), canvas, rowstride,
			pen, (uint8_t *) ccfont2_bits, CCPL, CCW, CCH,
			unicode_ccfont2(unicode, italic),
			underline * (3 << 24) /* cell row 24, 25 */);
}

void write_sputag(struct spupng_t *sp,LLONG ms_start,LLONG ms_end)
{
        fprintf(sp->fpxml, "<spu start=\"%.3f\"", ((double)ms_start) / 1000);
        fprintf(sp->fpxml, " end=\"%.3f\"", ((double)ms_end) / 1000);
        fprintf(sp->fpxml, " image=\"%s\"", sp->pngfile);
        fprintf(sp->fpxml, " xoffset=\"%d\"", sp->xOffset);
        fprintf(sp->fpxml, " yoffset=\"%d\"", sp->yOffset);
        fprintf(sp->fpxml, ">\n");
        fprintf(sp->fpxml, "</spu>\n");

}
void write_spucomment(struct spupng_t *sp,const char *str)
{
        fprintf(sp->fpxml, "<!--\n");
        fprintf(sp->fpxml, "%s\n", str);
        fprintf(sp->fpxml, "-->\n");
}

char* get_spupng_filename(void *ctx)
{
	struct spupng_t *sp = (struct spupng_t *)ctx;
	return sp->pngfile;
}
void inc_spupng_fileindex(void *ctx)
{
        struct spupng_t *sp = (struct spupng_t *)ctx;
        sp->fileIndex++;
        sprintf(sp->pngfile, "%s/sub%04d.png", sp->dirname, sp->fileIndex);
}
void set_spupng_offset(void *ctx,int x,int y)
{
	struct spupng_t *sp = (struct spupng_t *)ctx;
	sp->xOffset = x;
	sp->yOffset = y;
}
int save_spupng(const char *filename, uint8_t *bitmap, int w, int h,
		png_color *palette, png_byte *alpha, int nb_color)
{
	FILE *f = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytep* row_pointer = NULL;
	int i, j, ret = 0;
	int k = 0;
	if(!h)
		h = 1;
	if(!w)
		w = 1;


	f = fopen(filename, "wb");
	if (!f)
	{
		ccx_common_logging.log_ftn("DVB:unable to open %s in write mode \n", filename);
		ret = -1;
		goto end;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,NULL);
	if (!png_ptr)
	{
		ccx_common_logging.log_ftn("DVB:unable to create png write struct\n");
		goto end;
	}
	if (!(info_ptr = png_create_info_struct(png_ptr)))
	{
		ccx_common_logging.log_ftn("DVB:unable to create png info struct\n");
		ret = -1;
		goto end;
	}

	row_pointer = (png_bytep*) malloc(sizeof(png_bytep) * h);
	if (!row_pointer)
	{
		ccx_common_logging.log_ftn("DVB: unable to allocate row_pointer\n");
		ret = -1;
		goto end;
	}
	memset(row_pointer, 0, sizeof(png_bytep) * h);
	png_init_io(png_ptr, f);

	png_set_IHDR(png_ptr, info_ptr, w, h,
	/* bit_depth */8,
	PNG_COLOR_TYPE_PALETTE,
	PNG_INTERLACE_NONE,
	PNG_COMPRESSION_TYPE_DEFAULT,
	PNG_FILTER_TYPE_DEFAULT);

	png_set_PLTE(png_ptr, info_ptr, palette, nb_color);
	png_set_tRNS(png_ptr, info_ptr, alpha, nb_color, NULL);

	for (i = 0; i < h; i++)
	{
		row_pointer[i] = (png_byte*) malloc(
				png_get_rowbytes(png_ptr, info_ptr));
		if (row_pointer[i] == NULL)
			break;
	}
	if (i != h)
	{
		ccx_common_logging.log_ftn("DVB: unable to allocate row_pointer internals\n");
		ret = -1;
		goto end;
	}
	png_write_info(png_ptr, info_ptr);
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < png_get_rowbytes(png_ptr, info_ptr); j++)
		{
			if(bitmap)
				k = bitmap[i * w + (j)];
			else
				k = 0;
			row_pointer[i][j] = k;
		}
	}

	png_write_image(png_ptr, row_pointer);

	png_write_end(png_ptr, info_ptr);
	end: if (row_pointer)
	{
		for (i = 0; i < h; i++)
			freep(&row_pointer[i]);
		freep(&row_pointer);
	}
	png_destroy_write_struct(&png_ptr, &info_ptr);
	if (f)
		fclose(f);
	return ret;

}
/**
 * alpha value 255 means completely opaque
 * alpha value 0   means completely transparent
 * r g b all 0 means black
 * r g b all 255 means white
 */
int mapclut_paletee(png_color *palette, png_byte *alpha, uint32_t *clut,
		uint8_t depth)
{
	for (int i = 0; i < depth; i++)
	{
		palette[i].red = ((clut[i] >> 16) & 0xff);
		palette[i].green = ((clut[i] >> 8) & 0xff);
		palette[i].blue = (clut[i] & 0xff);
		alpha[i] = ((clut[i] >> 24) & 0xff);
	}
#if OCR_DEBUG
	ccx_common_logging.log_ftn("Colors present in original Image\n");
	for (int i = 0; i < depth; i++)
	{
		ccx_common_logging.log_ftn("%02d)r %03d g %03d b %03d a %03d\n",
			i, palette[i].red, palette[i].green, palette[i].blue, alpha[i]);
	}
#endif
	return 0;
}

int write_cc_bitmap_as_spupng(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	struct spupng_t *sp = (struct spupng_t *)context->out->spupng_data;
	int x_pos, y_pos, width, height, i;
	int x, y, y_off, x_off, ret=0;
	uint8_t *pbuf;
	char *filename;
	struct cc_bitmap* rect;
	png_color *palette = NULL;
	png_byte *alpha = NULL;

        x_pos = -1;
        y_pos = -1;
        width = 0;
        height = 0;

	if (context->prev_start != -1 && (sub->flags & SUB_EOD_MARKER))
		write_sputag(sp, context->prev_start, sub->start_time);
	else if ( !(sub->flags & SUB_EOD_MARKER))
		write_sputag(sp, sub->start_time, sub->end_time);

	if(sub->nb_data == 0 && (sub->flags & SUB_EOD_MARKER))
	{
		context->prev_start = -1;
		return 0;
	}
	rect = sub->data;
	for(i = 0;i < sub->nb_data;i++)
	{
		if(x_pos == -1)
		{
			x_pos = rect[i].x;
			y_pos = rect[i].y;
			width = rect[i].w;
			height = rect[i].h;
		}
		else
		{
			if(x_pos > rect[i].x)
			{
				width += (x_pos - rect[i].x);
				x_pos = rect[i].x;
			}

                        if (rect[i].y < y_pos)
                        {
                                height += (y_pos - rect[i].y);
                                y_pos = rect[i].y;
                        }

                        if (rect[i].x + rect[i].w > x_pos + width)
                        {
                                width = rect[i].x + rect[i].w - x_pos;
                        }

                        if (rect[i].y + rect[i].h > y_pos + height)
                        {
                                height = rect[i].y + rect[i].h - y_pos;
                        }

		}
	}
	inc_spupng_fileindex(sp);
	filename = get_spupng_filename(sp);
	set_spupng_offset(sp,y_pos,x_pos);
	if ( sub->flags & SUB_EOD_MARKER )
		context->prev_start =  sub->start_time;
	pbuf = (uint8_t*) malloc(width * height);
	memset(pbuf, 0x0, width * height);

	for(i = 0;i < sub->nb_data;i++)
	{
		x_off = rect[i].x - x_pos;
		y_off = rect[i].y - y_pos;
		for (y = 0; y < rect[i].h; y++)
		{
			for (x = 0; x < rect[i].w; x++)
				pbuf[((y + y_off) * width) + x_off + x] = rect[i].data[0][y * rect[i].w + x];

		}
	}
	palette = (png_color*) malloc(rect[0].nb_colors * sizeof(png_color));
	if(!palette)
	{
		ret = -1;
		goto end;
	}
        alpha = (png_byte*) malloc(rect[0].nb_colors * sizeof(png_byte));
        if(!alpha)
        {
                ret = -1;
                goto end;
        }

	/* TODO do rectangle wise, one color table should not be used for all rectangles */
        mapclut_paletee(palette, alpha, (uint32_t *)rect[0].data[1],rect[0].nb_colors);
#ifdef ENABLE_OCR
	if (rect[0].ocr_text && *(rect[0].ocr_text))
	{
		write_spucomment(sp, rect[0].ocr_text);
	}
#endif
	save_spupng(filename,pbuf,width, height, palette, alpha,rect[0].nb_colors);


end:
	sub->nb_data = 0;
	freep(&sub->data);
	freep(&palette);
	freep(&alpha);
	return ret;
}
