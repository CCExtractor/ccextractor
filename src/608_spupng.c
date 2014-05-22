#include <assert.h>
#include <sys/stat.h>
#include "608_spupng.h"

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
spupng_write_png(struct spupng_t *sp, struct eia608_screen* data,
                 png_structp png_ptr, png_infop info_ptr,
                 png_bytep image,
                 png_bytep* row_pointer,
                 unsigned int ww, unsigned int wh)
{
    unsigned int i;

    if (setjmp(png_jmpbuf(png_ptr)))
            return 0;

    png_init_io (png_ptr, sp->fppng);

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
spupng_export_png(struct spupng_t *sp, struct eia608_screen* data)
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

    if (!spupng_write_png (sp, data, png_ptr, info_ptr, image, row_pointer, ww, wh)) {
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

int
spupng_write_ccbuffer(struct spupng_t *sp, struct eia608_screen* data,
                      struct s_context_cc608 *context)
{
	LLONG ms_start = context->current_visible_start_ms + subs_delay;
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

    LLONG ms_end = get_visible_end() + subs_delay;

    sprintf(sp->pngfile, "%s/sub%04d.png", sp->dirname, sp->fileIndex++);
    if ((sp->fppng = fopen(sp->pngfile, "wb")) == NULL)
    {
        fatal(EXIT_FILE_CREATION_FAILED, "Cannot open %s: %s\n",
                sp->pngfile, strerror(errno));
    }
    if (!spupng_export_png(sp, data))
    {
        fatal(EXIT_FILE_CREATION_FAILED, "Cannot write %s: %s\n",
                sp->pngfile, strerror(errno));
    }
    fclose(sp->fppng);

    fprintf(sp->fpxml, "<spu start=\"%.3f\"", ((double)ms_start) / 1000);
    dbg_print(CCX_DMT_608, "<spu start=\"%.3f\"", ((double)ms_start) / 1000);
    fprintf(sp->fpxml, " end=\"%.3f\"", ((double)ms_end) / 1000);
    dbg_print(CCX_DMT_608, " end=\"%.3f\"", ((double)ms_end) / 1000);
    fprintf(sp->fpxml, " image=\"%s\"", sp->pngfile);
    dbg_print(CCX_DMT_608, " image=\"%s\"", sp->pngfile);
    fprintf(sp->fpxml, " xoffset=\"%d\"", sp->xOffset);
    dbg_print(CCX_DMT_608, " xoffset=\"%d\"", sp->xOffset);
    fprintf(sp->fpxml, " yoffset=\"%d\"", sp->yOffset);
    dbg_print(CCX_DMT_608, " yoffset=\"%d\"", sp->yOffset);
    fprintf(sp->fpxml, ">\n<!--\n");
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
            fprintf(sp->fpxml, "%s\n", subline);
            dbg_print(CCX_DMT_608, "%s\n", subline);
        }
    }
    fprintf(sp->fpxml, "--></spu>\n");
    dbg_print(CCX_DMT_608, "--></spu>\n");

    fflush(sp->fpxml);

    return 1;
}
int write_cc_buffer_as_spupng(struct eia608_screen *data,struct s_context_cc608 *context)
{
    if (0 != context->out->spupng_data)
    {
        struct spupng_t *sp = (struct spupng_t *) context->out->spupng_data;
        return spupng_write_ccbuffer(sp, data, context);
    }
    return 0;
}

