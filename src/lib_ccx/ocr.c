#include <math.h>
#include <png.h>
#include "lib_ccx.h"
#ifdef ENABLE_OCR
#include <tesseract/capi.h>
#include <leptonica/allheaders.h>
#include "ccx_common_constants.h"
#include <dirent.h>
#include "ccx_encoders_helpers.h"
#include "ccx_encoders_spupng.h"
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#include "ocr.h"

struct ocrCtx
{
	TessBaseAPI *api;
};

struct transIntensity
{
	uint8_t *t;
	png_color *palette;
};

static int check_trans_tn_intensity(const void *p1, const void *p2, void *arg)
{
	struct transIntensity *ti = arg;
	unsigned char *tmp = (unsigned char *)p1;
	unsigned char *act = (unsigned char *)p2;
	unsigned char tmp_i;
	unsigned char act_i;
	/** TODO verify that RGB follow ITU-R BT.709
	 *  Below formula is valid only for 709 standard
	 *  Y = 0.2126 R + 0.7152 G + 0.0722 B
	 */
	tmp_i = (0.2126 * ti->palette[*tmp].red) + (0.7152 * ti->palette[*tmp].green) + (0.0722 * ti->palette[*tmp].blue);
	act_i = (0.2126 * ti->palette[*act].red) + (0.7152 * ti->palette[*act].green) + (0.0722 * ti->palette[*act].blue);

	if (ti->t[*tmp] < ti->t[*act] || (ti->t[*tmp] == ti->t[*act] && tmp_i < act_i))
		return -1;
	else if (ti->t[*tmp] == ti->t[*act] && tmp_i == act_i)
		return 0;

	return 1;
}

