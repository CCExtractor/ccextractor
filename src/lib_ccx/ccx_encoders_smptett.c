/*
	Produces minimally-compliant SMPTE Timed Text (W3C TTML)
	format-compatible output

	See http://www.w3.org/TR/ttaf1-dfxp/ and
	https://www.smpte.org/sites/default/files/st2052-1-2010.pdf

	Copyright (C) 2012 John Kemp

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "ccx_encoders_common.h"
#include <png.h>
#include "ocr.h"
#include "utility.h"
#include "ccx_encoders_helpers.h"

void write_stringz_as_smptett(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end)
{
	int used;
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	int len = strlen(string);
	unsigned char *unescaped = (unsigned char *)malloc(len + 1);
	unsigned char *el = (unsigned char *)malloc(len * 3 + 1); // Be generous
	int pos_r = 0;
	int pos_w = 0;
	char str[1024];

	if (el == NULL || unescaped == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In write_stringz_as_smptett() - not enough memory.\n");

	// Write header on first caption (deferred file creation)
	if (write_subtitle_file_header(context, context->out) != 0)
		return;

	millis_to_time(ms_start, &h1, &m1, &s1, &ms1);
	millis_to_time(ms_end - 1, &h2, &m2, &s2, &ms2);

	snprintf(str, sizeof(str), "<p begin=\"%02u:%02u:%02u.%03u\" end=\"%02u:%02u:%02u.%03u\">\r\n", h1, m1, s1, ms1, h2, m2, s2, ms2);
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context, context->buffer, (unsigned char *)str);
	write_wrapped(context->out->fh, context->buffer, used);
	// Scan for \n in the string and replace it with a 0
	while (pos_r < len)
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
	while (begin < unescaped + len)
	{
		unsigned int u = encode_line(context, el, begin);
		if (context->encoding != CCX_ENC_UNICODE)
		{
			dbg_print(CCX_DMT_DECODER_608, "\r");
			dbg_print(CCX_DMT_DECODER_608, "%s\n", context->subline);
		}
		write_wrapped(context->out->fh, el, u);
		// write (wb->fh, encoded_br, encoded_br_length);

		write_wrapped(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
		begin += strlen((const char *)begin) + 1;
	}

	snprintf(str, sizeof(str), "</p>\n");
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context, context->buffer, (unsigned char *)str);
	write_wrapped(context->out->fh, context->buffer, used);

	free(el);
	free(unescaped);
}

int write_cc_bitmap_as_smptett(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
#ifdef ENABLE_OCR
	struct cc_bitmap *rect;
	// char timeline[128];
	int i, len = 0;

	if (sub->nb_data == 0)
		return 0;

	// Write header on first caption (deferred file creation)
	if (write_subtitle_file_header(context, context->out) != 0)
		return -1;

	rect = sub->data;

	if (sub->flags & SUB_EOD_MARKER)
		context->prev_start = sub->start_time;

	for (i = sub->nb_data - 1; i >= 0; i--)
	{
		if (rect[i].ocr_text && *(rect[i].ocr_text))
		{
			if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
			{
				char *buf = (char *)context->buffer;
				unsigned h1, m1, s1, ms1;
				unsigned h2, m2, s2, ms2;
				millis_to_time(sub->start_time, &h1, &m1, &s1, &ms1);
				millis_to_time(sub->end_time - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.
				int written = snprintf(buf, INITIAL_ENC_BUFFER_CAPACITY, "<p begin=\"%02u:%02u:%02u.%03u\" end=\"%02u:%02u:%02u.%03u\">\n", h1, m1, s1, ms1, h2, m2, s2, ms2);
				if (written > 0 && (size_t)written < INITIAL_ENC_BUFFER_CAPACITY)
					write_wrapped(context->out->fh, buf, written);
				len = strlen(rect[i].ocr_text);
				write_wrapped(context->out->fh, rect[i].ocr_text, len);
				write_wrapped(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
				written = snprintf(buf, INITIAL_ENC_BUFFER_CAPACITY, "</p>\n");
				if (written > 0 && (size_t)written < INITIAL_ENC_BUFFER_CAPACITY)
					write_wrapped(context->out->fh, buf, written);
			}
		}
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

int write_cc_subtitle_as_smptett(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
	struct cc_subtitle *osub = sub;
	struct cc_subtitle *lsub = sub;
	while (sub)
	{
		if (sub->type == CC_TEXT)
		{
			write_stringz_as_smptett(sub->data, context, sub->start_time, sub->end_time);
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

int write_cc_buffer_as_smptett(struct eia608_screen *data, struct encoder_ctx *context)
{
	int used;
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	int wrote_something = 0;
	char str[1024];

	// Write header on first caption (deferred file creation)
	if (write_subtitle_file_header(context, context->out) != 0)
		return -1;

	millis_to_time(data->start_time, &h1, &m1, &s1, &ms1);
	millis_to_time(data->end_time - 1, &h2, &m2, &s2, &ms2);

	for (int row = 0; row < 15; row++)
	{
		if (data->row_used[row])
		{
			float row1 = 0;
			float col1 = 0;
			int firstcol = -1;

			// ROWS is actually 90% of the screen size
			// Add +10% because row 0 is at position 10%
			row1 = ((100 * row) / (ROWS / 0.8)) + 10;

			for (int column = 0; column < COLUMNS; column++)
			{
				int unicode = 0;
				get_char_in_unicode((unsigned char *)&unicode, data->characters[row][column]);
				// if (COL_TRANSPARENT != data->colors[row][column])
				if (unicode != 0x20)
				{
					if (firstcol < 0)
					{
						firstcol = column;
					}
				}
			}
			// COLUMNS is actually 90% of the screen size
			// Add +10% because column 0 is at position 10%
			col1 = ((100 * firstcol) / (COLUMNS / 0.8)) + 10;

			if (firstcol >= 0)
			{
				wrote_something = 1;

				snprintf(str, sizeof(str), "      <p begin=\"%02u:%02u:%02u.%03u\" end=\"%02u:%02u:%02u.%03u\" tts:origin=\"%1.3f%% %1.3f%%\">\n        <span>", h1, m1, s1, ms1, h2, m2, s2, ms2, col1, row1);
				if (context->encoding != CCX_ENC_UNICODE)
				{
					dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
				}
				used = encode_line(context, context->buffer, (unsigned char *)str);
				write_wrapped(context->out->fh, context->buffer, used);
				// Trimming subs because the position is defined by "tts:origin"
				int old_trim_subs = context->trim_subs;
				context->trim_subs = 1;
				if (context->encoding != CCX_ENC_UNICODE)
				{
					dbg_print(CCX_DMT_DECODER_608, "\r");
					dbg_print(CCX_DMT_DECODER_608, "%s\n", context->subline);
				}

				get_decoder_line_encoded(context, context->subline, row, data);

				size_t subline_len = strlen((const char *)(context->subline));
				size_t buf_size = subline_len + 1000; // Being overly generous? :P
				char *final = malloc(buf_size);
				char *temp = malloc(buf_size);
				if (!final || !temp)
				{
					freep(&final);
					freep(&temp);
					fatal(EXIT_NOT_ENOUGH_MEMORY, "In write_cc_buffer_as_smptett() - not enough memory.\n");
				}
				*final = 0;
				*temp = 0;
				/*
					final	: stores formatted HTML sentence. This will be written in subtitle file.
					temp	: stored temporary sentences required while formatting

					+1000 because huge <span> and <style> tags are added. This will just prevent overflow (hopefully).
				*/

				int style = 0;

				/*

				0 = None or font colour
				1 = italics
				2 = bold
				3 = underline

				*/

				// Now, searching for first occurrence of <i> OR <u> OR <b>

				char *start = strstr((const char *)(context->subline), "<i>");
				if (start == NULL)
				{
					start = strstr((const char *)(context->subline), "<b>");

					if (start == NULL)
					{
						start = strstr((const char *)(context->subline), "<u>");
						style = 3; // underline
					}
					else
						style = 2; // bold
				}
				else
					style = 1; // italics

				if (start != NULL) // subtitle has style associated with it, will need formatting.
				{
					char *end_tag;
					if (style == 1)
					{
						end_tag = "</i>";
					}
					else if (style == 2)
					{
						end_tag = "</b>";
					}
					else
					{
						end_tag = "</u>";
					}

					char *end = strstr((const char *)(context->subline), end_tag); // occurrence of closing tag (</i> OR </b> OR </u>)

					if (end == NULL)
					{
						// Incorrect styling, writing as it is
						snprintf(final, buf_size, "%s", (const char *)(context->subline));
					}
					else
					{
						size_t final_len = 0;
						int start_index = start - (char *)(context->subline);
						int end_index = end - (char *)(context->subline);

						// copying content before opening tag e.g. <i>
						if (start_index > 0 && (size_t)start_index < buf_size - 1)
						{
							memcpy(final, (const char *)(context->subline), start_index);
							final[start_index] = '\0';
							final_len = start_index;
						}

						// adding <span> : replacement of <i>
						size_t remaining = buf_size - final_len;
						int written = snprintf(final + final_len, remaining, "<span>");
						if (written > 0 && (size_t)written < remaining)
							final_len += written;

						// The content in italics is between <i> and </i>, i.e. between (start_index + 3) and end_index.
						int content_len = end_index - start_index - 3;
						if (content_len > 0)
						{
							remaining = buf_size - final_len;
							if ((size_t)content_len < remaining - 1)
							{
								memcpy(final + final_len, (const char *)(context->subline) + start_index + 3, content_len);
								final_len += content_len;
								final[final_len] = '\0';
							}
						}

						// adding appropriate style tag
						remaining = buf_size - final_len;
						if (style == 1)
							written = snprintf(final + final_len, remaining, "<style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\" tts:fontStyle=\"italic\"/> </span>");
						else if (style == 2)
							written = snprintf(final + final_len, remaining, "<style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\" tts:fontWeight=\"bold\"/> </span>");
						else
							written = snprintf(final + final_len, remaining, "<style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\" tts:textDecoration=\"underline\"/> </span>");

						if (written > 0 && (size_t)written < remaining)
							final_len += written;

						// finding remaining sentence and adding it
						remaining = buf_size - final_len;
						snprintf(final + final_len, remaining, "%s", (const char *)(context->subline) + end_index + 4);
					}
				}
				else // No style or Font Color
				{

					start = strstr((const char *)(context->subline), "<font color"); // spec : <font color="#xxxxxx"> cc </font>
					if (start != NULL)						 // font color attribute is present
					{
						char *end = strstr((const char *)(context->subline), "</font>");
						if (end == NULL)
						{
							// Incorrect styling, writing as it is
							snprintf(final, buf_size, "%s", (const char *)(context->subline));
						}

						else
						{
							size_t final_len = 0;
							int start_index = start - (char *)(context->subline);
							int end_index = end - (char *)(context->subline);

							// copying content before opening tag e.g. <font ..>
							if (start_index > 0 && (size_t)start_index < buf_size - 1)
							{
								memcpy(final, (const char *)(context->subline), start_index);
								final[start_index] = '\0';
								final_len = start_index;
							}

							// adding <span> : replacement of <font ..>
							size_t remaining = buf_size - final_len;
							int written = snprintf(final + final_len, remaining, "<span>");
							if (written > 0 && (size_t)written < remaining)
								final_len += written;

							char *temp_pointer = strchr((const char *)(context->subline), '#'); // locating color code

							char color_code[8];
							if (temp_pointer)
							{
								snprintf(color_code, sizeof(color_code), "%.6s", temp_pointer + 1); // obtained color code
							}
							else
							{
								color_code[0] = '\0';
							}

							temp_pointer = strchr((const char *)(context->subline), '>'); // The content is in between <font ..> and </font>

							if (temp_pointer)
							{
								// Copy the content between <font ..> and </font>
								int content_len = end_index - (temp_pointer - (char *)(context->subline) + 1);
								if (content_len > 0)
								{
									remaining = buf_size - final_len;
									if ((size_t)content_len < remaining - 1)
									{
										memcpy(final + final_len, temp_pointer + 1, content_len);
										final_len += content_len;
										final[final_len] = '\0';
									}
								}
							}

							// adding font color tag
							remaining = buf_size - final_len;
							written = snprintf(final + final_len, remaining, "<style tts:backgroundColor=\"#FFFF00FF\" tts:color=\"%s\" tts:fontSize=\"18px\"/></span>", color_code);
							if (written > 0 && (size_t)written < remaining)
								final_len += written;

							// finding remaining sentence and adding it
							remaining = buf_size - final_len;
							snprintf(final + final_len, remaining, "%s", (const char *)(context->subline) + end_index + 7);
						}
					}

					else
					{
						// NO styling, writing as it is
						snprintf(final, buf_size, "%s", (const char *)(context->subline));
					}
				}

				write_wrapped(context->out->fh, final, strlen(final));

				write_wrapped(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
				context->trim_subs = old_trim_subs;

				snprintf(str, sizeof(str), "        <style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\"/></span>\n      </p>\n");
				if (context->encoding != CCX_ENC_UNICODE)
				{
					dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
				}
				used = encode_line(context, context->buffer, (unsigned char *)str);
				write_wrapped(context->out->fh, context->buffer, used);

				if (context->encoding != CCX_ENC_UNICODE)
				{
					dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
				}
				used = encode_line(context, context->buffer, (unsigned char *)str);
				// write (wb->fh, enc_buffer,enc_buffer_used);

				freep(&final);
				freep(&temp);
			}
		}
	}

	return wrote_something;
}
