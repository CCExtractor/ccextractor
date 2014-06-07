#ifndef OCR_H
#define OCR_H
#include <png.h>
char* ocr_bitmap(png_color *palette,png_byte *alpha, unsigned char* indata,int w, int h);

#endif
