#include <assert.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

#include "608_spupng.h"

// #include "wstfont2.xbm" // Teletext font, not used
#include "ccfont2.xbm" // CC font from libzvbi

// CC page dimensions
#define ROWS                    15
#define COLUMNS                 32

/* Closed Caption character cell dimensions */
#define CCW 16
#define CCH 26 /* line doubled */
#define CCPL (ccfont2_width / CCW * ccfont2_height / CCH)

void
write_spumux_header(struct ccx_s_write *wb)
{
    if (0 == wb->spupng_data)
        wb->spupng_data = new SpuPng(wb);
    ((SpuPng*)wb->spupng_data) -> writeHeader();
}

void
write_spumux_footer(struct ccx_s_write *wb)
{
    if (0 != wb->spupng_data)
    {
        ((SpuPng*)wb->spupng_data) -> writeFooter();
        delete (SpuPng*)wb->spupng_data;
        wb->spupng_data = 0;
		wb->fh = -1;
    }
}

int
write_cc_buffer_as_spupng(struct eia608_screen *data, struct ccx_s_write *wb)
{
    if (0 != wb->spupng_data)
    {
        return ((SpuPng*)wb->spupng_data) -> writeCCBuffer(data, wb);
    }
    return 0;
}

static int initialized = 0;

SpuPng::SpuPng(struct ccx_s_write* wb)
{
    if (!initialized)
    {
        initialized = 1;
        initFont();
    }

    if ((fpxml = fdopen(wb->fh, "w")) == NULL)
    {
        fatal(EXIT_FILE_CREATION_FAILED, "Cannot open %s: %s\n", wb->filename, strerror(errno));
    }
    dirname = new char [strlen(wb->filename) + 3];
    strcpy(dirname, wb->filename);
    char* p = strrchr(dirname, '.');
    if (0 == p)
        p = dirname + strlen(dirname);
    *p = '\0';
    strcat(dirname, ".d");
    if (mkdir(dirname, 0777) != 0)
    {
        if (errno != EEXIST)
        {
            fatal(EXIT_FILE_CREATION_FAILED, "Cannot create %s: %s\n", dirname, strerror(errno));
        }
        // If dirname isn't a directory or if we don't have write permission,
        // the first attempt to create a .png file will fail and we'll exit.
    }

    // enough to append /subNNNN.png
    pngfile = new char [ strlen(dirname) + 13 ];
    fileIndex = 0;

    // For NTSC closed captions and 720x480 DVD subtitle resolution:
    // Each character is 16x26.
    // 15 rows by 32 columns, plus 2 columns for left & right padding
    // So each .png image will be 16*34 wide and 26*15 high, or 544x390
    // To center image in 720x480 DVD screen, offset image by 88 and 45
    // Need to keep yOffset even to prevent flicker on interlaced displays
    // Would need to do something different for PAL format and teletext.
    xOffset = 88;
    yOffset = 46;
}

SpuPng::~SpuPng()
{
    delete [] dirname;
    delete [] pngfile;
}


void
SpuPng::writeHeader()
{
    fprintf(fpxml, "<subpictures>\n<stream>\n");
    if (num_input_files > 0)
        fprintf(fpxml, "<!-- %s -->\n", inputfile[0]);
}

void
SpuPng::writeFooter()
{
    fprintf(fpxml, "</stream>\n</subpictures>\n");
    fflush(fpxml);
    fclose(fpxml);
}

