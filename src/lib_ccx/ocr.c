#include "png.h"
#include "lib_ccx.h"
#ifdef ENABLE_OCR
#include "platform.h"
#include "capi.h"
#include "ccx_common_constants.h"
#include "allheaders.h"
#include <dirent.h>
#include "spupng_encoder.h"

struct ocrCtx
{
	TessBaseAPI* api;
};

struct transIntensity
{
	uint8_t *t;
	png_color *palette;
};
static int check_trans_tn_intensity(const void *p1, const void *p2, void *arg)
{
	struct transIntensity *ti = arg;
	unsigned char* tmp = (unsigned char*)p1;
	unsigned char* act = (unsigned char*)p2;
	unsigned char tmp_i;
	unsigned char act_i;
	/** TODO verify that RGB follow ITU-R BT.709
	 *  Below fomula is valid only for 709 standurd
         *  Y = 0.2126 R + 0.7152 G + 0.0722 B
         */
	tmp_i = (0.2126 * ti->palette[*tmp].red) + (0.7152 * ti->palette[*tmp].green) + (0.0722 * ti->palette[*tmp].blue);
	act_i = (0.2126 * ti->palette[*act].red) + (0.7152 * ti->palette[*act].green) + (0.0722 * ti->palette[*act].blue);;

	if (ti->t[*tmp] < ti->t[*act] || (ti->t[*tmp] == ti->t[*act] &&  tmp_i < act_i))
		return -1;
	else if (ti->t[*tmp] == ti->t[*act] &&  tmp_i == act_i)
		return 0;


	return 1;
}

static int search_language_pack(const char *dirname,const char *lang)
{
	DIR *dp;
	struct dirent *dirp;
	char filename[256];
	if ((dp = opendir(dirname)) == NULL)
	{
		return -1;
	}
	snprintf(filename, 256, "%s.traineddata",lang);
	while ((dirp = readdir(dp)) != NULL)
	{
		if(!strcmp(dirp->d_name, filename))
		{
			closedir(dp);
			return 0;
		}
	}
	closedir(dp);
	return -1;
}

static void delete_ocr (struct ocrCtx* ctx)
{
        TessBaseAPIEnd(ctx->api);
        TessBaseAPIDelete(ctx->api);
	freep(&ctx);
}
void* init_ocr(int lang_index)
{
	int ret;
	struct ocrCtx* ctx;

	ctx = (struct ocrCtx*)malloc(sizeof(struct ocrCtx));
	if(!ctx)
		return NULL;
	ctx->api = TessBaseAPICreate();

	/* if language was undefined use english */
	if(lang_index == 0)
	{
		/* select english */
		lang_index = 1;
	}

	/* if langauge pack not found use english */
	ret = search_language_pack("tessdata",language[lang_index]);
	if(ret < 0 )
	{
		/* select english */
		lang_index = 1;
	}
	ret = TessBaseAPIInit3(ctx->api,"", language[lang_index]);
	if(ret < 0)
	{
		goto fail;
	}
	return ctx;
fail:
	delete_ocr(ctx);
	return NULL;

}
char* ocr_bitmap(void* arg, png_color *palette,png_byte *alpha, unsigned char* indata,int w, int h)
{
	PIX	*pix;
	char*text_out= NULL;
	int i,j,index;
	unsigned int wpl;
	unsigned int *data,*ppixel;
	struct ocrCtx* ctx = arg;
	pix = pixCreate(w, h, 32);
	if(pix == NULL)
	{
		return NULL;
	}
	wpl = pixGetWpl(pix);
	data = pixGetData(pix);
#if LEPTONICA_VERSION > 69
	pixSetSpp(pix, 4);
#endif
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

	text_out = TessBaseAPIProcessPage(ctx->api, pix, 0, NULL, NULL, 0);
	if(!text_out)
		printf("\nsomething messy\n");

        //TessDeleteText(text_out);
        pixDestroy(&pix);

	return text_out;
}
/*
 * @param alpha out
 * @param intensity in
 * @param palette out should be already initialized
 * @param bitmap in
 * @param size in size of bitmap
 * @param max_color in
 * @param nb_color in
 */