static int search_language_pack(const char *dir_name, const char *lang_name)
{
	if (!dir_name)
		return -1;

	// Search for a tessdata folder in the specified directory
	char *dirname = strdup(dir_name);
	if (!dirname)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In search_language_pack: Out of memory allocating dirname.");
	}

	size_t dirname_len = strlen(dirname);
	int need_slash = (dirname[dirname_len - 1] != '/');
	size_t new_size = dirname_len + strlen("tessdata/") + need_slash + 1;
	char *new_dirname = realloc(dirname, new_size);
	if (!new_dirname)
	{
		free(dirname);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In search_language_pack: Out of memory reallocating dirname.");
	}
	dirname = new_dirname;

	// Append "/" if needed and "tessdata/" using snprintf
	snprintf(dirname + dirname_len, new_size - dirname_len, "%stessdata/", need_slash ? "/" : "");

	DIR *dp;
	struct dirent *dirp;
	char filename[256];
	if ((dp = opendir(dirname)) == NULL)
	{
		free(dirname);
		return -1;
	}
	snprintf(filename, 256, "%s.traineddata", lang_name);
	while ((dirp = readdir(dp)) != NULL)
	{
		if (!strcmp(dirp->d_name, filename))
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

void delete_ocr(void **arg)
{
	struct ocrCtx *ctx = *arg;
	TessBaseAPIEnd(ctx->api);
	TessBaseAPIDelete(ctx->api);
	freep(arg);
}

/**
 * get_executable_directory
 *
 * Returns the directory containing the executable.
 * Returns a pointer to a static buffer, or NULL on failure.
 */
static const char *get_executable_directory(void)
{
	static char exe_dir[1024] = {0};
	static int initialized = 0;

	if (initialized)
		return exe_dir[0] ? exe_dir : NULL;

	initialized = 1;

#ifdef _WIN32
	char exe_path[MAX_PATH];
	DWORD len = GetModuleFileNameA(NULL, exe_path, MAX_PATH);
	if (len == 0 || len >= MAX_PATH)
		return NULL;

	// Find the last backslash and truncate there
	char *last_sep = strrchr(exe_path, '\\');
	if (last_sep)
	{
		*last_sep = '\0';
		strncpy(exe_dir, exe_path, sizeof(exe_dir) - 1);
		exe_dir[sizeof(exe_dir) - 1] = '\0';
	}
#elif defined(__linux__)
	char exe_path[1024];
	ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
	if (len <= 0)
		return NULL;
	exe_path[len] = '\0';

	char *last_sep = strrchr(exe_path, '/');
	if (last_sep)
	{
		*last_sep = '\0';
		strncpy(exe_dir, exe_path, sizeof(exe_dir) - 1);
		exe_dir[sizeof(exe_dir) - 1] = '\0';
	}
#elif defined(__APPLE__)
	char exe_path[1024];
	uint32_t size = sizeof(exe_path);
	if (_NSGetExecutablePath(exe_path, &size) != 0)
		return NULL;

	char *last_sep = strrchr(exe_path, '/');
	if (last_sep)
	{
		*last_sep = '\0';
		strncpy(exe_dir, exe_path, sizeof(exe_dir) - 1);
		exe_dir[sizeof(exe_dir) - 1] = '\0';
	}
#endif

	return exe_dir[0] ? exe_dir : NULL;
}

/**
 * probe_tessdata_location
 *
 * This function probe tesseract data location
 *
 * Priority of Tesseract traineddata file search paths:-
 * 1. tessdata in TESSDATA_PREFIX, if it is specified. Overrides others
 * 2. tessdata in executable directory (for bundled tessdata)
 * 3. tessdata in current working directory
 * 4. tessdata in system locations (/usr/share, etc.)
 * 5. tessdata in default Tesseract install location (Windows)
 */
char *probe_tessdata_location(const char *lang)
{
	int ret = 0;

	const char *paths[] = {
	    getenv("TESSDATA_PREFIX"),
	    get_executable_directory(),
	    "./",
	    "/usr/share/",
	    "/usr/local/share/",
	    "/opt/homebrew/share/",
	    "/usr/share/tesseract-ocr/",
	    "/usr/share/tesseract-ocr/4.00/",
	    "/usr/share/tesseract-ocr/5/",
	    "/usr/share/tesseract/",
	    "C:\\Program Files\\Tesseract-OCR\\"};

	for (int i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
	{
		if (!search_language_pack(paths[i], lang))
			return (char *)paths[i];
	}

	return NULL;
}

void *init_ocr(int lang_index)
{
	int ret = -1;
	struct ocrCtx *ctx;
	const char *lang = NULL, *tessdata_path = NULL;

	ctx = (struct ocrCtx *)malloc(sizeof(struct ocrCtx));
	if (!ctx)
		return NULL;

	if (ccx_options.ocrlang)
		lang = ccx_options.ocrlang;
	else
	{
		if (lang_index == 0)
			lang_index = 1;
		lang = language[lang_index];
	}
	/* if language was undefined use english */

	tessdata_path = probe_tessdata_location(lang);
	if (!tessdata_path)
	{
		if (lang_index == 1)
		{
			mprint("eng.traineddata not found! No Switching Possible\n");
			free(ctx);
			return NULL;
		}
		mprint("%s.traineddata not found! Switching to English\n", lang);
		lang_index = 1;
		lang = language[lang_index];
		tessdata_path = probe_tessdata_location(lang);
		if (!tessdata_path)
		{
			mprint("eng.traineddata not found! No Switching Possible\n");
			free(ctx);
			return NULL;
		}
	}

	char *pars_vec = strdup("debug_file");
	if (!pars_vec)
	{
		free(ctx);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In init_ocr: Out of memory allocating pars_vec.");
	}
	char *pars_values = strdup("tess.log");
	if (!pars_values)
	{
		free(pars_vec);
		free(ctx);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In init_ocr: Out of memory allocating pars_values.");
	}

	ctx->api = TessBaseAPICreate();
	if (!strncmp("4.", TessVersion(), 2) || !strncmp("5.", TessVersion(), 2))
	{
		char tess_path[1024];
		snprintf(tess_path, 1024, "%s%s%s", tessdata_path, "/", "tessdata");
		if (ccx_options.ocr_oem < 0)
			ccx_options.ocr_oem = 1;
		ret = TessBaseAPIInit4(ctx->api, tess_path, lang, ccx_options.ocr_oem, NULL, 0, &pars_vec,
				       &pars_values, 1, false);
	}
	else
	{
		if (ccx_options.ocr_oem < 0)
			ccx_options.ocr_oem = 0;
		ret = TessBaseAPIInit4(ctx->api, tessdata_path, lang, ccx_options.ocr_oem, NULL, 0, &pars_vec,
				       &pars_values, 1, false);
	}

	// set PSM mode
	TessBaseAPISetPageSegMode(ctx->api, ccx_options.psm);

	// Set character blacklist to prevent common OCR errors (e.g. | vs I)
	// These characters are rarely used in subtitles but often misrecognized
	if (ccx_options.ocr_blacklist)
	{
		TessBaseAPISetVariable(ctx->api, "tessedit_char_blacklist", "|\\`_~");
	}

	free(pars_vec);
	free(pars_values);

	if (ret < 0)
	{
		mprint("Failed TessBaseAPIInit4 %d\n", ret);
		goto fail;
	}
	return ctx;
fail:
	delete_ocr((void **)&ctx);
	return NULL;
}

/*
 * The return value **has** to be freed:
 *
 * ```c
 * BOX *box = ignore_alpha_at_edge(...);
 * boxDestroy(&box);
 * ```
 */
BOX *ignore_alpha_at_edge(png_byte *alpha, unsigned char *indata, int w, int h, PIX *in, PIX **out)
{
	int i, j, index, start_x = -1, end_x = -1;
	BOX *cropWindow;

	// Find the leftmost and rightmost columns with visible (non-transparent) pixels
	for (j = 0; j < w; j++)
	{
		for (i = 0; i < h; i++)
		{
			index = indata[i * w + j];
			if (alpha[index] != 0)
			{
				if (start_x < 0)
					start_x = j;
				end_x = j;
				break; // Found visible pixel in this column, move to next
			}
		}
	}

	// Handle edge cases: no visible pixels or invalid dimensions
	if (start_x < 0 || end_x < start_x || w <= 0 || h <= 0)
	{
		// Return the entire image as fallback
		cropWindow = boxCreate(0, 0, w, h);
		*out = pixClone(in);
		return cropWindow;
	}

	int crop_width = end_x - start_x + 1;
	if (crop_width <= 0)
		crop_width = w;

	cropWindow = boxCreate(start_x, 0, crop_width, h);
	*out = pixClipRectangle(in, cropWindow, NULL);

	// If clipping failed, return the original image
	if (*out == NULL)
	{
		boxDestroy(&cropWindow);
		cropWindow = boxCreate(0, 0, w, h);
		*out = pixClone(in);
	}

	return cropWindow;
}

/**
 * Structure to hold the vertical boundaries of a detected text line.
 */
struct line_bounds
{
	int start_y; // Top row of line (inclusive)
	int end_y;   // Bottom row of line (inclusive)
};

/**
 * Detects horizontal text line boundaries in a bitmap by finding rows of
 * fully transparent pixels that separate lines of text.
 *
 * @param alpha     Palette alpha values (indexed by pixel value)
 * @param indata    Bitmap pixel data (palette indices, w*h bytes)
 * @param w         Image width
 * @param h         Image height
 * @param lines     Output: allocated array of line boundaries (caller must free)
 * @param num_lines Output: number of lines found
 * @param min_gap   Minimum consecutive transparent rows to count as line separator
 * @return 0 on success, -1 on failure
 */
static int detect_text_lines(png_byte *alpha, unsigned char *indata,
                             int w, int h,
                             struct line_bounds **lines, int *num_lines,
                             int min_gap)
{
	if (!alpha || !indata || !lines || !num_lines || w <= 0 || h <= 0)
		return -1;

	*lines = NULL;
	*num_lines = 0;

	// Allocate array to track which rows have visible content
	int *row_has_content = (int *)malloc(h * sizeof(int));
	if (!row_has_content)
		return -1;

	// Scan each row to determine if it has any visible (non-transparent) pixels
	for (int i = 0; i < h; i++)
	{
		row_has_content[i] = 0;
		for (int j = 0; j < w; j++)
		{
			int index = indata[i * w + j];
			if (alpha[index] != 0)
			{
				row_has_content[i] = 1;
				break; // Found visible pixel, no need to check rest of row
			}
		}
	}

	// Count lines by finding runs of content rows separated by gaps
	int max_lines = (h / 2) + 1; // Conservative upper bound
	struct line_bounds *temp_lines = (struct line_bounds *)malloc(max_lines * sizeof(struct line_bounds));
	if (!temp_lines)
	{
		free(row_has_content);
		return -1;
	}

	int line_count = 0;
	int in_line = 0;
	int line_start = 0;
	int gap_count = 0;

	for (int i = 0; i < h; i++)
	{
		if (row_has_content[i])
		{
			if (!in_line)
			{
				// Start of a new line
				line_start = i;
				in_line = 1;
			}
			gap_count = 0;
		}
		else
		{
			if (in_line)
			{
				gap_count++;
				if (gap_count >= min_gap)
				{
					// End of line found (gap is large enough)
					if (line_count < max_lines)
					{
						temp_lines[line_count].start_y = line_start;
						temp_lines[line_count].end_y = i - gap_count;
						line_count++;
					}
					in_line = 0;
					gap_count = 0;
				}
			}
		}
	}

	// Handle last line if we ended while still in a line
	if (in_line && line_count < max_lines)
	{
		temp_lines[line_count].start_y = line_start;
		// Find the last row with content
		int last_content = h - 1;
		while (last_content > line_start && !row_has_content[last_content])
			last_content--;
		temp_lines[line_count].end_y = last_content;
		line_count++;
	}

	free(row_has_content);

	if (line_count == 0)
	{
		free(temp_lines);
		return -1;
	}

	// Shrink allocation to actual size
	*lines = (struct line_bounds *)realloc(temp_lines, line_count * sizeof(struct line_bounds));
	if (!*lines)
	{
		*lines = temp_lines; // Keep original if realloc fails
	}
	*num_lines = line_count;

	return 0;
}

/**
 * Performs OCR on a single text line image using PSM 7 (single line mode).
 *
 * @param ctx      OCR context (contains Tesseract API)
 * @param line_pix Pre-processed PIX for single line (grayscale, inverted)
 * @return Recognized text (caller must free with free()), or NULL on failure
 */
static char *ocr_single_line(struct ocrCtx *ctx, PIX *line_pix)
{
	if (!ctx || !ctx->api || !line_pix)
		return NULL;

	// Save current PSM
	int saved_psm = TessBaseAPIGetPageSegMode(ctx->api);

	// Set PSM 7 for single line recognition
	TessBaseAPISetPageSegMode(ctx->api, 7); // PSM_SINGLE_LINE

	// Perform OCR
	TessBaseAPISetImage2(ctx->api, line_pix);
	BOOL ret = TessBaseAPIRecognize(ctx->api, NULL);

	char *text = NULL;
	if (!ret)
	{
		char *tess_text = TessBaseAPIGetUTF8Text(ctx->api);
		if (tess_text)
		{
			text = strdup(tess_text);
			TessDeleteText(tess_text);
		}
	}

	// Restore original PSM
	TessBaseAPISetPageSegMode(ctx->api, saved_psm);

	return text;
}

void debug_tesseract(struct ocrCtx *ctx, char *dump_path)
{
#ifdef OCR_DEBUG
	char str[1024] = "";
	static int i = 0;
	PIX *pix = NULL;
	PIXA *pixa = NULL;

	pix = TessBaseAPIGetInputImage(ctx->api);
	snprintf(str, sizeof(str), "%sinput_%d.jpg", dump_path, i);
	pixWrite(str, pix, IFF_JFIF_JPEG);

	pix = TessBaseAPIGetThresholdedImage(ctx->api);
	snprintf(str, sizeof(str), "%sthresholded_%d.jpg", dump_path, i);
	pixWrite(str, pix, IFF_JFIF_JPEG);

	TessBaseAPIGetRegions(ctx->api, &pixa);
	snprintf(str, sizeof(str), "%sregion_%d", dump_path, i);
	pixaWriteFiles(str, pixa, IFF_JFIF_JPEG);

	TessBaseAPIGetTextlines(ctx->api, &pixa, NULL);
	snprintf(str, sizeof(str), "%slines_%d", dump_path, i);
	pixaWriteFiles(str, pixa, IFF_JFIF_JPEG);

	TessBaseAPIGetWords(ctx->api, &pixa);
	snprintf(str, sizeof(str), "%swords_%d", dump_path, i);
	pixaWriteFiles(str, pixa, IFF_JFIF_JPEG);

	i++;
#endif
}
char *ocr_bitmap(void *arg, png_color *palette, png_byte *alpha, unsigned char *indata, int w, int h, struct image_copy *copy)
{
	// uncomment the below lines to output raw image as debug.png iteratively
	// save_spupng("debug.png", indata, w, h, palette, alpha, 16);

	PIX *pix = NULL;
	PIX *cpix = NULL;
	PIX *cpix_gs = NULL; // Grayscale version
	PIX *color_pix = NULL;
	PIX *color_pix_out = NULL;
	int i, j, index;
	unsigned int wpl;
	unsigned int *data, *ppixel;
	BOOL tess_ret = FALSE;
	struct ocrCtx *ctx = arg;
	char *combined_text = NULL; // Used by line-split mode
	size_t combined_len = 0;    // Used by line-split mode
	pix = pixCreate(w, h, 32);
	color_pix = pixCreate(w, h, 32);
	if (pix == NULL || color_pix == NULL)
	{
		if (pix)
			pixDestroy(&pix);
		if (color_pix)
			pixDestroy(&color_pix);
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
			composeRGBPixel(palette[index].red, palette[index].green, palette[index].blue, ppixel);
			SET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL, alpha[index]);
			ppixel++;
		}
	}
	BOX *temp = ignore_alpha_at_edge(alpha, indata, w, h, pix, &cpix);
	boxDestroy(&temp);

	// For the unquantized bitmap
	wpl = pixGetWpl(color_pix);
	data = pixGetData(color_pix);
	for (i = 0; i < h; i++)
	{
		ppixel = data + i * wpl;
		for (j = 0; j < w; j++)
		{
			index = copy->data[i * w + (j)];
			composeRGBPixel(copy->palette[index].red, copy->palette[index].green, copy->palette[index].blue, ppixel);
			SET_DATA_BYTE(ppixel, L_ALPHA_CHANNEL, copy->alpha[index]);
			ppixel++;
		}
	}

	BOX *crop_points = ignore_alpha_at_edge(copy->alpha, copy->data, w, h, color_pix, &color_pix_out);

	l_int32 x, y, _w, _h;

	boxGetGeometry(crop_points, &x, &y, &_w, &_h);

	// Converting image to grayscale for OCR to avoid issues with transparency
	cpix_gs = pixConvertRGBToGray(cpix, 0.0, 0.0, 0.0);

	// Invert the grayscale image for better OCR accuracy
	// DVB subtitles typically have light text on dark background, but
	// Tesseract expects dark text on light background
	if (cpix_gs != NULL)
		pixInvert(cpix_gs, cpix_gs);

	// Note: Upscaling was removed - testing showed it degrades OCR quality for DVB subtitles
	// The original bitmap quality (e.g., 520x84) is sufficient for Tesseract

	if (cpix_gs == NULL)
	{
		// Grayscale conversion failed (likely due to invalid/corrupt bitmap data)
		// Skip this bitmap instead of crashing - this can happen with
		// corrupted DVB subtitle packets or live stream discontinuities
		mprint("\nIn ocr_bitmap: Failed to convert bitmap to grayscale. Skipped.\n");

		boxDestroy(&crop_points);
		pixDestroy(&pix);
		pixDestroy(&cpix);
		pixDestroy(&color_pix);
		pixDestroy(&color_pix_out);

		return NULL;
	}

	// Line splitting mode: detect lines and OCR each separately with PSM 7
	if (ccx_options.ocr_line_split && h > 30)
	{
		struct line_bounds *lines = NULL;
		int num_lines = 0;

		// Use min_gap of 3 rows to detect line boundaries
		if (detect_text_lines(alpha, indata, w, h, &lines, &num_lines, 3) == 0 && num_lines > 1)
		{
			// Multiple lines detected - process each separately with PSM 7
			// (combined_text and combined_len are declared at function scope)

			for (int line_idx = 0; line_idx < num_lines; line_idx++)
			{
				int line_h = lines[line_idx].end_y - lines[line_idx].start_y + 1;
				if (line_h <= 0)
					continue;

				// Extract line region from the grayscale image
				BOX *line_box = boxCreate(0, lines[line_idx].start_y,
				                          pixGetWidth(cpix_gs), line_h);
				PIX *line_pix_raw = pixClipRectangle(cpix_gs, line_box, NULL);
				boxDestroy(&line_box);

				if (line_pix_raw)
				{
					// Add white padding around the line (helps Tesseract with edge characters)
					// The image is inverted (dark text on light bg), so add white (255) border
					int padding = 10;
					PIX *line_pix = pixAddBorderGeneral(line_pix_raw, padding, padding, padding, padding, 255);
					pixDestroy(&line_pix_raw);
					if (!line_pix)
						continue;
					char *line_text = ocr_single_line(ctx, line_pix);
					pixDestroy(&line_pix);

					if (line_text)
					{
						// Trim trailing whitespace from line
						size_t line_len = strlen(line_text);
						while (line_len > 0 && (line_text[line_len - 1] == '\n' ||
						                        line_text[line_len - 1] == '\r' ||
						                        line_text[line_len - 1] == ' '))
						{
							line_text[--line_len] = '\0';
						}

						if (line_len > 0)
						{
							// Append to combined result
							size_t new_len = combined_len + line_len + 2; // +1 for newline, +1 for null
							char *new_combined = (char *)realloc(combined_text, new_len);
							if (new_combined)
							{
								combined_text = new_combined;
								if (combined_len > 0)
								{
									combined_text[combined_len++] = '\n';
								}
								strcpy(combined_text + combined_len, line_text);
								combined_len += line_len;
							}
						}
						free(line_text);
					}
				}
			}

			free(lines);

			if (combined_text && combined_len > 0)
			{
				// Successfully processed lines - skip whole-image OCR
				// but continue to color detection below
				goto line_split_color_detection;
			}

			// If we got here, line splitting didn't produce results
			// Fall through to whole-image OCR
			if (combined_text)
				free(combined_text);
			combined_text = NULL;
		}
		else
		{
			// Line detection failed or only 1 line - fall through to whole-image OCR
			if (lines)
				free(lines);
		}
	}

	// Standard whole-image OCR path
	TessBaseAPISetImage2(ctx->api, cpix_gs);
	tess_ret = TessBaseAPIRecognize(ctx->api, NULL);
	debug_tesseract(ctx, "./temp/");
	if (tess_ret)
	{
		mprint("\nIn ocr_bitmap: Failed to perform OCR. Skipped.\n");

		boxDestroy(&crop_points);
		pixDestroy(&pix);
		pixDestroy(&cpix);
		pixDestroy(&cpix_gs);
		pixDestroy(&color_pix);
		pixDestroy(&color_pix_out);

		return NULL;
	}

	char *text_out_from_tes = TessBaseAPIGetUTF8Text(ctx->api);
	if (text_out_from_tes == NULL)
	{
		// OCR succeeded but no text was recognized - this is not a fatal error,
		// it just means the bitmap didn't contain recognizable text
		mprint("\nIn ocr_bitmap: OCR returned no text. Skipped.\n");

		boxDestroy(&crop_points);
		pixDestroy(&pix);
		pixDestroy(&cpix);
		pixDestroy(&cpix_gs);
		pixDestroy(&color_pix);
		pixDestroy(&color_pix_out);

		return NULL;
	}
	// Make a copy and get rid of the one from Tesseract since we're going to be operating on it
	// and using it directly causes new/free() warnings.
	char *text_out = strdup(text_out_from_tes);
	TessDeleteText(text_out_from_tes);
	if (!text_out)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_bitmap: Out of memory allocating text_out.");
	}

	// Jump target for line-split mode: use combined_text and continue with color detection
	if (0)
	{
	line_split_color_detection:
		text_out = combined_text;
		combined_text = NULL; // Transfer ownership
	}

	// Begin color detection
	// Using tlt_config.nofontcolor or ccx_options.nofontcolor (true when "--no-fontcolor" parameter used) to skip color detection if not required
	// This is also skipped if --no-spupngocr is set since the OCR output won't be used anyway
	int text_out_len;
	if ((text_out_len = strlen(text_out)) > 0 && !tlt_config.nofontcolor && !ccx_options.nofontcolor)
	{
		float h0 = -100;
		int written_tag = 0;
		TessResultIterator *ri = 0;
		TessPageIteratorLevel level = RIL_WORD;
		PIX *color_pix_processed = NULL; // Will hold preprocessed image for cleanup

		// Preprocess color_pix_out for Tesseract the same way as cpix_gs
		// Tesseract expects dark text on light background, but DVB subtitles typically
		// have light text on dark background. Without preprocessing, Tesseract
		// produces garbage results or crashes when iterating over words.
		color_pix_processed = pixConvertRGBToGray(color_pix_out, 0.0, 0.0, 0.0);
		if (color_pix_processed == NULL)
		{
			goto skip_color_detection;
		}
		pixInvert(color_pix_processed, color_pix_processed);

		// Note: Upscaling removed from color detection pass as well

		TessBaseAPISetImage2(ctx->api, color_pix_processed);
		tess_ret = TessBaseAPIRecognize(ctx->api, NULL);
		if (tess_ret != 0)
		{
			mprint("\nTessBaseAPIRecognize returned %d, skipping this bitmap.\n", tess_ret);
		}
		else
		{
			ri = TessBaseAPIGetIterator(ctx->api);
		}

		if (!tess_ret && ri != 0)
		{
			int iteration_count = 0;
			const int max_iterations = 10000; // Safety limit to prevent infinite loops
			do
			{
				// Safety check: limit iterations to prevent crashes on malformed data
				if (++iteration_count > max_iterations)
				{
					mprint("Warning: OCR color detection exceeded maximum iterations, skipping.\n");
					break;
				}

				char *word = TessResultIteratorGetUTF8Text(ri, level);
				// float conf = TessResultIteratorConfidence(ri,level);
				int x1, y1, x2, y2;
				if (!TessPageIteratorBoundingBox((TessPageIterator *)ri, level, &x1, &y1, &x2, &y2))
				{
					if (word)
						TessDeleteText(word);
					continue;
				}
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
				int max_color = 2;

				histogram = (uint32_t *)malloc(copy->nb_colors * sizeof(uint32_t));
				if (!histogram)
				{
					fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_bitmap: Out of memory allocating histogram.");
				}
				iot = (uint8_t *)malloc(copy->nb_colors * sizeof(uint8_t));
				if (!iot)
				{
					free(histogram);
					fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_bitmap: Out of memory allocating iot.");
				}
				mcit = (uint32_t *)malloc(copy->nb_colors * sizeof(uint32_t));
				if (!mcit)
				{
					free(histogram);
					free(iot);
					fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_bitmap: Out of memory allocating mcit.");
				}
				struct transIntensity ti = {copy->alpha, copy->palette};
				memset(histogram, 0, copy->nb_colors * sizeof(uint32_t));

				/* initializing intensity ordered table with serial order of unsorted color table */
				for (int i = 0; i < copy->nb_colors; i++)
				{
					iot[i] = i;
				}
				memset(mcit, 0, copy->nb_colors * sizeof(uint32_t));

				/* calculate histogram of image */
				int firstpixel = copy->data[0]; // TODO: Verify this border pixel assumption holds

				// Bounds check: validate bounding box coordinates
				// The bounding box (x1,y1,x2,y2) is relative to the cropped image.
				// With crop offset (x,y), the original coordinates are (x+x1, y+y1) to (x+x2, y+y2).
				// Ensure we don't access outside the original image bounds.
				int orig_x1 = x + x1;
				int orig_y1 = y + y1;
				int orig_x2 = x + x2;
				int orig_y2 = y + y2;

				if (orig_x1 < 0 || orig_y1 < 0 || orig_x2 >= w || orig_y2 >= h ||
				    orig_x1 > orig_x2 || orig_y1 > orig_y2)
				{
					// Invalid bounding box - skip this word
					freep(&histogram);
					freep(&mcit);
					freep(&iot);
					if (word)
						TessDeleteText(word);
					continue;
				}

				for (int i = y1; i <= y2; i++)
				{
					for (int j = x1; j <= x2; j++)
					{
						int idx = (y + i) * w + (x + j);
						if (idx >= 0 && idx < w * h)
						{
							int color_idx = copy->data[idx];
							if (color_idx >= 0 && color_idx < copy->nb_colors)
							{
								if (color_idx != firstpixel)
									histogram[color_idx]++;
							}
						}
					}
				}
				/* sorted in increasing order of intensity */
				shell_sort((void *)iot, copy->nb_colors, sizeof(*iot), check_trans_tn_intensity, (void *)&ti);
				// ccx_common_logging.log_ftn("Intensity ordered table\n");
				// for (int i = 0; i < copy->nb_colors; i++)
				// {
				// 	ccx_common_logging.log_ftn("%02d) map %02d hist %02d\n",
				// 		i, iot[i], histogram[iot[i]]);
				// }
				/**
				 * using selection sort since need to find only max_color
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
					alpha[i] = copy->alpha[i];
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
				int r_avg = 0, g_avg = 0, b_avg = 0, denom = 0;
				for (int i = 0; i < copy->nb_colors; i++)
				{
					if (palette[i].red == ((copy->bgcolor >> 16) & 0xff) &&
					    palette[i].green == ((copy->bgcolor >> 8) & 0xff) &&
					    palette[i].blue == ((copy->bgcolor >> 0) & 0xff))
						continue;
					denom++;
					r_avg += palette[i].red;
					g_avg += palette[i].green;
					b_avg += palette[i].blue;
				}
				if (denom != 0)
				{
					r_avg /= denom;
					g_avg /= denom;
					b_avg /= denom;
				}

				// Getting the hue value
				float h;
				float max = (((r_avg > g_avg) && (r_avg > b_avg)) ? r_avg : (g_avg > b_avg) ? g_avg
													    : b_avg);
				float min = (((r_avg < g_avg) && (r_avg < b_avg)) ? r_avg : (g_avg < b_avg) ? g_avg
													    : b_avg);
				if (max == 0.0f || max - min == 0.0f)
					h = 0;
				else if (max == r_avg)
					h = 60 * ((g_avg - b_avg) / (max - min)) + 0;
				else if (max == g_avg)
					h = 60 * ((b_avg - r_avg) / (max - min)) + 120;
				else
					h = 60 * ((r_avg - g_avg) / (max - min)) + 240;

				if (fabsf(h - h0) > 50) // Color has changed
				{
					// Write <font> tags for SRT and WebVTT
					if (ccx_options.write_format == CCX_OF_SRT ||
					    ccx_options.write_format == CCX_OF_WEBVTT)
					{
						const char *substr_format;
						int substr_len;
						if (written_tag)
						{
							substr_format = "</font><font color=\"#%02x%02x%02x\">";
							substr_len = sizeof("</font><font color=\"#000000\">") - 1;
						}
						else
						{
							substr_format = "<font color=\"#%02x%02x%02x\">";
							substr_len = sizeof("<font color=\"#000000\">") - 1;
						}

						char *pos;
						if ((pos = strstr(text_out, word)))
						{
							int index = pos - text_out;
							// Insert `<font>` tag into `text_out` at the location of `word`/`pos`
							char *new_text_out = realloc(text_out, text_out_len + substr_len + 1);
							if (!new_text_out)
							{
								fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_bitmap: Out of memory reallocating text_out.");
							}
							text_out = new_text_out;
							// Save the value is that is going to get overwritten by `snprintf`
							char replaced_by_null = text_out[index];
							memmove(text_out + index + substr_len + 1, text_out + index + 1, text_out_len - index);
							snprintf(text_out + index, substr_len + 1, substr_format, r_avg, g_avg, b_avg);
							text_out[index + substr_len] = replaced_by_null;
							text_out_len += substr_len;
							written_tag = 1;
						}
						else if (!written_tag)
						{
							// Insert `substr` at the beginning of `text_out`
							char *new_text_out = realloc(text_out, text_out_len + substr_len + 1);
							if (!new_text_out)
							{
								fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_bitmap: Out of memory reallocating text_out.");
							}
							text_out = new_text_out;
							char replaced_by_null = *text_out;
							memmove(text_out + substr_len + 1, text_out + 1, text_out_len);
							snprintf(text_out, substr_len + 1, substr_format, r_avg, g_avg, b_avg);
							text_out[substr_len] = replaced_by_null;
							text_out_len += substr_len;
							written_tag = 1;
						}
					}
				}

				h0 = h;

				freep(&histogram);
				freep(&mcit);
				freep(&iot);
				TessDeleteText(word);
			} while (TessPageIteratorNext((TessPageIterator *)ri, level));

			// Write missing <font> or </font> for each line
			if (ccx_options.write_format == CCX_OF_SRT ||
			    ccx_options.write_format == CCX_OF_WEBVTT)
			{
				const char *closing_font = "</font>";
				int length_closing_font = 7; // exclude '\0'

				char *line_start = text_out;
				int length = strlen(text_out) + length_closing_font * 10; // usually enough
				char *new_text_out = malloc(length);
				if (!new_text_out)
				{
					fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_bitmap: Out of memory allocating new_text_out.");
				}
				char *new_text_out_iter = new_text_out;

				char *last_valid_char = text_out; // last character that is not '\n' or '\0'

				for (char *iter = text_out; *iter; iter++)
					if (*iter != '\n')
						last_valid_char = iter;

				char *last_font_tag = text_out; // Last <font> in this line
				char *last_font_tag_end = NULL;

				while (1)
				{

					char *line_end = line_start;
					while (*line_end && *line_end != '\n')
						line_end++; // find the line end

					if (new_text_out_iter != new_text_out)
					{
						memcpy(new_text_out_iter, "\n", 1);
						new_text_out_iter += 1;
					}

					// realloc if memory allocated may be not enough
					int length_needed = (new_text_out_iter - new_text_out) +
							    (line_end - line_start) +
							    (last_font_tag_end ? (last_font_tag_end - last_font_tag) : 0) +
							    length_closing_font + 32;

					if (length_needed > length)
					{

						length = max(length * 1.5, length_needed);
						long diff = new_text_out_iter - new_text_out;
						char *tmp = realloc(new_text_out, length);
						if (!tmp)
						{
							free(new_text_out);
							fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_bitmap: Out of memory reallocating new_text_out.");
						}
						new_text_out = tmp;
						new_text_out_iter = new_text_out + diff;
					}

					// Add <font> to the beginning of the line if it is missing
					// Assume there is always a <font> at the beginning of the first line
					if (last_font_tag_end && strstr(line_start, "<font color=\"#") != line_start)
					{
						if ((new_text_out_iter - new_text_out) +
							(last_font_tag_end - last_font_tag) >
						    length)
						{
							fatal(CCX_COMMON_EXIT_BUG_BUG, "In ocr_bitmap: Running out of memory. It shouldn't happen. Please report.\n", errno);
						}
						memcpy(new_text_out_iter, last_font_tag, last_font_tag_end - last_font_tag);
						new_text_out_iter += last_font_tag_end - last_font_tag;
					}

					// Find the last <font> tag
					char *font_tag = line_start;
					while (1)
					{
						font_tag = strstr(font_tag + 1, "<font color=\"#");
						if (font_tag == NULL || font_tag > line_end)
							break;
						last_font_tag = font_tag;
					}
					last_font_tag_end = strstr(last_font_tag, ">");
					if (last_font_tag_end > line_end)
						last_font_tag_end = NULL;
					if (last_font_tag_end)
					{
						last_font_tag_end += 1; // move string to the "right" if ">" was found, otherwise leave empty string (solves #1084)
					}
					// Copy the content of the subtitle
					memcpy(new_text_out_iter, line_start, line_end - line_start);
					new_text_out_iter += line_end - line_start;

					// Add </font> if it is indeed missing
					if (line_end - line_start < length_closing_font ||
					    strncmp(line_start, closing_font, length_closing_font))
					{
						memcpy(new_text_out_iter, closing_font, length_closing_font);
						new_text_out_iter += length_closing_font;
					}

					if (line_end - 1 == last_valid_char)
						break;
					line_start = line_end + 1;
				}
				*new_text_out_iter = '\0';
				freep(&text_out);
				text_out = new_text_out;
			}
		}
	skip_color_detection:
		if (ri)
			TessResultIteratorDelete(ri);
		if (color_pix_processed)
			pixDestroy(&color_pix_processed);
	}
	// End Color Detection
	boxDestroy(&crop_points);

	pixDestroy(&pix);
	pixDestroy(&cpix);
	pixDestroy(&cpix_gs);
	pixDestroy(&color_pix);
	pixDestroy(&color_pix_out);

	return text_out;
}

void erode(png_color *palette, png_byte *alpha, uint8_t *bitmap, int w, int h, int nb_color, int background_index)
{
	// we will use a 2*2 kernel for the erosion
	for (int row = 0; row < h - 1; row++)
	{
		for (int col = 0; col < w - 1; col++)
		{
			if (bitmap[row * w + col] == background_index || bitmap[(row + 1) * w + col] == background_index ||
			    bitmap[row * w + (col + 1)] == background_index || bitmap[(row + 1) * w + (col + 1)] == background_index)
			{
				bitmap[row * w + col] = background_index;
			}
		}
	}
}

void dilate(png_color *palette, png_byte *alpha, uint8_t *bitmap, int w, int h, int nb_color, int foreground_index)
{
	// we will use a 2*2 kernel for the erosion
	for (int row = 0; row < h - 1; row++)
	{
		for (int col = 0; col < w - 1; col++)
		{
			if ((bitmap[row * w + col] == foreground_index && bitmap[(row + 1) * w + col] == foreground_index &&
			     bitmap[row * w + (col + 1)] == foreground_index && bitmap[(row + 1) * w + (col + 1)] == foreground_index))
			{
				bitmap[row * w + col] = foreground_index;
			}
		}
	}
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
			uint8_t *bitmap, int w, int h, int max_color, int nb_color)
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
	struct transIntensity ti = {alpha, palette};
	int text_color, text_bg_color;

	int ret = 0;

	histogram = (uint32_t *)malloc(nb_color * sizeof(uint32_t));
	if (!histogram)
	{
		ret = -1;
		goto end;
	}

	iot = (uint8_t *)malloc(nb_color * sizeof(uint8_t));
	if (!iot)
	{
		ret = -1;
		goto end;
	}

	mcit = (uint32_t *)malloc(nb_color * sizeof(uint32_t));
	if (!mcit)
	{
		ret = -1;
		goto end;
	}

	memset(histogram, 0, nb_color * sizeof(uint32_t));

	/* initializing intensity ordered table with serial order of unsorted color table */
	for (int i = 0; i < nb_color; i++)
	{
		iot[i] = i;
	}
	memset(mcit, 0, nb_color * sizeof(uint32_t));

	/* calculate histogram of image */
	for (int i = 0; i < w * h; i++)
	{
		histogram[bitmap[i]]++;
	}
	/* sorted in increasing order of intensity */
	shell_sort((void *)iot, nb_color, sizeof(*iot), check_trans_tn_intensity, (void *)&ti);

#ifdef OCR_DEBUG
	ccx_common_logging.log_ftn("Intensity ordered table\n");
	for (int i = 0; i < nb_color; i++)
	{
		ccx_common_logging.log_ftn("%02d) map %02d hist %02d\n",
					   i, iot[i], histogram[iot[i]]);
	}
#endif
	/**
	 * using selection sort since need to find only max_color
	 * Histogram becomes invalid in this loop
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

		// Assume second most frequent color to be text background (first is alpha channel)
		if (i == 1)
			text_bg_color = iot[max_ind];
		// Assume third most frequent color to be text color
		if (i == 2)
			text_color = iot[max_ind];

		for (j = i; j > 0 && max_ind < mcit[j - 1]; j--)
		{
			mcit[j] = mcit[j - 1];
		}
		mcit[j] = max_ind;
		histogram[iot[max_ind]] = 0;
	}

#ifdef OCR_DEBUG
	ccx_common_logging.log_ftn("max redundant intensities table\n");
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
	erode(palette, alpha, bitmap, w, h, nb_color, text_bg_color);
	dilate(palette, alpha, bitmap, w, h, nb_color, text_color);
#ifdef OCR_DEBUG
	ccx_common_logging.log_ftn("Colors present in quantized Image\n");
	for (int i = 0; i < nb_color; i++)
	{
		ccx_common_logging.log_ftn("%02d)r %03d g %03d b %03d a %03d\n",
					   i, palette[i].red, palette[i].green, palette[i].blue, alpha[i]);
	}
#endif
end:
	freep(&histogram);
	freep(&mcit);
	freep(&iot);
	return ret;
}

int ocr_rect(void *arg, struct cc_bitmap *rect, char **str, int bgcolor, int ocr_quantmode)
{
	int ret = 0;
	png_color *palette = NULL;
	png_byte *alpha = NULL;

	struct image_copy *copy;
	copy = (struct image_copy *)malloc(sizeof(struct image_copy));
	if (!copy)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_rect: Out of memory allocating copy.");
	}
	copy->nb_colors = rect->nb_colors;
	copy->bgcolor = bgcolor;
	copy->data = NULL;    // Initialize to NULL in case of early goto end
	copy->palette = NULL; // Initialize to NULL for safe cleanup
	copy->alpha = NULL;   // Initialize to NULL for safe cleanup

	copy->palette = (png_color *)malloc(rect->nb_colors * sizeof(png_color));
	if (!copy->palette)
	{
		free(copy);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_rect: Out of memory allocating copy->palette.");
	}
	copy->alpha = (png_byte *)malloc(rect->nb_colors * sizeof(png_byte));
	if (!copy->alpha)
	{
		free(copy->palette);
		free(copy);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_rect: Out of memory allocating copy->alpha.");
	}

	palette = (png_color *)malloc(rect->nb_colors * sizeof(png_color));
	if (!palette)
	{
		free(copy->alpha);
		free(copy->palette);
		free(copy);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_rect: Out of memory allocating palette.");
	}
	alpha = (png_byte *)malloc(rect->nb_colors * sizeof(png_byte));
	if (!alpha)
	{
		free(palette);
		free(copy->alpha);
		free(copy->palette);
		free(copy);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_rect: Out of memory allocating alpha.");
	}

	mapclut_paletee(palette, alpha, (uint32_t *)rect->data1, rect->nb_colors);
	mapclut_paletee(copy->palette, copy->alpha, (uint32_t *)rect->data1, rect->nb_colors);

	int size = rect->w * rect->h;
	dbg_print(CCX_DMT_DVB, "ocr_rect(): Trying W*H (%d * %d) so size = %d\n",
		  rect->w, rect->h, size);

	if (size < 0)
	{
		dbg_print(CCX_DMT_VERBOSE, "Width or height has a negative value");
		ret = -1;
		goto end;
	}

	copy->data = (unsigned char *)malloc(sizeof(unsigned char) * size);
	if (!copy->data)
	{
		free(alpha);
		free(palette);
		free(copy->alpha);
		free(copy->palette);
		free(copy);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In ocr_rect: Out of memory allocating copy->data.");
	}
	for (int i = 0; i < size; i++)
	{
		copy->data[i] = rect->data0[i];
	}

	switch (ocr_quantmode)
	{
		case 1:
			quantize_map(alpha, palette, rect->data0, rect->w, rect->h, 3, rect->nb_colors);
			break;

			// Case 2 reduces the color set of the image
		case 2:
			for (int i = 0; i < (rect->nb_colors); i++)
			{
				// Taking the quotient of the palette color with 8 shades in each RGB
				palette[i].red = (int)((palette[i].red + 1) / 32);
				palette[i].blue = (int)((palette[i].blue + 1) / 32);
				palette[i].green = (int)((palette[i].green + 1) / 32);

				// Making the palette color value closest to original, from among the 8 set colors
				palette[i].red *= 32;
				palette[i].blue *= 32;
				palette[i].green *= 32;
			}
			break;
	}

	*str = ocr_bitmap(arg, palette, alpha, rect->data0, rect->w, rect->h, copy);

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
int compare_rect_by_ypos(const void *p1, const void *p2, void *arg)
{
	const struct cc_bitmap *r1 = p1;
	const struct cc_bitmap *r2 = p2;
	if (r1->y > r2->y)
	{
		return 1;
	}
	else if (r1->y == r2->y)
	{
		if (r1->x > r2->x)
			return 1;
	}
	return -1;
}

void add_ocrtext2str(char *dest, char *src, const unsigned char *crlf, unsigned crlf_length)
{
	char *line_scan;
	int char_found;
	while (*dest != '\0')
		dest++;
	while (*src != '\0')
	{
		// checks if a line has actual content in it before adding it
		if (*src == '\n')
		{
			char_found = 0;
			line_scan = src + 1;
			// multiple blocks of newlines
			while (*(line_scan) == '\n')
			{
				line_scan++;
				src++;
			}
			// empty lines
			while (*line_scan != '\n' && *line_scan != '\0')
			{
				if (*line_scan > 32)
				{
					char_found = 1;
					break;
				}
				line_scan++;
			}
			if (!char_found)
			{
				src = line_scan;
			}
			if (*src == '\0')
				break;
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

char *paraof_ocrtext(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int i;
	int len = 0;
	char *str;
	struct cc_bitmap *rect;

	shell_sort(sub->data, sub->nb_data, sizeof(struct cc_bitmap), compare_rect_by_ypos, NULL);
	for (i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		if (rect->ocr_text)
			len += strlen(rect->ocr_text);
	}
	if (len <= 0)
	{
		for (i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
		{
			freep(&rect->ocr_text);
		}
		return NULL;
	}
	else
	{
		str = malloc(len + 1 + 10); // Extra space for possible trailing '/n's at the end of tesseract UTF8 text
		if (!str)
		{
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In paraof_ocrtext: Out of memory allocating str.");
		}
		*str = '\0';
	}

	for (i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		if (!rect->ocr_text)
			continue;
		add_ocrtext2str(str, rect->ocr_text, context->encoded_crlf, context->encoded_crlf_length);
		freep(&rect->ocr_text);
	}
	return str;
}
#else

struct image_copy;

char *ocr_bitmap(png_color *palette, png_byte *alpha, unsigned char *indata, unsigned char d, int w, int h, struct image_copy *copy)
{
	mprint("ocr not supported without tesseract\n");
	return NULL;
}
#endif
