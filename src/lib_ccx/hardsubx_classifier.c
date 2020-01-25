#include "lib_ccx.h"
#include "utility.h"

#ifdef ENABLE_HARDSUBX
//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <leptonica/allheaders.h>
#include "hardsubx.h"

char *get_ocr_text_simple(struct lib_hardsubx_ctx *ctx, PIX *image)
{
	char *text_out = NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if (TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{
		mprint("Error in Tesseract recognition, skipping frame\n");
		return NULL;
	}

	if ((text_out = TessBaseAPIGetUTF8Text(ctx->tess_handle)) == NULL)
	{
		mprint("Error getting text, skipping frame\n");
	}
	return text_out;
}

char *get_ocr_text_wordwise(struct lib_hardsubx_ctx *ctx, PIX *image)
{
	char *text_out = NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if (TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{
		mprint("Error in Tesseract recognition, skipping word\n");
		return NULL;
	}

	TessResultIterator *it = TessBaseAPIGetIterator(ctx->tess_handle);
	TessPageIteratorLevel level = RIL_WORD;

	int prev_ital = 0;

	if (it != 0)
	{
		do
		{
			char *word;
			char *ts_word = TessResultIteratorGetUTF8Text(it, level);
			if (ts_word == NULL || strlen(ts_word) == 0)
				continue;
			word = strdup(ts_word);
			TessDeleteText(ts_word);
			if (text_out == NULL)
			{
				if (ctx->detect_italics)
				{
					int italic = 0;
					int dummy = 0;
					TessResultIteratorWordFontAttributes(it, &dummy, &italic, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);
					if (italic == 1 && prev_ital == 0)
					{
						char *word_copy = strdup(word);
						word = realloc(word, strlen(word) + strlen("<i>") + 2);
						strcpy(word, "<i>");
						strcat(word, word_copy);
						free(word_copy);
						prev_ital = 1;
					}
					else if (italic == 0 && prev_ital == 1)
					{
						word = realloc(word, strlen(word) + strlen("</i>") + 2);
						strcat(word, "</i>");
						prev_ital = 0;
					}
				}
				text_out = strdup(word);
				text_out = realloc(text_out, strlen(text_out) + 2);
				strcat(text_out, " ");
				continue;
			}
			if (ctx->detect_italics)
			{
				int italic = 0;
				int dummy = 0;
				TessResultIteratorWordFontAttributes(it, &dummy, &italic, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);
				if (italic == 1 && prev_ital == 0)
				{
					char *word_copy = strdup(word);
					word = realloc(word, strlen(word) + strlen("<i>") + 2);
					strcpy(word, "<i>");
					strcat(word, word_copy);
					free(word_copy);
					prev_ital = 1;
				}
				else if (italic == 0 && prev_ital == 1)
				{
					word = realloc(word, strlen(word) + strlen("</i>") + 2);
					strcat(word, "</i>");
					prev_ital = 0;
				}
			}
			text_out = realloc(text_out, strlen(text_out) + strlen(word) + 2);
			strcat(text_out, word);
			strcat(text_out, " ");
			free(word);
		} while (TessPageIteratorNext((TessPageIterator *)it, level));
	}

	if (ctx->detect_italics && prev_ital == 1)
	{
		text_out = realloc(text_out, strlen(text_out) + strlen("</i>") + 2);
		strcat(text_out, "</i>");
	}

	TessResultIteratorDelete(it);

	return text_out;
}

char *get_ocr_text_letterwise(struct lib_hardsubx_ctx *ctx, PIX *image)
{
	char *text_out = NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if (TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{
		mprint("Error in Tesseract recognition, skipping symbol\n");
		return NULL;
	}

	TessResultIterator *it = TessBaseAPIGetIterator(ctx->tess_handle);
	TessPageIteratorLevel level = RIL_SYMBOL;

	if (it != 0)
	{
		do
		{
			char *letter;
			char *ts_letter = TessResultIteratorGetUTF8Text(it, level);
			if (ts_letter == NULL || strlen(ts_letter) == 0)
				continue;
			letter = strdup(ts_letter);
			TessDeleteText(ts_letter);
			if (text_out == NULL)
			{
				text_out = strdup(letter);
				continue;
			}
			text_out = realloc(text_out, strlen(text_out) + strlen(letter) + 1);
			strcat(text_out, letter);
			free(letter);
		} while (TessPageIteratorNext((TessPageIterator *)it, level));
	}

	TessResultIteratorDelete(it);

	return text_out;
}

char *get_ocr_text_simple_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold)
{
	char *text_out = NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if (TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{
		mprint("Error in Tesseract recognition, skipping frame\n");
		return NULL;
	}

	if ((text_out = TessBaseAPIGetUTF8Text(ctx->tess_handle)) == NULL)
	{
		mprint("Error getting text, skipping frame\n");
	}

	int conf = TessBaseAPIMeanTextConf(ctx->tess_handle);

	if (conf < threshold)
		return NULL;

	ctx->cur_conf = (float)conf;

	return text_out;
}

char *get_ocr_text_wordwise_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold)
{
	char *text_out = NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if (TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{
		mprint("Error in Tesseract recognition, skipping word\n");
		return NULL;
	}

	TessResultIterator *it = TessBaseAPIGetIterator(ctx->tess_handle);
	TessPageIteratorLevel level = RIL_WORD;

	int prev_ital = 0;
	float total_conf = 0.0;
	int num_words = 0;

	if (it != 0)
	{
		do
		{
			char *word;
			char *ts_word = TessResultIteratorGetUTF8Text(it, level);
			if (ts_word == NULL || strlen(ts_word) == 0)
				continue;
			word = strdup(ts_word);
			TessDeleteText(ts_word);
			float conf = TessResultIteratorConfidence(it, level);
			if (conf < threshold)
				continue;
			total_conf += conf;
			num_words++;
			if (text_out == NULL)
			{
				if (ctx->detect_italics)
				{
					int italic = 0;
					int dummy = 0;
					TessResultIteratorWordFontAttributes(it, &dummy, &italic, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);
					if (italic == 1 && prev_ital == 0)
					{
						char *word_copy = strdup(word);
						word = realloc(word, strlen(word) + strlen("<i>") + 2);
						strcpy(word, "<i>");
						strcat(word, word_copy);
						free(word_copy);
						prev_ital = 1;
					}
					else if (italic == 0 && prev_ital == 1)
					{
						word = realloc(word, strlen(word) + strlen("</i>") + 2);
						strcat(word, "</i>");
						prev_ital = 0;
					}
				}
				text_out = strdup(word);
				text_out = realloc(text_out, strlen(text_out) + 2);
				strcat(text_out, " ");
				continue;
			}
			if (ctx->detect_italics)
			{
				int italic = 0;
				int dummy = 0;
				TessResultIteratorWordFontAttributes(it, &dummy, &italic, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);
				if (italic == 1 && prev_ital == 0)
				{
					char *word_copy = strdup(word);
					word = realloc(word, strlen(word) + strlen("<i>") + 2);
					strcpy(word, "<i>");
					strcat(word, word_copy);
					free(word_copy);
					prev_ital = 1;
				}
				else if (italic == 0 && prev_ital == 1)
				{
					word = realloc(word, strlen(word) + strlen("</i>") + 2);
					strcat(word, "</i>");
					prev_ital = 0;
				}
			}
			text_out = realloc(text_out, strlen(text_out) + strlen(word) + 2);
			strcat(text_out, word);
			strcat(text_out, " ");
			free(word);
		} while (TessPageIteratorNext((TessPageIterator *)it, level));
	}

	if (ctx->detect_italics && prev_ital == 1)
	{
		text_out = realloc(text_out, strlen(text_out) + strlen("</i>") + 2);
		strcat(text_out, "</i>");
	}

	if (num_words > 0)
		total_conf = total_conf / num_words;
	ctx->cur_conf = total_conf;

	TessResultIteratorDelete(it);

	return text_out;
}

char *get_ocr_text_letterwise_threshold(struct lib_hardsubx_ctx *ctx, PIX *image, float threshold)
{
	char *text_out = NULL;

	TessBaseAPISetImage2(ctx->tess_handle, image);
	if (TessBaseAPIRecognize(ctx->tess_handle, NULL) != 0)
	{
		mprint("Error in Tesseract recognition, skipping symbol\n");
		return NULL;
	}

	TessResultIterator *it = TessBaseAPIGetIterator(ctx->tess_handle);
	TessPageIteratorLevel level = RIL_SYMBOL;

	float total_conf = 0.0;
	int num_words = 0;

	if (it != 0)
	{
		do
		{
			char *letter;
			char *ts_letter = TessResultIteratorGetUTF8Text(it, level);
			if (ts_letter == NULL || strlen(ts_letter) == 0)
				continue;
			letter = strdup(ts_letter);
			TessDeleteText(ts_letter);
			float conf = TessResultIteratorConfidence(it, level);
			if (conf < threshold)
				continue;
			total_conf += conf;
			num_words++;
			if (text_out == NULL)
			{
				text_out = strdup(letter);
				continue;
			}
			text_out = realloc(text_out, strlen(text_out) + strlen(letter) + 1);
			strcat(text_out, letter);
			free(letter);
		} while (TessPageIteratorNext((TessPageIterator *)it, level));
	}

	if (num_words > 0)
		total_conf = total_conf / num_words;
	ctx->cur_conf = total_conf;

	TessResultIteratorDelete(it);

	return text_out;
}

#endif
