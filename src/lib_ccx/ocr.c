#include "png.h"
#include "lib_ccx.h"
#ifdef ENABLE_OCR
#include "capi.h"
#include "ccx_common_constants.h"
#include "allheaders.h"
#include <dirent.h>
#include "spupng_encoder.h"
#include "ccx_encoders_helpers.h"
#undef OCR_DEBUG
struct ocrCtx
{
	TessBaseAPI* api;
};

struct transIntensity
{
	uint8_t *t;
	png_color *palette;
};

struct image_copy
{
	int nb_colors;
	png_color *palette;
	png_byte *alpha;
	unsigned char *data;
	int bgcolor;
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

void delete_ocr (void** arg)
{
	struct ocrCtx* ctx = *arg;
	TessBaseAPIEnd(ctx->api);
	TessBaseAPIDelete(ctx->api);
	freep(arg);
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

	ret = TessBaseAPIInit3(ctx->api, NULL, language[lang_index]);
	if(ret < 0)
	{
		goto fail;
	}
	return ctx;
fail:
	delete_ocr((void**)&ctx);
	return NULL;

}

BOX* ignore_alpha_at_edge(png_byte *alpha, unsigned char* indata, int w, int h, PIX *in, PIX **out)
{
	int i, j, index, start_y, end_y;
	int find_end_x = CCX_FALSE;
	BOX* cropWindow;
	for (j = 1; j < w-1; j++)
	{
		for (i = 0; i < h; i++)
		{
			index = indata[i * w + (j)];
			if(alpha[index] != 0)
			{
				if(find_end_x == CCX_FALSE)
				{
					start_y = j;
					find_end_x = CCX_TRUE;
				}
				else
				{
					end_y = j;
				}
			}
		}
	}
	cropWindow = boxCreate(start_y, 0, (w - (start_y + ( w - end_y) )), h - 1);
	*out = pixClipRectangle(in, cropWindow, NULL);
	//boxDestroy(&cropWindow);

	return cropWindow;
}
char* ocr_bitmap(void* arg, png_color *palette,png_byte *alpha, unsigned char* indata,int w, int h, struct image_copy *copy)
{
	PIX	*pix = NULL;
	PIX	*cpix = NULL;
	PIX *color_pix = NULL;
	PIX *color_pix_out = NULL;
	char*text_out= NULL;
	int i,j,index;
	unsigned int wpl;
	unsigned int *data,*ppixel;
	BOOL tess_ret = FALSE;
	struct ocrCtx* ctx = arg;
	pix = pixCreate(w, h, 32);
	color_pix = pixCreate(w, h, 32);
	if(pix == NULL||color_pix == NULL)
	{
		return NULL;
	}
	wpl = pixGetWpl(pix);
	data = pixGetData(pix);
#if LEPTONICA_VERSION > 69
	pixSetSpp(pix, 4);
	pixSetSpp(color_pix, 4);
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
	ignore_alpha_at_edge(alpha, indata, w, h, pix, &cpix);
	// For the unquantized bitmap
	wpl = pixGetWpl(color_pix);
	data = pixGetData(color_pix);
	for (i = 0; i < h; i++)
	{
		ppixel = data + i * wpl;
		for (j = 0; j < w; j++)
		{
			index = copy->data[i * w + (j)];
			composeRGBPixel(copy->palette[index].red, copy->palette[index].green,copy->palette[index].blue, ppixel);
			SET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL,copy->alpha[index]);
			ppixel++;
		}
	}
	BOX *crop_points = ignore_alpha_at_edge(copy->alpha, copy->data, w, h, color_pix, &color_pix_out);
// #ifdef OCR_DEBUG
	{
	char str[128] = "";
	static int i = 0;
	sprintf(str,"temp/file_c_%d.jpg",i);
	pixWrite(str, color_pix_out, IFF_JFIF_JPEG);
	i++;
	}
// #endif
	TessBaseAPISetImage2(ctx->api, cpix);
	tess_ret = TessBaseAPIRecognize(ctx->api, NULL);
	if( tess_ret != 0)
		printf("\nsomething messy\n");

	text_out = TessBaseAPIGetUTF8Text(ctx->api);

	TessBaseAPISetImage2(ctx->api, color_pix_out);
	tess_ret = TessBaseAPIRecognize(ctx->api, NULL);
	if( tess_ret != 0)
		printf("\nsomething messy\n");
	TessResultIterator* ri = TessBaseAPIGetIterator(ctx->api);
	TessPageIteratorLevel level = RIL_WORD;

