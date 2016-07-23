#ifndef OCR_H
#define OCR_H
#include <png.h>

struct image_copy //A copy of the original OCR image, used for color detection
{
	int nb_colors;
	png_color *palette;
	png_byte *alpha;
	unsigned char *data;
	int bgcolor;
};

void delete_ocr (void** arg);
void* init_ocr(int lang_index);
char* ocr_bitmap(void* arg, png_color *palette,png_byte *alpha, unsigned char* indata,int w, int h, struct image_copy *copy);
int ocr_rect(void* arg, struct cc_bitmap *rect, char **str, int bgcolor);
char *paraof_ocrtext(struct cc_subtitle *sub, const char *crlf, unsigned crlf_length);

#endif