int
SpuPng::writeCCBuffer(struct eia608_screen* data, struct ccx_s_write *wb)
{
    LLONG ms_start = wb->data608->current_visible_start_ms + subs_delay;
    if (ms_start < 0)
    {
        dbg_print(CCX_DMT_VERBOSE, "Negative start\n");
        return 0;
    }

    int row;
    int empty_buf = 1;
    for (row = 0; row < 15; row++)
    {
        if (data->row_used[row])
        {
            empty_buf = 0;
            break;
        }
    }
    if (empty_buf)
    {
        dbg_print(CCX_DMT_VERBOSE, "Blank page\n");
        return 0;
    }

    LLONG ms_end=get_visible_end()+subs_delay;

    sprintf(pngfile, "%s/sub%04d.png", dirname, fileIndex++);
    if ((fppng = fopen(pngfile, "wb")) == NULL)
    {
        fatal(EXIT_FILE_CREATION_FAILED, "Cannot open %s: %s\n", pngfile, strerror(errno));
    }
    if (!exportPNG(data))
    {
        fatal(EXIT_FILE_CREATION_FAILED, "Cannot write %s: %s\n", pngfile, strerror(errno));
    }
    fclose(fppng);

    fprintf(fpxml, "<spu start=\"%.3f\"", ((double)ms_start) / 1000);
    dbg_print(CCX_DMT_608, "<spu start=\"%.3f\"", ((double)ms_start) / 1000);
    fprintf(fpxml, " end=\"%.3f\"", ((double)ms_end) / 1000);
    dbg_print(CCX_DMT_608, " end=\"%.3f\"", ((double)ms_end) / 1000);
    fprintf(fpxml, " image=\"%s\"", pngfile);
    dbg_print(CCX_DMT_608, " image=\"%s\"", pngfile);
    fprintf(fpxml, " xoffset=\"%d\"", xOffset);
    dbg_print(CCX_DMT_608, " xoffset=\"%d\"", xOffset);
    fprintf(fpxml, " yoffset=\"%d\"", yOffset);
    dbg_print(CCX_DMT_608, " yoffset=\"%d\"", yOffset);
    fprintf(fpxml, ">\n<!--\n");
    dbg_print(CCX_DMT_608, ">\n<!--\n");
    for (row = 0; row < ROWS; row++)
    {
        if (data->row_used[row])
        {
            int len = get_decoder_line_encoded(subline, row, data);
            // Check for characters that spumux won't parse
            // null chars will be changed to space
            // pairs of dashes will be changed to underscores
            for (unsigned char* ptr = subline; ptr < subline+len; ptr++)
            {
                switch (*ptr)
                {
                    case 0:
                        *ptr = ' ';
                        break;
                    case '-':
                        if (*(ptr+1) == '-')
                        {
                            *ptr++ = '_';
                            *ptr = '_';
                        }
                        break;
                }
            }
            fprintf(fpxml, "%s\n", subline);
            dbg_print(CCX_DMT_608, "%s\n", subline);
        }
    }
    fprintf(fpxml, "--></spu>\n");
    dbg_print(CCX_DMT_608, "--></spu>\n");

    fflush(fpxml);

    return 1;
}

//
// Begin copy from http://zapping.cvs.sourceforge.net/viewvc/zapping/vbi/src/exp-gfx.c?view=markup&pathrev=zvbi-0-2-33
//

