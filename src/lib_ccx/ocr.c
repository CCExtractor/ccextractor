#include "png.h"
#include "lib_ccx.h"
#ifdef ENABLE_OCR
#include "capi.h"
#include "ccx_common_constants.h"
#include "allheaders.h"
#include <dirent.h>
#include "ccx_encoders_helpers.h"
#include "ocr.h"
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

static int check_trans_tn_intensity(const void *p1, const void *p2, void *arg)
{
	struct transIntensity *ti = arg;
	unsigned char* tmp = (unsigned char*)p1;
	unsigned char* act = (unsigned char*)p2;
	unsigned char tmp_i;
	unsigned char act_i;
	/** TODO verify that RGB follow ITU-R BT.709
	 *  Below formula is valid only for 709 standard
		 *  Y = 0.2126 R + 0.7152 G + 0.0722 B
		 */
	tmp_i = (0.2126 * ti->palette[*tmp].red) + (0.7152 * ti->palette[*tmp].green) + (0.0722 * ti->palette[*tmp].blue);
	act_i = (0.2126 * ti->palette[*act].red) + (0.7152 * ti->palette[*act].green) + (0.0722 * ti->palette[*act].blue);

	if (ti->t[*tmp] < ti->t[*act] || (ti->t[*tmp] == ti->t[*act] &&  tmp_i < act_i))
		return -1;
	else if (ti->t[*tmp] == ti->t[*act] &&  tmp_i == act_i)
		return 0;

	return 1;
}

