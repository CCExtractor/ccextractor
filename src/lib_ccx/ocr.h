#ifndef OCR_H
#define OCR_H
#include <png.h>

void delete_ocr (void** arg);
void* init_ocr(int lang_index);
char* ocr_bitmap(void* arg, png_color *palette,png_byte *alpha, unsigned char* indata,int w, int h);
int ocr_rect(void* arg, struct cc_bitmap *rect, char **str);
char *paraof_ocrtext(struct cc_subtitle *sub);

#endif
