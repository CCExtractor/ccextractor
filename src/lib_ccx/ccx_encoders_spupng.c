#include "ccfont2.xbm" // CC font from libzvbi
#include "ccx_common_common.h"
#include "ccx_encoders_common.h"
#include "png.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "lib_ccx.h"
#include "ccx_encoders_helpers.h"
#include <assert.h>
#ifdef ENABLE_OCR
#include "ocr.h"
#undef OCR_DEBUG
#endif

/* Closed Caption character cell dimensions */
#define CCW 16
#define CCH 26 /* line doubled */

// TODO: To make them customizable
#define FONT_SIZE 20
#define CANVAS_WIDTH 600

FT_Library  ft_library = NULL;
FT_Face	 face = NULL;

struct spupng_t
{
	FILE* fpxml;
	FILE* fppng;
	char* dirname;
	char* pngfile;
	char* relative_path_png;
	int fileIndex;
	int xOffset;
	int yOffset;
};

#define CCPL (ccfont2_width / CCW * ccfont2_height / CCH)

static int initialized = 0;

void spupng_init_font()
{
	uint8_t *t, *p;
	int i, j;

	/* de-interleave font image (puts all chars in row 0) */
	if (!(t = (uint8_t*)malloc(ccfont2_width * ccfont2_height / 8)))
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "spupng_init_font: Memory allocation failed");

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
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "spunpg_init: Memory allocation failed (sp)");

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
	sp->dirname = (char *)malloc(
		sizeof(char) * (strlen(out->filename) + 3));
	if (NULL == sp->dirname)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "spunpg_init: Memory allocation failed (sp->dirname)");

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
	sp->pngfile = (char *)malloc(sizeof(char) * (strlen(sp->dirname) + 13));
	sp->relative_path_png = (char *)malloc(sizeof(char) * (strlen(sp->dirname) + 13));

	if (NULL == sp->pngfile || NULL == sp->relative_path_png)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "spunpg_init: Memory allocation failed (sp->pngfile)");
	sp->fileIndex = 0;
	sprintf(sp->pngfile, "%s/sub%04d.png", sp->dirname, sp->fileIndex);

	// Make relative path
	char* last_slash = strrchr(sp->dirname, '/');
	if (last_slash == NULL) last_slash = strrchr(sp->dirname, '\\');
	if (last_slash != NULL)
		sprintf(sp->relative_path_png, "%s/sub%04d.png", last_slash + 1, sp->fileIndex);
	else // do NOT do sp->relative_path_png = sp->pngfile (to avoid double free).
		strcpy(sp->relative_path_png, sp->pngfile);

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
	free(sp->relative_path_png);
	free(sp);
}

void spupng_write_header(struct spupng_t *sp, int multiple_files, char *first_input_file)
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