void
SpuPng::initFont(void)
{
    uint8_t *t, *p;
    int i, j;

    /* de-interleave font image (puts all chars in row 0) */
#if 0
    if (!(t = malloc(wstfont2_width * wstfont2_height / 8)))
        exit(EXIT_FAILURE);

    for (p = t, i = 0; i < TCH; i++)
        for (j = 0; j < wstfont2_height; p += wstfont2_width / 8, j += TCH)
            memcpy(p, wstfont2_bits + (j + i) * wstfont2_width / 8,
                   wstfont2_width / 8);

    memcpy(wstfont2_bits, t, wstfont2_width * wstfont2_height / 8);
    free (t);
#endif
    if (!(t = (uint8_t*)malloc(ccfont2_width * ccfont2_height / 8)))
        exit(EXIT_FAILURE);

    for (p = t, i = 0; i < CCH; i++)
        for (j = 0; j < ccfont2_height; p += ccfont2_width / 8, j += CCH)
            memcpy(p, ccfont2_bits + (j + i) * ccfont2_width / 8,
                   ccfont2_width / 8);

    memcpy(ccfont2_bits, t, ccfont2_width * ccfont2_height / 8);
    free(t);
}

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
static unsigned int
unicode_ccfont2(unsigned int c, int italic)
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
        c = c;
    else {
        for (i = 0; i < sizeof(specials) / sizeof(specials[0]); i++)
            if (specials[i] == c) {
                c = i + 6;
                goto slant;
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
static inline void
draw_char(int canvas_type, uint8_t *canvas, int rowstride,
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

    for (y = 0; y < ch; underline >>= 1, y++) {
        int bits = ~0;

        if (!(underline & 1)) {
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
 */
static inline void
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

/*
 * PNG and XPM drawing functions (palette-based)
 */
static void
draw_char_indexed(uint8_t * canvas, int rowstride,  uint8_t * pen,
		     int unicode, int italic, int underline)
{
    draw_char(sizeof(*canvas), canvas, rowstride,
              pen, (uint8_t *) ccfont2_bits, CCPL, CCW, CCH,
              unicode_ccfont2(unicode, italic),
              underline * (3 << 24) /* cell row 24, 25 */);
}


void
draw_row(struct eia608_screen* data, int row, uint8_t * canvas, int rowstride)
{
    int column;
    int unicode = 0;
    uint8_t pen[2];
    uint8_t* cell;
    int first = -1;
    int last = 0;

    pen[0] = COL_BLACK;

    for (column = 0; column < COLUMNS ; column++) {

        if (COL_TRANSPARENT != data->colors[row][column])
        {
            cell = canvas + ((column+1) * CCW);
            get_char_in_unicode((unsigned char*)&unicode, data->characters[row][column]);
            pen[1] = data->colors[row][column];

            int attr = data->fonts[row][column];
            draw_char_indexed(cell, rowstride, pen, unicode, (attr & FONT_ITALICS) != 0, (attr & FONT_UNDERLINED) != 0);
            if (first < 0)
            {
                // draw a printable space before the first non-space char
                first = column;
                if (unicode != 0x20)
                {
                    cell = canvas + ((first) * CCW);
                    draw_char_indexed(cell, rowstride, pen, 0x20, 0, 0);
                }
            }
            last = column;
        }
    }
    // draw a printable space after the last non-space char
    // unicode should still contain the last character
    // check whether it is a space
    if (unicode != 0x20)
    {
        cell = canvas + ((last+2) * CCW);
        draw_char_indexed(cell, rowstride, pen, 0x20, 0, 0);
    }
}

static png_color palette[10] =
{
    { 0xff, 0xff, 0xff }, // COL_WHITE = 0,
    { 0x00, 0xff, 0x00 }, // COL_GREEN = 1,
    { 0x00, 0x00, 0xff }, // COL_BLUE = 2,
    { 0x00, 0xff, 0xff }, // COL_CYAN = 3,
    { 0xff, 0x00, 0x00 }, // COL_RED = 4,
    { 0xff, 0xff, 0x00 }, // COL_YELLOW = 5,
    { 0xff, 0x00, 0xff }, // COL_MAGENTA = 6,
    { 0xff, 0xff, 0xff }, // COL_USERDEFINED = 7,
    { 0x00, 0x00, 0x00 }, // COL_BLACK = 8
    { 0x00, 0x00, 0x00 } // COL_TRANSPARENT = 9
};

static png_byte alpha[10] =
{
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    255,
    0
};

int
SpuPng::writePNG(struct eia608_screen* data,
        png_structp png_ptr, png_infop info_ptr,
        png_bytep image,
        png_bytep* row_pointer,
        unsigned int ww,
        unsigned int wh)
{
    unsigned int i;

    if (setjmp(png_jmpbuf(png_ptr)))
            return 0;

    png_init_io (png_ptr, fppng);

    png_set_IHDR (png_ptr,
                  info_ptr,
                  ww,
                  wh,
                  /* bit_depth */ 8,
                  PNG_COLOR_TYPE_PALETTE,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);

    png_set_PLTE (png_ptr, info_ptr, palette, sizeof(palette) / sizeof(palette[0]));
    png_set_tRNS (png_ptr, info_ptr, alpha, sizeof(alpha) / sizeof(alpha[0]), NULL);

    png_set_gAMA (png_ptr, info_ptr, 1.0 / 2.2);

    png_write_info (png_ptr, info_ptr);

    for (i = 0; i < wh; i++)
        row_pointer[i] = image + i * ww;

    png_write_image (png_ptr, row_pointer);

    png_write_end (png_ptr, info_ptr);

    return 1;
}

int
SpuPng::exportPNG(struct eia608_screen* data)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointer;
    png_bytep image;
    int ww, wh, rowstride, row_adv;
    int row;    

    assert ((sizeof(png_byte) == sizeof(uint8_t))
            && (sizeof(*image) == sizeof(uint8_t)));

    // Allow space at beginning and end of each row for a padding space
    ww = CCW * (COLUMNS+2);
    wh = CCH * ROWS;
    row_adv = (COLUMNS+2) * CCW * CCH;

    rowstride = ww * sizeof(*image);

    if (!(row_pointer = (png_bytep*)malloc(sizeof(*row_pointer) * wh))) {
        mprint("Unable to allocate %d byte buffer.\n",
                                sizeof(*row_pointer) * wh);
        return 0;
    }

    if (!(image = (png_bytep)malloc(wh * ww * sizeof(*image)))) {
        mprint("Unable to allocate %d KB image buffer.",
                                wh * ww * sizeof(*image) / 1024);
        free(row_pointer);
        return 0;
    }
    // Initialize image to transparent
    memset(image, COL_TRANSPARENT, wh * ww * sizeof(*image));

    /* draw the image */

    for (row = 0; row < ROWS; row++) {
        if (data->row_used[row])
            draw_row(data, row, image + row * row_adv, rowstride);
    }

    /* Now save the image */

    if (!(png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                            NULL, NULL, NULL)))
        goto unknown_error;

    if (!(info_ptr = png_create_info_struct(png_ptr))) {
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        goto unknown_error;
    }

    if (!writePNG (data, png_ptr, info_ptr, image, row_pointer, ww, wh)) {
        png_destroy_write_struct (&png_ptr, &info_ptr);
        goto write_error;
    }

    png_destroy_write_struct (&png_ptr, &info_ptr);

    free (row_pointer);

    free (image);

    return 1;

write_error:

unknown_error:
    free (row_pointer);

    free (image);

    return 0;
}

//
// End copy from http://zapping.cvs.sourceforge.net/viewvc/zapping/vbi/src/exp-gfx.c?view=markup&pathrev=zvbi-0-2-33
//

