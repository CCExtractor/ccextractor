#ifndef _SPUPNG_ENCODER_H
#define _SPUPNG_ENCODER_H

#include <stdio.h>
#include "png.h"
#include "ccextractor.h"

// CC page dimensions
#define ROWS                    15
#define COLUMNS                 32
/* Closed Caption character cell dimensions */
#define CCW 16
#define CCH 26 /* line doubled */

struct spupng_t
{
    FILE* fpxml;
    FILE* fppng;
    char* dirname;
    char* pngfile;
    int fileIndex;
    int xOffset;
    int yOffset;
};

void write_spumux_header(struct ccx_s_write *out);
void write_spumux_footer(struct ccx_s_write *out);
void draw_char_indexed(uint8_t * canvas, int rowstride,  uint8_t * pen,
		     int unicode, int italic, int underline);
#endif
