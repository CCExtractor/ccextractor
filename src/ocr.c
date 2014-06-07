#include "png.h"
#ifdef ENABLE_OCR
#include "capi.h"
#include "allheaders.h"

char* ocr_bitmap(png_color *palette,png_byte *alpha, unsigned char* indata,int w, int h)
{
	TessBaseAPI* api;
	PIX	*pix;
	char*text_out= NULL;
	int i,j,index,ret;
	unsigned int wpl;
	unsigned int *data,*ppixel;
	api = TessBaseAPICreate();

	pix = pixCreate(w, h, 32);
	if(pix == NULL)
	{
		return NULL;
	}
	wpl = pixGetWpl(pix);
	data = pixGetData(pix);
	pixSetSpp(pix, 4);
	for (i = 0; i < h; i++)
	{
		ppixel = data + i * wpl; 
		for (j = 0; j < w; j++)
		{
			index = indata[i * w + (j)];
			composeRGBPixel(palette[index].red, palette[index].green,palette[index].blue, ppixel);
			SET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL,alpha[index]);
			ppixel++;
		}    
	}    

	ret = TessBaseAPIInit3(api,"", "eng");
	if(ret < 0)
	{
		return NULL;
	}

	//text_out = TessBaseAPIProcessPages(api, "/home/anshul/test_videos/dvbsubtest.d/sub0018.png", 0, 0);
	text_out = TessBaseAPIProcessPage(api, pix, 0, NULL, NULL, 1000);
	if(!text_out)
		printf("\nsomething messy\n");
	return text_out;
}
#else
char* ocr_bitmap(png_color *palette,png_byte *alpha, unsigned char* indata,unsigned char d,int w, int h)
{
	mprint("ocr not supported without tesseract\n");
	return NULL;
}
#endif
