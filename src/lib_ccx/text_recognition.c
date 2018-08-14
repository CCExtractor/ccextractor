#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*Text Recognition (OCR)*/

typedef struct OCRTesseract
{
	TessBaseAPI tess;
};

TessBaseAPI* init_OCRTesseract(const char* datapath, const char* language, const char* char_whitelist, int oemode, int psmode)
{
	TessBaseAPI* tess = malloc(sizeof(TessBaseAPI));
	const char *lang = "eng";
	if(language != NULL)
        lang = language;

    char* pars_vec = strdup("debug_file");
    char* pars_values = strdup("/dev/null");

    int ret = TessBaseAPIInit4(tess, NULL, ocrlang, oemode, NULL, 0, &pars_vec,
            &pars_values, 1, false);
    if(ret != 0);
    {
        mprint("Failed loading language: %s, trying to load eng\n", lang);
        ret = TessBaseAPIInit4(tess, NULL, "eng", ccx_options.ocr_oem, NULL, 0, &pars_vec,
            &pars_values, 1, false);
    }

    TessPageSegMode pagesegmode = (TessPageSegMode)psmode;
    TessBaseAPISetPageSegMode(tess, pagesegmode);

    if(char_whitelist != NULL)
		TessBaseAPISetVariable(tess, "tessedit_char_whitelist", char_whitelist);
    else
        TessBaseAPISetVariable(tess, "tessedit_char_whitelist", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

    TessBaseAPISetVariable(tess, "save_best_choices", "T");
    return tess;
}


/** @brief Recognize text using the tesseract-ocr API.

    Takes image on input and returns recognized text in the output_text parameter. Optionally
    provides also the Rects for individual text elements found (e.g. words), and the list of those
    text elements with their confidence values.

    @param image Input
    @param output_text Output text of the tesseract-ocr.
    @param component_rects If provided the method will output a list of Rects for the individual
    text elements found (e.g. words or text lines).
    @param component_texts If provided the method will output a list of text strings for the
    recognition of individual text elements found (e.g. words or text lines).
    @param component_confidences If provided the method will output a list of confidence values
    for the recognition of individual text elements found (e.g. words or text lines).
    @param component_level OCR_LEVEL_WORD (by default), or OCR_LEVEL_TEXTLINE.
     */
void run_ocr(TessBaseAPI* tess, Mat image, char* output_text, int component_level/*0*/)
{
	TessBaseAPISetImage(tess, (unsigned char*)image.data, image.cols, image.rows, channels(image), (image.step[0])/elemSize(image));
	TessBaseAPIRecognize(tess, 0);
	
	output_text = TessBaseAPIGetUTF8Text(tess);
    TessResultIterator* ri = TessBaseAPIGetIterator(tess);
    TessPageIteratorLevel level = component_level;
    if(ri != 0)
    {
        do
        {
            const char* word = TessResultIteratorGetUTF8Text(tess, level);
            if(word == NULL)
                continue;
            float conf = TessResultIteratorConfidence(ri, level);
            int x1, y1, x2, y2;
            TessPageIteratorBoundingBox(ri, level, &x1, &y1, &x2, &y2);

        } while(TessPageIteratorNext(tess, level));
        TessResultIteratorDelete(ri);
    }
    TessBaseAPIClear(tess);
}