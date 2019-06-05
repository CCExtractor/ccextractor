#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "ccx_encoders_common.h"
#include "ccx_encoders_helpers.h"
#include "utility.h"
#include "ocr.h"


static const char *webvtt_outline_css = "@import(%s)\n";

static const char *webvtt_inline_css = "/* default values */\n"
		"::cue {\n"
		"  line-height: 5.33vh;\n"
		"  font-size: 4.1vh;\n"
		"  font-family: monospace;\n"
		"  font-style: normal;\n"
		"  font-weight: normal;\n"
		"  background-color: black;\n"
		"  color: white;\n"
		"}\n"
		"/* special cue parts */\n"
		"::cue(c.transparent) {\n"
		"  color: transparent;\n"
		"}\n"
		"/* need to set this before changing color, otherwise the color is lost */\n"
		"::cue(c.semi-transparent) {\n"
		"  color: rgba(0, 0, 0, 0.5);\n"
		"}\n"
		"/* need to set this before changing color, otherwise the color is lost */\n"
		"::cue(c.opaque) {\n"
		"  color: rgba(0, 0, 0, 1);\n"
		"}\n"
		"::cue(c.blink) {\n"
		"  text-decoration: blink;\n"
		"}\n"
		"::cue(c.white) {\n"
		"  color: white;\n"
		"}\n"
		"::cue(c.red) {\n"
		"  color: red;\n"
		"}\n"
		"::cue(c.green) {\n"
		"  color: lime;\n"
		"}\n"
		"::cue(c.blue) {\n"
		"  color: blue;\n"
		"}\n"
		"::cue(c.cyan) {\n"
		"  color: cyan;\n"
		"}\n"
		"::cue(c.yellow) {\n"
		"  color: yellow;\n"
		"}\n"
		"::cue(c.magenta) {\n"
		"  color: magenta;\n"
		"}\n"
		"::cue(c.bg_transparent) {\n"
		"  background-color: transparent;\n"
		"}\n"
		"/* need to set this before changing color, otherwise the color is lost */\n"
		"::cue(c.bg_semi-transparent) {\n"
		"  background-color: rgba(0, 0, 0, 0.5);\n"
		"}\n"
		"/* need to set this before changing color, otherwise the color is lost */\n"
		"::cue(c.bg_opaque) {\n"
		"  background-color: rgba(0, 0, 0, 1);\n"
		"}\n"
		"::cue(c.bg_white) {\n"
		"  background-color: white;\n"
		"}\n"
		"::cue(c.bg_green) {\n"
		"  background-color: lime;\n"
		"}\n"
		"::cue(c.bg_blue) {\n"
		"  background-color: blue;\n"
		"}\n"
		"::cue(c.bg_cyan) {\n"
		"  background-color: cyan;\n"
		"}\n"
		"::cue(c.bg_red) {\n"
		"  background-color: red;\n"
		"}\n"
		"::cue(c.bg_yellow) {\n"
		"  background-color: yellow;\n"
		"}\n"
		"::cue(c.bg_magenta) {\n"
		"  background-color: magenta;\n"
		"}\n"
		"::cue(c.bg_black) {\n"
		"  background-color: black;\n"
		"}\n"
		"/* Examples of combined colors */\n"
		"::cue(c.bg_white.bg_semi-transparent) {\n"
		"  background-color: rgba(255, 255, 255, 0.5);\n"
		"}\n"
		"::cue(c.bg_green.bg_semi-transparent) {\n"
		"  background-color: rgba(0, 256, 0, 0.5);\n"
		"}\n"
		"::cue(c.bg_blue.bg_semi-transparent) {\n"
		"  background-color: rgba(0, 0, 255, 0.5);\n"
		"}\n"
		"::cue(c.bg_cyan.bg_semi-transparent) {\n"
		"  background-color: rgba(0, 255, 255, 0.5);\n"
		"}\n"
		"::cue(c.bg_red.bg_semi-transparent) {\n"
		"  background-color: rgba(255, 0, 0, 0.5);\n"
		"}\n"
		"::cue(c.bg_yellow.bg_semi-transparent) {\n"
		"  background-color: rgba(255, 255, 0, 0.5);\n"
		"}\n"
		"::cue(c.bg_magenta.bg_semi-transparent) {\n"
		"  background-color: rgba(255, 0, 255, 0.5);\n"
		"}\n"
		"::cue(c.bg_black.bg_semi-transparent) {\n"
		"  background-color: rgba(0, 0, 0, 0.5);\n"
		"}";

