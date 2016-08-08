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
	char *text_out=NULL;

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
	char *text_out=NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}

	TessResultIterator *it = TessBaseAPIGetIterator(ctx->tess_handle);
	TessPageIteratorLevel level = RIL_WORD;

	if(it!=0)
	{
		do
		{
			char* word = TessResultIteratorGetUTF8Text(it, level);
			if(word==NULL || strlen(word)==0)
				continue;
			if(text_out == NULL)
			{
				text_out = strdup(word);
				text_out = realloc(text_out, strlen(text_out)+2);
				strcat(text_out, " ");
				continue;
			}
			text_out = realloc(text_out, strlen(text_out)+strlen(word)+2);
			strcat(text_out, word);
			strcat(text_out, " ");
			free(word);
		} while(TessPageIteratorNext((TessPageIterator *)it, level));
	}

	TessResultIteratorDelete(it);

	return text_out;
}

char *get_ocr_text_letterwise(struct lib_hardsubx_ctx *ctx, PIX *image)
{
	char *text_out=NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}

	TessResultIterator *it = TessBaseAPIGetIterator(ctx->tess_handle);
	TessPageIteratorLevel level = RIL_SYMBOL;

	if(it!=0)
	{
		do
		{
			char* letter = TessResultIteratorGetUTF8Text(it, level);
			if(letter==NULL || strlen(letter)==0)
				continue;
			if(text_out==NULL)
			{
				text_out = strdup(letter);
				continue;
			}
			text_out = realloc(text_out, strlen(text_out) + strlen(letter) + 1);
			strcat(text_out, letter);
			free(letter);
		} while(TessPageIteratorNext((TessPageIterator *)it, level));
	}

	TessResultIteratorDelete(it);

	return text_out;
}

char *get_ocr_text_simple_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold)
{
	char *text_out=NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}

	int conf = TessBaseAPIMeanTextConf(ctx->tess_handle);

	return text_out;
}

char *get_ocr_text_wordwise_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold)
{
	char *text_out=NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}

	TessResultIterator *it = TessBaseAPIGetIterator(ctx->tess_handle);
	TessPageIteratorLevel level = RIL_WORD;

	if(it!=0)
	{
		do
		{
			char* word = TessResultIteratorGetUTF8Text(it, level);
			if(word==NULL || strlen(word)==0)
				continue;
			float conf = TessResultIteratorConfidence(it,level);
			if(conf < threshold)
				continue;
			if(text_out == NULL)
			{
				text_out = strdup(word);
				text_out = realloc(text_out, strlen(text_out)+2);
				strcat(text_out, " ");
				continue;
			}
			text_out = realloc(text_out, strlen(text_out)+strlen(word)+2);
			strcat(text_out, word);
			strcat(text_out, " ");
			free(word);
		} while(TessPageIteratorNext((TessPageIterator *)it, level));
	}

	TessResultIteratorDelete(it);

	return text_out;
}

char *get_ocr_text_letterwise_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold)
{
	char *text_out=NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if(TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{	
		//TODO: Display error message
		printf("Error in Tesseract recognition\n");
		return NULL;
	}

	TessResultIterator *it = TessBaseAPIGetIterator(ctx->tess_handle);
	TessPageIteratorLevel level = RIL_SYMBOL;

	if(it!=0)
	{
		do
		{
			char* letter = TessResultIteratorGetUTF8Text(it, level);
			if(letter==NULL || strlen(letter)==0)
				continue;
			float conf = TessResultIteratorConfidence(it,level);
			if(text_out==NULL)
			{
				text_out = strdup(letter);
				continue;
			}
			text_out = realloc(text_out, strlen(text_out) + strlen(letter) + 1);
			strcat(text_out, letter);
			free(letter);
		} while(TessPageIteratorNext((TessPageIterator *)it, level));
	}

	TessResultIteratorDelete(it);

	return text_out;
}

#endif