static int quantize_map(png_byte *alpha, png_color *palette,
		uint8_t *bitmap, int size, int max_color, int nb_color)
{
	/*
	 * occurrence of color in image
	 */
	uint32_t *histogram = NULL;
	/* intensity ordered table */
	uint8_t *iot = NULL;
	/* array of color with most occurrence according to histogram
	 * save index of intensity order table
	 */
	uint32_t *mcit = NULL;
	struct transIntensity ti = { alpha,palette};

	int ret = 0;

	histogram = (uint32_t*) malloc(nb_color * sizeof(uint32_t));
	if (!histogram)
	{
		ret = -1;
		goto end;
	}

	iot = (uint8_t*) malloc(nb_color * sizeof(uint8_t));
	if (!iot)
	{
		ret = -1;
		goto end;
	}

	mcit = (uint32_t*) malloc(nb_color * sizeof(uint32_t));
	if (!mcit)
	{
		ret = -1;
		goto end;
	}

	memset(histogram, 0, nb_color * sizeof(uint32_t));

	/* initializing intensity  ordered table with serial order of unsorted color table */
	for (int i = 0; i < nb_color; i++)
	{
		iot[i] = i;
	}
	memset(mcit, 0, nb_color * sizeof(uint32_t));

	/* calculate histogram of image */
	for (int i = 0; i < size; i++)
	{
		histogram[bitmap[i]]++;
	}
	/* sorted in increasing order of intensity */
	shell_sort((void*)iot, nb_color, sizeof(*iot), check_trans_tn_intensity, (void*)&ti);

#if OCR_DEBUG
	ccx_common_logging.log_ftn("Intensity ordered table\n");
	for (int i = 0; i < nb_color; i++)
	{
		ccx_common_logging.log_ftn("%02d) map %02d hist %02d\n",
			i, iot[i], histogram[iot[i]]);
	}
#endif
	/**
	 * using selection  sort since need to find only max_color
	 * Hostogram becomes invalid in this loop
	 */
	for (int i = 0; i < max_color; i++)
	{
		uint32_t max_val = 0;
		uint32_t max_ind = 0;
		int j;
		for (j = 0; j < nb_color; j++)
		{
			if (max_val < histogram[iot[j]])
			{
				max_val = histogram[iot[j]];
				max_ind = j;
			}
		}
		for (j = i; j > 0 && max_ind < mcit[j - 1]; j--)
		{
			mcit[j] = mcit[j - 1];
		}
		mcit[j] = max_ind;
		histogram[iot[max_ind]] = 0;
	}

#if OCR_DEBUG
	ccx_common_logging.log_ftn("max redundant  intensities table\n");
	for (int i = 0; i < max_color; i++)
	{
		ccx_common_logging.log_ftn("%02d) mcit %02d\n",
			i, mcit[i]);
	}
#endif
	for (int i = 0, mxi = 0; i < nb_color; i++)
	{
		int step, inc;
		if (i == mcit[mxi])
		{
			mxi = (mxi < max_color) ? mxi + 1 : mxi;
			continue;
		}
		inc = (mxi) ? -1 : 0;
		step = mcit[mxi + inc] + ((mcit[mxi] - mcit[mxi + inc]) / 2);
		if (i <= step)
		{
			int index = iot[mcit[mxi + inc]];
			alpha[iot[i]] = alpha[index];
			palette[iot[i]].red = palette[index].red;
			palette[iot[i]].blue = palette[index].blue;
			palette[iot[i]].green = palette[index].green;
		}
		else
		{
			int index = iot[mcit[mxi]];
			alpha[iot[i]] = alpha[index];
			palette[iot[i]].red = palette[index].red;
			palette[iot[i]].blue = palette[index].blue;
			palette[iot[i]].green = palette[index].green;
		}

	}
#if OCR_DEBUG
	ccx_common_logging.log_ftn("Colors present in quantized Image\n");
	for (int i = 0; i < nb_color; i++)
	{
		ccx_common_logging.log_ftn("%02d)r %03d g %03d b %03d a %03d\n",
			i, palette[i].red, palette[i].green, palette[i].blue, alpha[i]);
	}
#endif
	end: freep(&histogram);
	freep(&mcit);
	freep(&iot);
	return ret;
}

int ocr_rect(void* arg, struct cc_bitmap *rect, char **str)
{
	int ret = 0;
	png_color *palette = NULL;
	png_byte *alpha = NULL;

	palette = (png_color*) malloc(rect[0].nb_colors * sizeof(png_color));
	if(!palette)
	{
		ret = -1;
		goto end;
	}
        alpha = (png_byte*) malloc(rect[0].nb_colors * sizeof(png_byte));
        if(!alpha)
        {
                ret = -1;
                goto end;
        }

	/* TODO do rectangle wise, one color table should not be used for all rectangles */
        mapclut_paletee(palette, alpha, (uint32_t *)rect->data[1],rect->nb_colors);

        quantize_map(alpha, palette, rect->data[0], rect->w * rect->h, 3, rect->nb_colors);
        *str = ocr_bitmap(arg, palette, alpha, rect->data[0], rect->w, rect->h);
end:
	freep(&palette);
	freep(&alpha);
	return ret;

}
#else
char* ocr_bitmap(png_color *palette,png_byte *alpha, unsigned char* indata,unsigned char d,int w, int h)
{
	mprint("ocr not supported without tesseract\n");
	return NULL;
}
#endif