static const char* webvtt_pac_row_percent[] = { "10", "15.33", "20.66", "26", "31.33", "36.66", "42",
		"47.33", "52.66", "58", "63.33", "68.66", "74", "79.33", "84.66" };

/* The timing here is not PTS based, but output based, i.e. user delay must be accounted for
if there is any */
int write_stringz_as_webvtt(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end)
{
	int used;
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	int written;
	char timeline[128];

	millis_to_time(ms_start, &h1, &m1, &s1, &ms1);
	millis_to_time(ms_end - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.

	sprintf(timeline, "%02u:%02u:%02u.%03u --> %02u:%02u:%02u.%03u%s",
		h1, m1, s1, ms1, h2, m2, s2, ms2, context->encoded_crlf);
	used = encode_line(context, context->buffer, (unsigned char *)timeline);
	dbg_print(CCX_DMT_DECODER_608, "\n- - - WEBVTT caption - - -\n");
	dbg_print(CCX_DMT_DECODER_608, "%s", timeline);

	written = write(context->out->fh, context->buffer, used);
	if (written != used)
		return -1;
	int len = strlen(string);
	unsigned char *unescaped = (unsigned char *)malloc(len + 1);
	unsigned char *el = (unsigned char *)malloc(len * 3 + 1); // Be generous
	if (el == NULL || unescaped == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In write_stringz_as_webvtt() - not enough memory.\n");
	int pos_r = 0;
	int pos_w = 0;
	// Scan for \n in the string and replace it with a 0
	while (pos_r<len)
	{
		if (string[pos_r] == '\\' && string[pos_r + 1] == 'n')
		{
			unescaped[pos_w] = 0;
			pos_r += 2;
		}
		else
		{
			unescaped[pos_w] = string[pos_r];
			pos_r++;
		}
		pos_w++;
	}
	unescaped[pos_w] = 0;
	// Now read the unescaped string (now several string'z and write them)
	unsigned char *begin = unescaped;
	while (begin<unescaped + len)
	{
		unsigned int u = encode_line(context, el, begin);
		if (context->encoding != CCX_ENC_UNICODE)
		{
			dbg_print(CCX_DMT_DECODER_608, "\r");
			dbg_print(CCX_DMT_DECODER_608, "%s\n", context->subline);
		}
		written = write(context->out->fh, el, u);
		if (written != u)
		{
			free(el);
			free(unescaped);
			return -1;
		}
		written = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
		if (written != context->encoded_crlf_length)
		{   
            		free(el);
			return -1;
		}
		begin += strlen((const char *)begin) + 1;
	}

	dbg_print(CCX_DMT_DECODER_608, "- - - - - - - - - - - -\r\n");

	written = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
	free(el);
	free(unescaped);
	if (written != context->encoded_crlf_length)
	{
		return -1;
	}

	return 0;
}

int write_webvtt_header(struct encoder_ctx *context)
{
	if (context->wrote_webvtt_header) // Already done
		return 1;

	if (ccx_options.webvtt_create_css)
	{
		char *basefilename = get_basename(context->first_input_file);
		char *css_file_name = (char*)malloc((strlen(basefilename) + 4) * sizeof(char));		// strlen(".css") == 4
		sprintf(css_file_name, "%s.css", basefilename);

		FILE *f = fopen(css_file_name, "wb");
		if (f == NULL)
		{
			mprint("Warning: Error creating the file %s\n", css_file_name);
			return -1;
		}
		fprintf(f, "%s",webvtt_inline_css);
		fclose(f);

		char* outline_css_file = (char*)malloc((strlen(css_file_name) + strlen(webvtt_outline_css)) * sizeof(char));
		sprintf(outline_css_file, webvtt_outline_css, css_file_name);
		write (context->out->fh, outline_css_file, strlen(outline_css_file));
	} else if (ccx_options.use_webvtt_styling) {
		write(context->out->fh, webvtt_inline_css, strlen(webvtt_inline_css));
		if(ccx_options.enc_cfg.line_terminator_lf == 1) // If -lf parameter is set.
		{
			write(context->out->fh, "\n", 1);
		}
		else
		{
			write(context->out->fh,"\r\n",2);
		}
	}

	write(context->out->fh, "##\n", 3);
	write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);

	if (context->timing->sync_pts2fts_set)
	{
		char header_string[200];
		int used;
		unsigned h1, m1, s1, ms1;
		millis_to_time(context->timing->sync_pts2fts_fts, &h1, &m1, &s1, &ms1);
		sprintf(header_string, "X-TIMESTAMP-MAP=MPEGTS:%ld, LOCAL %02u:%02u:%02u.%03u\r\n\n",
			context->timing->sync_pts2fts_pts,
			h1, m1, s1, ms1);
		used = encode_line(context, context->buffer, (unsigned char *)header_string);
		write(context->out->fh, context->buffer, used);

	}

	context->wrote_webvtt_header = 1; // Do it even if couldn't write the header, because it won't be possible anyway
}


int write_cc_bitmap_as_webvtt(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
#ifdef ENABLE_OCR
	struct cc_bitmap* rect;
	LLONG ms_start, ms_end;
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	char timeline[128];
	int len = 0;
	int used;
	int i = 0;
	char *str;

	ms_start = sub->start_time;
	ms_end = sub->end_time;

	if (sub->nb_data == 0)
		return 0;

	write_webvtt_header(context);

	if (sub->flags & SUB_EOD_MARKER)
		context->prev_start = sub->start_time;

	str = paraof_ocrtext(sub, context->encoded_crlf, context->encoded_crlf_length);
	if (str)
	{
		if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
		{
			millis_to_time(ms_start, &h1, &m1, &s1, &ms1);
			millis_to_time(ms_end - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.
			context->srt_counter++; // Not needed for WebVTT but let's keep it around for now
			sprintf(timeline, "%02u:%02u:%02u.%03u --> %02u:%02u:%02u.%03u%s",
				h1, m1, s1, ms1, h2, m2, s2, ms2, context->encoded_crlf);
			used = encode_line(context, context->buffer, (unsigned char *)timeline);
            write(context->out->fh, context->buffer, used);
			len = strlen(str);
			write(context->out->fh, str, len);
			write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
		}
		freep(&str);
	}
	for (i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		freep(&rect->data0);
		freep(&rect->data1);
	}
#endif
	sub->nb_data = 0;
	freep(&sub->data);
	return ret;

}

int write_cc_subtitle_as_webvtt(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
	struct cc_subtitle *osub = sub;
	struct cc_subtitle *lsub = sub;

	while (sub)
	{
		if (sub->type == CC_TEXT)
		{
			ret = write_stringz_as_webvtt(sub->data, context, sub->start_time, sub->end_time);
			freep(&sub->data);
			sub->nb_data = 0;
		}
		lsub = sub;
		sub = sub->next;
	}
	while (lsub != osub)
	{
		sub = lsub->prev;
		freep(&lsub);
		lsub = sub;
	}

	return ret;
}

void get_color_events(int *color_events, int line_num, struct eia608_screen *data)
{
	int first, last;
	get_sentence_borders(&first, &last, line_num, data);

	int last_color = COL_WHITE;
	for (int i = first; i <= last; i++)
	{
		if (data->colors[line_num][i] != last_color)
		{
			// It does not make sense to keep the default white color in the events
			// WebVTT supports colors only is [COL_WHITE..COL_MAGENTA]
			if (data->colors[line_num][i] <= COL_MAGENTA)
				color_events[i] |= data->colors[line_num][i];	// Add this new color
			
			if (last_color != COL_WHITE && last_color <= COL_MAGENTA)
				color_events[i - 1] |= last_color << 16;	// Remove old color (event in the second part of the integer)
			
			last_color = data->colors[line_num][i];
		}
	}

	if (last_color != COL_WHITE)
	{
		color_events[last] |= last_color << 16;
	}
}

void get_font_events(int *font_events, int line_num, struct eia608_screen *data)
{
	int first, last;
	get_sentence_borders(&first, &last, line_num, data);

	int last_font = FONT_REGULAR;
	for (int i = first; i <= last; i++)
	{
		if (data->fonts[line_num][i] != last_font)
		{
			// It does not make sense to keep the regular font in the events
			// WebVTT supports all fonts from C608
			if (data->fonts[line_num][i] != FONT_REGULAR)	// Really can do it without condition because FONT_REGULAR == 0
				font_events[i] |= data->fonts[line_num][i];		// Add this new font

			if (last_font != FONT_REGULAR)
				font_events[i] |= last_font << 16;	// Remove old font (event in the second part of the integer)

			last_font = data->fonts[line_num][i];
		}
	}

	if (last_font != FONT_REGULAR)
	{
		font_events[last] |= last_font << 16;
	}
}

int write_cc_buffer_as_webvtt(struct eia608_screen *data, struct encoder_ctx *context)
{
	int used;
	int written;
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	LLONG ms_start, ms_end;
	int wrote_something = 0;
	ms_start = data->start_time;

	int empty_buf = 1;
	for (int i = 0; i<15; i++)
	{
		if (data->row_used[i])
		{
			empty_buf = 0;
			break;
		}
	}
	if (empty_buf) // Prevent writing empty screens. Not needed in .vtt
		return 0;

	ms_start += context->subs_delay;
	if (ms_start<0) // Drop screens that because of subs_delay start too early
		return 0;

	write_webvtt_header(context);

	ms_end = data->end_time;

	millis_to_time(ms_start, &h1, &m1, &s1, &ms1);
	millis_to_time(ms_end - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.

	for (int i = 0; i<15; i++)
	{
		if (data->row_used[i])
		{
			char timeline[128] = "";

			sprintf(timeline, "%02u:%02u:%02u.%03u --> %02u:%02u:%02u.%03u line:%s%%%s",
				h1, m1, s1, ms1, h2, m2, s2, ms2, webvtt_pac_row_percent[i], context->encoded_crlf);
			used = encode_line(context, context->buffer, (unsigned char *)timeline);

			dbg_print(CCX_DMT_DECODER_608, "\n- - - WEBVTT caption - - -\n");
			dbg_print(CCX_DMT_DECODER_608, "%s", timeline);
			written = write(context->out->fh, context->buffer, used);
			if (written != used)
				return -1;

			int length = get_line_encoded(context, context->subline, i, data);

			if (context->encoding != CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r");
				dbg_print(CCX_DMT_DECODER_608, "%s\n", context->subline);
			}

			int *color_events;
			int *font_events;
			if (ccx_options.use_webvtt_styling)
			{
				color_events = (int *)malloc(sizeof(int) * length);
				font_events = (int *)malloc(sizeof(int) * length);
				memset(color_events, 0, sizeof(int) * length);
				memset(font_events, 0, sizeof(int) * length);

				get_color_events(color_events, i, data);
				get_font_events(font_events, i, data);
			}

			// Write symbol by symbol with events
			for (int j = 0; j < length; j++)
			{
				if (ccx_options.use_webvtt_styling)
				{
					// opening events for fonts
					int open_font = font_events[j] & 0xFF;	// Last 16 bytes
					if (open_font != FONT_REGULAR)
					{
						if (open_font & FONT_ITALICS)
							write(context->out->fh, strdup("<i>"), 3);
						if (open_font & FONT_UNDERLINED)
							write(context->out->fh, strdup("<u>"), 3);
					}

					// opening events for colors
					int open_color = color_events[j] & 0xFF;	// Last 16 bytes
					if (open_color != COL_WHITE)
					{
						write(context->out->fh, strdup("<c."), 3);
						write(context->out->fh, color_text[open_color][0], strlen(color_text[open_color][0]));
						write(context->out->fh, ">", 1);
					}
				}

				// write current text symbol
				write(context->out->fh, &(context->subline[j]), 1);

				if (ccx_options.use_webvtt_styling)
				{
					// closing events for colors
					int close_color = color_events[j] >> 16;	// First 16 bytes
					if (close_color != COL_WHITE)
					{
						write(context->out->fh, strdup("</c>"), 4);
					}

					// closing events for fonts
					int close_font = font_events[j] >> 16;	// First 16 bytes
					if (close_font != FONT_REGULAR)
					{
						if (close_font & FONT_ITALICS)
							write(context->out->fh, strdup("</i>"), 4);
						if (close_font & FONT_UNDERLINED)
							write(context->out->fh, strdup("</u>"), 4);
					}
				}
			}

			if (ccx_options.use_webvtt_styling)
			{
				free(color_events);
				free(font_events);
			}

			written = write(context->out->fh,
				context->encoded_crlf, context->encoded_crlf_length);
			if (written != context->encoded_crlf_length)
				return -1;

			written = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
			if (written != context->encoded_crlf_length)
				return -1;

			wrote_something = 1;
		}
	}
	dbg_print(CCX_DMT_DECODER_608, "- - - - - - - - - - - -\r\n");

	return wrote_something;
}