void write_spumux_header(struct encoder_ctx *ctx, struct ccx_s_write *out)
{
	if (0 == out->spupng_data)
		out->spupng_data = spunpg_init(out);

	spupng_write_header((struct spupng_t*)out->spupng_data, ctx->multiple_files, ctx->first_input_file);

	if (ctx->write_format == CCX_OF_RAW) {		// WARN: Memory leak with this flag, free by hand. Maybe bug.
		spupng_write_footer(out->spupng_data);
		spunpg_free(out->spupng_data);
	}
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

void write_sputag_open(struct spupng_t *sp, LLONG ms_start, LLONG ms_end)
{
	fprintf(sp->fpxml, "<spu start=\"%.3f\"", ((double)ms_start) / 1000);
	fprintf(sp->fpxml, " end=\"%.3f\"", ((double)ms_end) / 1000);
	fprintf(sp->fpxml, " image=\"%s\"", sp->relative_path_png);
	fprintf(sp->fpxml, " xoffset=\"%d\"", sp->xOffset);
	fprintf(sp->fpxml, " yoffset=\"%d\"", sp->yOffset);
	fprintf(sp->fpxml, ">\n");
}

void write_sputag_close(struct spupng_t *sp)
{
	fprintf(sp->fpxml, "</spu>\n");
}
void write_spucomment(struct spupng_t *sp, const char *str)
{
	fprintf(sp->fpxml, "<!--\n%s\n-->\n", str);
}

char* get_spupng_filename(void *ctx)
{
	struct spupng_t *sp = (struct spupng_t *)ctx;
	return sp->pngfile;
}
void inc_spupng_fileindex(struct spupng_t *sp)
{
	sp->fileIndex++;
	sprintf(sp->pngfile, "%s/sub%04d.png", sp->dirname, sp->fileIndex);

	// Make relative path
	char* last_slash = strrchr(sp->dirname, '/');
	if (last_slash == NULL) last_slash = strrchr(sp->dirname, '\\');
	if (last_slash != NULL)
		sprintf(sp->relative_path_png, "%s/sub%04d.png", last_slash + 1, sp->fileIndex);
	else // do NOT do sp->relative_path_png = sp->pngfile (to avoid double free).
		strcpy(sp->relative_path_png, sp->pngfile);
}
void set_spupng_offset(void *ctx, int x, int y)
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
	if (!h)
		h = 1;
	if (!w)
		w = 1;

	f = fopen(filename, "wb");
	if (!f)
	{
		ccx_common_logging.log_ftn("DVB:unable to open %s in write mode \n", filename);
		ret = -1;
		goto end;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
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

	row_pointer = (png_bytep*)malloc(sizeof(png_bytep) * h);
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
		row_pointer[i] = (png_byte*)malloc(
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
			if (bitmap)
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
	int x, y, y_off, x_off, ret = 0;
	uint8_t *pbuf;
	char *filename;
	struct cc_bitmap* rect;
	png_color *palette = NULL;
	png_byte *alpha = NULL;
	int wrote_opentag = 1;

	x_pos = -1;
	y_pos = -1;
	width = 0;
	height = 0;

	if (sub->data == NULL)
		return 0;

	inc_spupng_fileindex(sp);
	write_sputag_open(sp, sub->start_time, sub->end_time - 1);

	if (sub->nb_data == 0 && (sub->flags & SUB_EOD_MARKER))
	{
		context->prev_start = -1;
		if (wrote_opentag)
			write_sputag_close(sp);
		return 0;
	}
	rect = sub->data;
	for (i = 0; i < sub->nb_data; i++)
	{
		if (x_pos == -1)
		{
			x_pos = rect[i].x;
			y_pos = rect[i].y;
			width = rect[i].w;
			height = rect[i].h;
		}
		else
		{
			if (x_pos > rect[i].x)
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
	filename = get_spupng_filename(sp);
	set_spupng_offset(sp, x_pos, y_pos);
	if (sub->flags & SUB_EOD_MARKER)
		context->prev_start = sub->start_time;
	pbuf = (uint8_t*)malloc(width * height);
	memset(pbuf, 0x0, width * height);

	for (i = 0; i < sub->nb_data; i++)
	{
		x_off = rect[i].x - x_pos;
		y_off = rect[i].y - y_pos;
		for (y = 0; y < rect[i].h; y++)
		{
			for (x = 0; x < rect[i].w; x++)
				pbuf[((y + y_off) * width) + x_off + x] = rect[i].data0[y * rect[i].w + x];

		}
	}
	palette = (png_color*)malloc(rect[0].nb_colors * sizeof(png_color));
	if (!palette)
	{
		ret = -1;
		goto end;
	}
	alpha = (png_byte*)malloc(rect[0].nb_colors * sizeof(png_byte));
	if (!alpha)
	{
		ret = -1;
		goto end;
	}

	/* TODO do rectangle wise, one color table should not be used for all rectangles */
	mapclut_paletee(palette, alpha, (uint32_t *)rect[0].data1, rect[0].nb_colors);
#ifdef ENABLE_OCR	
	if (!context->nospupngocr)
	{
		char *str;
		str = paraof_ocrtext(sub, context->encoded_crlf, context->encoded_crlf_length);
		if (str)
		{
			write_spucomment(sp, str);
			freep(&str);
		}
	}
#endif
	save_spupng(filename, pbuf, width, height, palette, alpha, rect[0].nb_colors);
	freep(&pbuf);


end:
	if (wrote_opentag)
		write_sputag_close(sp);

	for (i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		freep(&rect->data0);
		freep(&rect->data1);
	}
	sub->nb_data = 0;
	freep(&sub->data);
	freep(&palette);
	freep(&alpha);
	freep(&pbuf);
	return ret;
}

struct pixel_t {
	unsigned char r, g, b, a;
};

// Write the buffer to a file named filename
// Return 1 on success.
int write_image(struct pixel_t *buffer, FILE* fp, int width, int height)
{
	int ret_code = 1;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytep row = NULL;

	if (!(png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
		mprint("\nUnknown error encountered!\n");
		ret_code = 0;
		goto finalise;
	}
	if (!(info_ptr = png_create_info_struct(png_ptr))) {
		mprint("\nUnknown error encountered!\n");
		ret_code = 0;
		goto finalise;
	}

	png_init_io(png_ptr, fp);

	// Write header
	png_set_IHDR(png_ptr, info_ptr, width, height,
		8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);

	// Allocate memory for one row (4 bytes per pixel - RGBA)
	row = (png_bytep)malloc(4 * width * sizeof(png_byte));

	// Write image data
	int x, y;
	for (y = 0; y<height; y++) {
		for (x = 0; x<width; x++) {
			row[x * 4 + 0] = buffer[y*width + x].r;
			row[x * 4 + 1] = buffer[y*width + x].g;
			row[x * 4 + 2] = buffer[y*width + x].b;
			row[x * 4 + 3] = buffer[y*width + x].a;
		}
		png_write_row(png_ptr, row);
	}

	// End write
	png_write_end(png_ptr, NULL);

finalise:
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	if (row != NULL) free(row);

	return ret_code;
}

// Draw a FT_Bitmap to the target surface
// Dest: target - an array which stores image data (ARGB), row by row.
// Src: bitmap.buffer - 8bit grayscale image, row by row, with the size of bitmap.rows*bitmap.width
void draw_to_buffer(struct pixel_t *target, int target_width, FT_Bitmap bitmap, int x_pos, int y_pos) {
	int height = bitmap.rows;
	int width = bitmap.width;

	// The x, y here is based on the `bitmap` argument (i.e. the input bitmap)
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			struct pixel_t p;
			p.a = p.r = p.g = p.b = bitmap.buffer[x + y * width];
			target[(x_pos+x) + (y_pos+y) * target_width] = p;
		}
	}
}

// Center justify the subtitle
// The area of the subtitle is (0, valid_y) to (valid_w, valid_y + valid_h)
// NOTE: valid_x is assumed to be 0
// The function will move the subtitle to (target_w/2 + valid_w/2, valid_y),
//   and erase anything else in the line.
void center_justify(struct pixel_t *target, int target_w,
					int valid_y, int valid_w, int valid_h) {
	// 1. Make a copy of the line, to avoid overriding.
	// Note that the copy is smaller than the line:
	//   its width is set to just enough to contain the valid area
	struct pixel_t* temp_buffer = malloc(valid_w*valid_h*sizeof(struct pixel_t));

	// x,y here is input-based (i.e. `buffer`)
	for (int x = 0; x < valid_w; ++x) {
		for (int y = valid_y; y < valid_y + valid_h; ++y) {
			temp_buffer[x + (y - valid_y)*valid_w] = target[x + y * target_w];
		}
	}

	// 2. Clean up the original subtitle
	memset(target + valid_y * target_w, 0, target_w*valid_h * sizeof(struct pixel_t));

	// 3. Center justify
	int move_to = target_w / 2 - valid_w / 2; // the target x

	// x,y here is output-based (i.e. `buffer`)
	// valid_y is also output_y since we are printing to the same line.
	for (int y = valid_y; y < valid_y + valid_h; ++y) {
		for (int x = move_to; x < move_to + valid_w; ++x) {
			target[x + y * target_w] = temp_buffer[(x - move_to) + (y - valid_y)*valid_w];
		}
	}

	// 4. Clean up
	free(temp_buffer);
}

// Draw black background for the subtitle
// Will not override the text
// target_w is the width of the canvas
void black_background(struct pixel_t *target, int target_w, int x, int y, int w, int h) {
	for (int _x = x; _x < x + w; ++_x) {
		for (int _y = y; _y < y + h; ++_y) {
			target[_x + _y * target_w].a = 255;
		}
	}
}

// Convert big-endian and little-endian
#define BigtoLittle32(A)   ((( (uint32_t)(A) & 0xff000000) >> 24)\
 | (((uint32_t)(A) & 0x00ff0000) >> 8)\
 | (((uint32_t)(A) & 0x0000ff00) << 8)\
 | (((uint32_t)(A) & 0x000000ff) << 24))

// Generate PNG file from an UTF-32 string (str)
// Convert first if it is UTF-8 string!
// PNG file will be stored at output
// Return 1 on success.
int spupng_export_string2png(struct spupng_t *sp, uint32_t *str, FILE* output)
{
	// Init FreeType if it hasn't been inited yet.
	if (ft_library==NULL){
		int error;
		if (error = FT_Init_FreeType(&ft_library))
		{
			mprint("\nFailed to init freetype, error code: %d\n", error);
			return 0;
		}
		if (error = FT_New_Face(ft_library, ccx_options.enc_cfg.render_font, 0, &face))
		{
			mprint("\n\nFailed to init freetype when trying to init face, error code: %d\n", error);
			mprint("It's usually caused by the specified font is not found: %s \n", ccx_options.enc_cfg.render_font);
			mprint("If this is the case, please use -font to manually specify a font that exists. \n\n");
			return 0;
		}
		if (error = FT_Set_Pixel_Sizes(face, 0, FONT_SIZE))
		{
			mprint("\nFailed to init freetype when trying to set size, error code: %d\n", error);
			return 0;
		}
	}

	int canvas_width = CANVAS_WIDTH;
	int canvas_height = FONT_SIZE * 3.5;
	int line_height = FONT_SIZE * 1.0;
	int extender = FONT_SIZE * 0.2; // to prevent characters like $ (exceed baseline) from being cut
	int line_spacing = FONT_SIZE * 0.3;

	int cursor_x = 0;
	int cursor_y = line_height * 2;

	// Allocate buffer
	// Note: (0, 0) of buffer is at the top left corner.
	struct pixel_t *buffer = malloc(canvas_width * canvas_height * sizeof(struct pixel_t));
	if (buffer == NULL) {
		mprint("\nFailed to alloc memory for buffer. Need %d bytes.\n",
			canvas_width * canvas_height * sizeof(struct pixel_t));
	}
	memset(buffer, 0, canvas_width*canvas_height * sizeof(struct pixel_t));
	
	FT_GlyphSlot slot = face->glyph;

	// Render characters to image
	for (uint32_t *iter = str; *iter; ++iter)
	{
		uint32_t current_char_code = BigtoLittle32(*iter); // Convert big-endian and little-endian

		if (FT_Load_Char(face, current_char_code, FT_LOAD_RENDER)) continue; // ignore errors
		unsigned char* bitmap = slot->bitmap.buffer;

		// Handle '\n'
		if (current_char_code == '\n') {
//			mprint("\n'\\n' line break");
			black_background(buffer, canvas_width, 0, cursor_y - line_height - extender, cursor_x, line_height + extender * 2);
			center_justify(buffer, canvas_width, cursor_y - line_height - extender, cursor_x, line_height + extender * 2);
			cursor_x = 0;
			cursor_y += line_height + line_spacing;
			continue;
		}

		// Expand canvas if needed
		while (cursor_y - slot->bitmap_top + line_height + line_spacing + extender * 2 >= canvas_height) {
			int old_height = canvas_height;
			canvas_height += line_height + line_spacing + extender * 2;
			struct pixel_t* new_buffer = realloc(buffer, canvas_width * canvas_height * sizeof(struct pixel_t));
			if (new_buffer == NULL) {
				mprint("\nFailed to alloc memory for buffer. Need %d bytes.\n",
					canvas_width * canvas_height * sizeof(struct pixel_t));
				return 0;
			}
			memset(new_buffer + old_height * canvas_width, 0, (canvas_height-old_height) * canvas_width * sizeof(struct pixel_t));
			buffer = new_buffer;
			
		}

		// Characters such as ' ' don't have bitmap.
		if (bitmap != NULL) {

			int width = slot->bitmap.width;
			int height = slot->bitmap.rows;

			// TODO: this kind of line break may break characters in the middle!
			if ((cursor_x + (slot->advance.x >> 6)) > canvas_width) { // Time for a line-break!
				// But before that, let's center justify the subtitle.
				// Valid subtitle area: (0, cursor_y) to (cursor_x, cursor_y + line_height)
				black_background(buffer, canvas_width, 0, cursor_y - line_height - extender, cursor_x, line_height + extender * 2);
				center_justify(buffer, canvas_width, cursor_y - line_height - extender, cursor_x, line_height + extender * 2);
				
				// Set the cursor
				cursor_x = 0;
				cursor_y += line_height + line_spacing;
			}
			
			draw_to_buffer(buffer, canvas_width, slot->bitmap, cursor_x, cursor_y - slot->bitmap_top);

		}

		/*
		mprint("\nDrawing [%c] (%d), advance %d,%d, at %d,%d. bitmap_top=%d",
			current_char_code, current_char_code, slot->advance.x >> 6, slot->advance.y >> 6, cursor_x, cursor_y, slot->bitmap_top);
		*/

		// Increase pen position
		cursor_x += slot->advance.x >> 6;
		cursor_y += slot->advance.y >> 6;
	}
	// Draw black background
	black_background(buffer, canvas_width, 0, cursor_y - line_height - extender, cursor_x, line_height + extender * 2);
	// Center justify the last line.
	center_justify(buffer, canvas_width, cursor_y - line_height - extender, cursor_x, line_height + extender * 2);

	/*
	// Draw a blue bar at the beginning of every line.
	// Debug purpose only
	for (int x = 0; x < 10; x++) for (int y = 0; y < canvas_height; y++) {
		struct pixel_t p; p.b = (y / line_height) * 25; p.a = 255; p.g = p.r = 0;
		buffer[x + y * canvas_width] = p;
	}
	*/

	// Save image
	write_image(buffer, output, canvas_width, canvas_height);

	free(buffer);
	return 1;
}

// Convert EIA608 Data(buffer) to string
// out must have at least 256 characters' space
// Return value is the length of the output string
int eia608_to_str(struct encoder_ctx *context, struct eia608_screen *data, char *out) {

	int str_len = 0;
	int first = 1;
	for (int row = 0; row < ROWS; row++)
	{
		if (data->row_used[row])
		{
			size_t len = get_decoder_line_encoded(context, context->subline, row, data);

			unsigned char* start = context->subline;

			if (start!=NULL) {
				// Remove the space at the beginning of the subtitle
				while ((*start == ' ' || *start == '\n') && len-->0)
					start++;
			}

			// Check for characters that spumux won't parse
			// null chars will be changed to space
			// pairs of dashes will be changed to underscores
			for (unsigned char* ptr = start; ptr < start + len; ptr++)
			{
				switch (*ptr)
				{
				case 0:
					*ptr = ' ';
					break;
				case '-':
					if (*(ptr + 1) == '-')
					{
						*ptr++ = '_';
						*ptr = '_';
					}
					break;
				}
			}
			
			// Remove the space at the end of the subtitle
			if (start != NULL) {
				unsigned char *end = start;
				while (*end && end - start < len && end - start >= 0) end++; // Find the end
				while ((*end==' '||*end=='\n'||*end=='\0')&&len-->0) end--; // `len` is changed
				len++;
			}

			if (!first) { // Add '\n' if it is not the first line.
				strcat(out, "\n");
				str_len += 2;
			}
			first = 0;
			strncat(out, start, len);
			str_len += len;
		}
	}
	return str_len;
}

// The function will NOT free src.
// You need to free the src and return value yourself!
uint32_t* utf8_to_utf32(char* src) {
	// Convert UTF-8 to UTF-32 for generating bitmap.
	size_t len_src, len_dst;

	len_src = strlen(src);
	len_dst = (len_src + 2) * 4; // one for FEFF and one for \0

	uint32_t* string_utf32 = (uint32_t *) calloc(len_dst, 1);
	size_t inbufbytesleft = len_src;
	size_t outbufbytesleft = len_dst;
	char* inbuf = src;
	char* outbuf = (char*) string_utf32;

	iconv_t cd = iconv_open("UTF-32", "UTF-8");
	int result = iconv(cd, &inbuf, &inbufbytesleft, &outbuf, &outbufbytesleft);
	if (result == -1) mprint("iconv failed to convert UTF-8 to UTF-32. errno is %d\n", errno);
	iconv_close(cd);

	return string_utf32;
}

// string needs to be in UTF-8 encoding.
// This function will take care of encoding.
int spupng_write_string(struct spupng_t *sp, char *string, LLONG start_time, LLONG end_time,
	struct encoder_ctx *context)
{
	LLONG ms_start = start_time + context->subs_delay;
	if (ms_start < 0)
	{
		dbg_print(CCX_DMT_VERBOSE, "Negative start\n");
		return 0;
	}

	LLONG ms_end = end_time + context->subs_delay;

	inc_spupng_fileindex(sp);
	if ((sp->fppng = fopen(sp->pngfile, "wb")) == NULL)
	{
		fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Cannot open %s: %s\n",
			sp->pngfile, strerror(errno));
	}

	uint32_t* string_utf32 = utf8_to_utf32(string);
	if (!spupng_export_string2png(sp, string_utf32, sp->fppng))
	{
		free(string_utf32);
		fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Cannot write %s: %s\n",
			sp->pngfile, strerror(errno));
	}
	free(string_utf32);
	fclose(sp->fppng);
	write_sputag_open(sp, ms_start, ms_end);
	write_spucomment(sp, string);
	write_sputag_close(sp);
	return 1;
}

int write_cc_subtitle_as_spupng(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	struct spupng_t *sp = (struct spupng_t *) context->out->spupng_data;
	if (!sp)
		return -1;

	if (sub->type == CC_TEXT)
	{
		spupng_write_string(sp, sub->data, sub->start_time, sub->end_time, context);
	}

	return 0;
}

int spupng_write_ccbuffer(struct spupng_t *sp, struct eia608_screen* data,
	struct encoder_ctx *context)
{

	int row;
	int empty_buf = 1;
	char str[512] = "";

	// Check if it has negative start.
	LLONG ms_start = data->start_time + context->subs_delay;
	if (ms_start < 0)
	{
		dbg_print(CCX_DMT_VERBOSE, "Negative start\n");
		return 0;
	}

	// Check if it is blank page.
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

	eia608_to_str(context, data, str);

	return spupng_write_string(context->out->spupng_data, str, data->start_time, data->end_time, context);
}

int write_cc_buffer_as_spupng(struct eia608_screen *data, struct encoder_ctx *context)
{
	struct spupng_t *sp = (struct spupng_t *) context->out->spupng_data;
	if (NULL != sp)
	{
		return spupng_write_ccbuffer(sp, data, context);
	}
	return 0;
}