static int search_language_pack(const char *dir_name,const char *lang_name)
{
	//Search for a tessdata folder in the specified directory
	char *dirname = strdup(dir_name);
	dirname = realloc(dirname,strlen(dirname)+strlen("/tessdata/")+1);
	strcat(dirname,"/tessdata/");

	DIR *dp;
	struct dirent *dirp;
	char filename[256];
	if ((dp = opendir(dirname)) == NULL)
	{
		free(dirname);
		return -1;
	}
	snprintf(filename, 256, "%s.traineddata",lang_name);
	while ((dirp = readdir(dp)) != NULL)
	{
		if(!strcmp(dirp->d_name, filename))
		{
			closedir(dp);
			free(dirname);
			return 0;
		}
	}
	free(dirname);
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
	int ret = -1;
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

	if(ccx_options.dvblang)
	{
		if(strcmp(language[lang_index],ccx_options.dvblang)!=0)
			goto fail;
	}

	/*Priority of Tesseract traineddata file search paths:-
		1. tessdata in TESSDATA_PREFIX, if it is specified. Overrides others
		2. tessdata in current working directory
		3. tessdata in /usr/share
	*/
	int data_location = 0;
	char *tessdata_dir_path=".";
	if(!getenv("TESSDATA_PREFIX"))
	{
		ret = search_language_pack(tessdata_dir_path,language[lang_index]);
	}
	if(ret < 0)
	{
		data_location = 1;
		if(getenv("TESSDATA_PREFIX"))
			ret = search_language_pack(getenv("TESSDATA_PREFIX"), language[lang_index]);
		else
			ret = search_language_pack("/usr/share", language[lang_index]);
	}
	if(ret < 0 && lang_index != 1 && ccx_options.ocrlang==NULL)
	{
		mprint("%s.traineddata not found! Switching to English\n",language[lang_index]);
		/* select english */
		lang_index = 1;
	}

	const char* lang = NULL, *tessdata_path = NULL;
	if (ccx_options.ocrlang)
		lang = ccx_options.ocrlang;
	else if (data_location == 1)
		lang = language[lang_index];
	else {
		lang = language[lang_index];
		tessdata_path = tessdata_dir_path;
	}

	char* pars_vec = strdup("debug_file");
	char* pars_values = strdup("/dev/null");

	ret = TessBaseAPIInit4(ctx->api, tessdata_path, lang, ccx_options.ocr_oem, NULL, 0, &pars_vec,
		&pars_values, 1, false);

	free(pars_vec);
	free(pars_values);

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
	int i, j, index, start_y=0, end_y=0;
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
#ifdef OCR_DEBUG
	{
	char str[128] = "";
	static int i = 0;
	sprintf(str,"temp/file_c_%d.jpg",i);
	printf("Writing file_c_%d.jpg\n", i);
	pixWrite(str, pixConvertRGBToGray(cpix, 0.0, 0.0, 0.0), IFF_JFIF_JPEG);
	i++;
	}
#endif

	cpix = pixConvertRGBToGray(cpix, 0.0, 0.0, 0.0); // Abhinav95: Converting image to grayscale for OCR to avoid issues with transparency
	TessBaseAPISetImage2(ctx->api, cpix);
	color_pix_out = TessBaseAPIGetThresholdedImage(ctx->api);
	if (tess_ret = TessBaseAPIRecognize(ctx->api, NULL)) {
		mprint("\nIn ocr_bitmap: Failed to perform OCR. Skipped.\n");

		pixDestroy(&pix);
		pixDestroy(&cpix);
		pixDestroy(&color_pix);
		pixDestroy(&color_pix_out);
		
		return NULL;
	}

	text_out = TessBaseAPIGetUTF8Text(ctx->api);
	if (text_out == NULL)
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In ocr_bitmap: Failed to perform OCR - Failed to get text. Please report.\n", errno);

	// Begin color detection
	if(ccx_options.dvbcolor && strlen(text_out)>0)
	{
		float h0 = -100;
		int written_tag = 0;
		TessResultIterator* ri = 0;
		TessPageIteratorLevel level = RIL_WORD;
		TessBaseAPISetImage2(ctx->api, color_pix_out);
		tess_ret = TessBaseAPIRecognize(ctx->api, NULL);
		if (tess_ret != 0) {
			mprint("\nTessBaseAPIRecognize returned %d, skipping this bitmap.\n", tess_ret);
		}
		else 
			ri = TessBaseAPIGetIterator(ctx->api);

		if(!tess_ret && ri!=0)
		{
			do
			{
				char* word = TessResultIteratorGetUTF8Text(ri,level);
				float conf = TessResultIteratorConfidence(ri,level);
				int x1, y1, x2, y2;
				if (!TessPageIteratorBoundingBox((TessPageIterator *)ri, level, &x1, &y1, &x2, &y2))
					continue;
				// printf("word: '%s';  \tconf: %.2f; BoundingBox: %d,%d,%d,%d;",word, conf, x1, y1, x2, y2);
				// printf("word: '%s';", word);
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
				int firstpixel = copy->data[0];  //TODO: Verify this border pixel assumption holds
				for(int i=y1;i<=y2;i++)
				{
					for(int j=x1;j<=x2;j++)
					{
						if(copy->data[(crop_points->y+i)*w + (crop_points->x+j)]!=firstpixel)
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
				 * Histogram becomes invalid in this loop
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
				
				// Detecting the color present in quantized word image			
				int r_avg=0,g_avg=0,b_avg=0,denom=0;
				for (int i = 0; i < copy->nb_colors; i++)
				{
					if(palette[i].red == ((copy->bgcolor >> 16) & 0xff) &&
					   palette[i].green == ((copy->bgcolor >> 8) & 0xff) && 
					   palette[i].blue == ((copy->bgcolor >> 0) & 0xff))
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

				// Getting the hue value
				float h;
				float max = (((r_avg > g_avg) && (r_avg > b_avg)) ? r_avg : (g_avg > b_avg) ? g_avg : b_avg);
				float min = (((r_avg < g_avg) && (r_avg < b_avg)) ? r_avg : (g_avg < b_avg) ? g_avg : b_avg);
				if(max==0.0f||max-min==0.0f) h = 0;
				else if(max==r_avg) h = 60 * ((g_avg - b_avg)/(max - min)) + 0;
				else if(max==g_avg) h = 60 * ((b_avg - r_avg)/(max - min)) + 120;
				else h = 60 * ((r_avg - g_avg)/(max - min)) + 240;

				if(abs(h-h0)>50) // Color has changed
				{
					// Write <font> tags for SRT and WebVTT
					if(ccx_options.write_format==CCX_OF_SRT ||
					   ccx_options.write_format==CCX_OF_WEBVTT)
					{
						char *substr;
						if(written_tag)
						{
							substr = (char*)malloc(sizeof("</font><font color=\"#000000\">"));
							sprintf(substr,"</font><font color=\"#%02x%02x%02x\">",r_avg,g_avg,b_avg);
						}
						else
						{
							substr = (char*)malloc(sizeof("<font color=\"#000000\">"));
							sprintf(substr,"<font color=\"#%02x%02x%02x\">",r_avg,g_avg,b_avg);
						}
						if(strstr(text_out,word))
						{
							char *text_out_copy = strdup(text_out);
							TessDeleteText(text_out);
							text_out = malloc(strlen(text_out_copy)+strlen(substr)+1);
							memset(text_out,0,strlen(text_out_copy)+strlen(substr)+1);
							int pos = (int)(strstr(text_out_copy,word)-text_out_copy);
							strncpy(text_out,text_out_copy,pos);
							int len = strlen(text_out);
							strcpy(text_out+len,substr);
							strcpy(text_out+len+strlen(substr),text_out_copy+len);
							free(text_out_copy);
							written_tag = 1;
						}
						else if(!written_tag)
						{
							char *text_out_copy = strdup(text_out);
							TessDeleteText(text_out);
							text_out = malloc(strlen(text_out_copy)+strlen(substr)+1);
							memset(text_out,0,strlen(text_out_copy)+strlen(substr)+1);
							strcpy(text_out,substr);
							strcpy(text_out+strlen(substr),text_out_copy);
							written_tag = 1;
						}
						free(substr);
					}
				}

				h0=h;

				freep(&histogram);
				freep(&mcit);
				freep(&iot);
				TessDeleteText(word);
			} while (TessPageIteratorNext((TessPageIterator *)ri,level));

			// Write missing <font> or </font> for each line
			if(ccx_options.write_format==CCX_OF_SRT ||
			   ccx_options.write_format==CCX_OF_WEBVTT)
			{
				const char *closing_font = "</font>";
				int length_closing_font = 7; // exclude '\0'

				char *line_start = text_out;
				int length = strlen(text_out) + length_closing_font * 10;  // usually enough
				char *new_text_out = malloc(length);
				char *new_text_out_iter = new_text_out;

				char *last_valid_char = text_out; // last character that is not '\n' or '\0'

				for (char *iter = text_out; *iter; iter++)
					if (*iter != '\n') last_valid_char = iter;

				char *last_font_tag = text_out; // Last <font> in this line
				char *last_font_tag_end = NULL;

				while (1) {

					char *line_end = line_start;
					while (*line_end && *line_end != '\n') line_end++; // find the line end

					if (new_text_out_iter != new_text_out) {
						memcpy(new_text_out_iter, "\n", 1);
						new_text_out_iter += 1;
					}

					// realloc if memory allocated may be not enough
					int length_needed = (new_text_out_iter - new_text_out) +
						(line_end - line_start) +
						length_closing_font + 32;

					if (length_needed > length) {

						length = max(length * 1.5, length_needed);
						long diff = new_text_out_iter - new_text_out;
						new_text_out = realloc(new_text_out, length);
						new_text_out_iter = new_text_out + diff;
						
					}

					// Add <font> to the beginning of the line if it is missing
					// Assume there is always a <font> at the beginning of the first line
					if (last_font_tag_end && strstr(line_start, "<font color=\"#") != line_start) {
						if ((new_text_out_iter - new_text_out) +
							(last_font_tag_end - last_font_tag) > length) {
							fatal(CCX_COMMON_EXIT_BUG_BUG, "In ocr_bitmap: Running out of memory. It shouldn't happen. Please report.\n", errno);
						}
						memcpy(new_text_out_iter, last_font_tag, last_font_tag_end - last_font_tag);
						new_text_out_iter += last_font_tag_end - last_font_tag;
					}

					// Find the last <font> tag
					char *font_tag = line_start;
					while (1) {

						font_tag = strstr(font_tag + 1, "<font color=\"#");
						if (font_tag == NULL || font_tag > line_end) break;
						last_font_tag = font_tag;

					}
					last_font_tag_end = strstr(last_font_tag, ">") + 1;

					// Copy the content of the subtitle
					memcpy(new_text_out_iter, line_start, line_end - line_start);
					new_text_out_iter += line_end - line_start;

					// Add </font> if it is indeed missing
					if (line_end - line_start < length_closing_font ||
						strncmp(line_start, closing_font, length_closing_font)) {
						
						memcpy(new_text_out_iter, closing_font, length_closing_font);
						new_text_out_iter += length_closing_font;

					}

					if (line_end - 1 == last_valid_char) break;
					line_start = line_end + 1;
				}
				*new_text_out_iter = '\0';
				TessDeleteText(text_out);
				text_out = new_text_out;
			}
		}
		TessResultIteratorDelete(ri);
	}
	// End Color Detection

	// boxDestroy(crop_points);

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
	char *line_scan;
	int char_found;
	while (*dest != '\0')
			dest++;
	while (*src != '\0')
	{
		//checks if a line has actual content in it before adding it
		if (*src == '\n') {
			char_found = 0;
			line_scan = src + 1;
			//multiple blocks of newlines
			while (*(line_scan) == '\n') {
				line_scan++;
				src++;
			}
			//empty lines
			while (*line_scan != '\n' && *line_scan != '\0') {
				if (*line_scan > 32) {
					char_found = 1;
					break;
				}
				line_scan++;
			}
			if (!char_found) {
				src = line_scan;
			}
			if (*src == '\0') break;
		}
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
		str = malloc(len+1+10); //Extra space for possible trailing '/n's at the end of tesseract UTF8 text
		if(!str)
			return NULL;
		*str = '\0';
	}

	for(i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		if (!rect->ocr_text) continue;
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
