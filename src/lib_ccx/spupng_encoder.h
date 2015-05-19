#ifndef _SPUPNG_ENCODER_H
#define _SPUPNG_ENCODER_H

#include "ccx_common_platform.h"
#include "ccx_encoders_structs.h"
#include "png.h"

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
void write_sputag(struct spupng_t *sp,LLONG ms_start,LLONG ms_end);
void write_spucomment(struct spupng_t *sp,const char *str);
char* get_spupng_filename(void *ctx);
void inc_spupng_fileindex(void *ctx);
void set_spupng_offset(void *ctx,int x,int y);
int mapclut_paletee(png_color *palette, png_byte *alpha, uint32_t *clut,
		uint8_t depth);
#endif