	// printf("%d %d %d %d\n", crop_points->x,crop_points->y,crop_points->w,crop_points->h);
	if(ri!=0)
	{
		do
		{
			char* word = TessResultIteratorGetUTF8Text(ri,level);
			float conf = TessResultIteratorConfidence(ri,level);
			int x1, y1, x2, y2;
			TessPageIteratorBoundingBox(ri,level, &x1, &y1, &x2, &y2);
			printf("word: '%s';  \tconf: %.2f; BoundingBox: %d,%d,%d,%d;",word, conf, x1, y1, x2, y2);
			// printf("word: '%s';\n", word);
			// {
			// char str[128] = "";
			// static int i = 0;
			// sprintf(str,"temp/file_c_%d.jpg",i);
			// pixWrite(str, pixClipRectangle(color_pix_out, boxCreate(x1,y1,x2-x1,y2-y1) ,NULL), IFF_JFIF_JPEG);
			// i++;
			// }

			uint32_t *histogram = NULL;
			uint8_t *iot = NULL;
			uint32_t *mcit = NULL;
			int ret = 0;
			int max_color=2;

			histogram = (uint32_t*) malloc(copy->nb_colors * sizeof(uint32_t));
			iot = (uint8_t*) malloc(copy->nb_colors * sizeof(uint8_t));
			mcit = (uint32_t*) malloc(copy->nb_colors * sizeof(uint32_t));
			struct transIntensity ti = {copy->alpha,copy->palette};
			memset(histogram, 0, copy->nb_colors * sizeof(uint32_t));

			/* initializing intensity  ordered table with serial order of unsorted color table */
			for (int i = 0; i < copy->nb_colors; i++)
			{
				iot[i] = i;
			}
			memset(mcit, 0, copy->nb_colors * sizeof(uint32_t));

			/* calculate histogram of image */
			for(int i=y1;i<=y2;i++)
			{
				for(int j=x1;j<=x2;j++)
				{
					histogram[copy->data[(crop_points->y+i)*w + (crop_points->x+j)]]++;
				}
			}
			/* sorted in increasing order of intensity */
			shell_sort((void*)iot, copy->nb_colors, sizeof(*iot), check_trans_tn_intensity, (void*)&ti);
			// ccx_common_logging.log_ftn("Intensity ordered table\n");
			// for (int i = 0; i < copy->nb_colors; i++)
			// {
			// 	ccx_common_logging.log_ftn("%02d) map %02d hist %02d\n",
			// 		i, iot[i], histogram[iot[i]]);
			// }
			/**
			 * using selection  sort since need to find only max_color
			 * Hostogram becomes invalid in this loop
			 */
			for (int i = 0; i < max_color; i++)
			{
				uint32_t max_val = 0;
				uint32_t max_ind = 0;
				int j;
				for (j = 0; j < copy->nb_colors; j++)
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
			for (int i = 0; i < copy->nb_colors; i++)
			{
				palette[i].red = copy->palette[i].red;
				palette[i].green = copy->palette[i].green;
				palette[i].blue = copy->palette[i].blue;
				alpha[i]=copy->alpha[i];
			}
			// for (int i = 0; i < max_color; i++)
			// {
				// ccx_common_logging.log_ftn("%02d) mcit %02d\n",
				// 	i, mcit[i]);
				// printf("palette: %d %d %d\n", palette[mcit[i]].red,palette[mcit[i]].green,palette[mcit[i]].blue);
			// }
			for (int i = 0, mxi = 0; i < copy->nb_colors; i++)
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
			// #ifdef OCR_DEBUG
				// ccx_common_logging.log_ftn("Colors present in quantized Image\n");
				// for (int i = 0; i < copy->nb_colors; i++)
				// {
				// 	ccx_common_logging.log_ftn("%02d)r %03d g %03d b %03d a %03d\n",
				// 		i, palette[i].red, palette[i].green, palette[i].blue, alpha[i]);
				// }
			// #endif
				int r_avg=0,g_avg=0,b_avg=0,denom=0;
				for (int i = 0; i < copy->nb_colors; i++)
				{
					if(palette[i].red == 0 && palette[i].green == 0 && palette[i].blue == 0)
						continue;
					denom++;
					r_avg+=palette[i].red;
					g_avg+=palette[i].green;
					b_avg+=palette[i].blue;
				}
				if(denom!=0)
				{
					r_avg/=denom;
					g_avg/=denom;
					b_avg/=denom;
				}
				if(r_avg==0&&b_avg==0&&g_avg==0)
					exit(0);
				printf("\tColor: '%d %d %d';\n", r_avg, g_avg, b_avg);
			
		} while (TessResultIteratorNext(ri,level));
	}

	pixDestroy(&pix);
	pixDestroy(&cpix);
	pixDestroy(&color_pix);
	pixDestroy(&color_pix_out);

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

#ifdef OCR_DEBUG
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

#ifdef OCR_DEBUG
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
#ifdef OCR_DEBUG
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

int ocr_rect(void* arg, struct cc_bitmap *rect, char **str, int bgcolor)
{
	int ret = 0;
	png_color *palette = NULL;
	png_byte *alpha = NULL;
	
	struct image_copy *copy;
	copy = (struct image_copy *)malloc(sizeof(struct image_copy));
	copy->nb_colors = rect->nb_colors;
	copy->palette = (png_color*) malloc(rect->nb_colors * sizeof(png_color));
	copy->alpha = (png_byte*) malloc(rect->nb_colors * sizeof(png_byte));
	copy->bgcolor = bgcolor;

	palette = (png_color*) malloc(rect->nb_colors * sizeof(png_color));
	if(!palette||!copy->palette)
	{
		ret = -1;
		goto end;
	}
		alpha = (png_byte*) malloc(rect->nb_colors * sizeof(png_byte));
		if(!alpha||!copy->alpha)
		{
				ret = -1;
				goto end;
		}

		mapclut_paletee(palette, alpha, (uint32_t *)rect->data[1],rect->nb_colors);
		mapclut_paletee(copy->palette, copy->alpha, (uint32_t *)rect->data[1],rect->nb_colors);

		int size = rect->w * rect->h;
		copy->data = (unsigned char *)malloc(sizeof(unsigned char)*size);
		for(int i = 0; i < size; i++)
		{
			copy->data[i] = rect->data[0][i];
		}

		quantize_map(alpha, palette, rect->data[0], size, 3, rect->nb_colors);
		*str = ocr_bitmap(arg, palette, alpha, rect->data[0], rect->w, rect->h, copy);

end:
	freep(&palette);
	freep(&alpha);
	freep(&copy->palette);
	freep(&copy->alpha);
	freep(&copy->data);
	freep(&copy);
	return ret;

}

/**
 * Call back function used while sorting rectangle by y position
 * if both rectangle have same y position then x position is considered
 */
int compare_rect_by_ypos(const void*p1, const void *p2, void*arg)
{
	const struct cc_bitmap* r1 = p1;
	const struct cc_bitmap* r2 = p2;
	if(r1->y > r2->y)
	{
		return 1;
	}
	else if (r1->y == r2->y)
	{
		if(r1->x > r2->x)
			return 1;
	}
	else
	{
		return -1;
	}
}

void add_ocrtext2str(char *dest, char *src, const char *crlf, unsigned crlf_length)
{
	while (*dest != '\0')
		dest++;
	while(*src != '\0' && *src != '\n')
	{
		*dest = *src;
		src++;
		dest++;
	}
	memcpy(dest, crlf, crlf_length);
	dest[crlf_length] = 0;	
	/*
	*dest++ = '\n';
	*dest = '\0'; */
}

/**
 * Check multiple rectangles and combine them to give one paragraph
 * for all text detected from rectangles
 */

char *paraof_ocrtext(struct cc_subtitle *sub, const char *crlf, unsigned crlf_length)
{
	int i;
	int len = 0;
	char *str;
	struct cc_bitmap* rect;

	shell_sort(sub->data, sub->nb_data, sizeof(struct cc_bitmap), compare_rect_by_ypos, NULL);
	for(i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		if(rect->ocr_text)
			len += strlen(rect->ocr_text);
	}
	if(len <= 0)
		return NULL;
	else
	{
		str = malloc(len+1);
		if(!str)
			return NULL;
		*str = '\0';
	}

	for(i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		add_ocrtext2str(str, rect->ocr_text, crlf, crlf_length);
		TessDeleteText(rect->ocr_text);
	}
	return str;
}
#else
char* ocr_bitmap(png_color *palette,png_byte *alpha, unsigned char* indata,unsigned char d,int w, int h, struct image_copy *copy)
{
	mprint("ocr not supported without tesseract\n");
	return NULL;
}
#endif
