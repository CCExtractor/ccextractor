#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_HARDSUBX
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include "allheaders.h"
#include "hardsubx.h"

char *get_ocr_text_simple(struct lib_hardsubx_ctx *ctx, PIX *image)
{
	char *text_out;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}

	if((text_out = TessBaseAPIGetUTF8Text(ctx->tess_handle)) == NULL)
	{
		//TODO: Display error message
		printf("Error getting text\n");
	}
	return text_out;
}

char *get_ocr_text_wordwise(struct lib_hardsubx_ctx *ctx, PIX *image)
{
	char *text_out;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}


	return text_out;
}

char *get_ocr_text_letterwise(struct lib_hardsubx_ctx *ctx, PIX *image)
{
	char *text_out;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}


	return text_out;
}

char *get_ocr_text_simple_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold)
{
	char *text_out;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}


	return text_out;
}

char *get_ocr_text_wordwise_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold)
{
	char *text_out;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}


	return text_out;
}

char *get_ocr_text_letterwise_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold)
{
	char *text_out;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}


	return text_out;
}

#endif
