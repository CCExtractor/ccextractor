#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "ccx_encoders_common.h"
#include "utility.h"
#include "ccx_encoders_helpers.h"
#include "ocr.h"
#include "ccextractor.h"

/* Helper function to write SRT to a specific output file (issue #665 - teletext multi-page)
   Takes output file descriptor and counter pointer as parameters */
static int write_stringz_as_srt_to_output(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end,
					  int out_fh, unsigned int *srt_counter)
{
	int used;
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	char timeline[128];

	if (!string || !string[0])
		return 0;

	millis_to_time(ms_start, &h1, &m1, &s1, &ms1);
	millis_to_time(ms_end - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.
	(*srt_counter)++;
	snprintf(timeline, sizeof(timeline), "%u%s", *srt_counter, context->encoded_crlf);
	used = encode_line(context, context->buffer, (unsigned char *)timeline);
	write_wrapped(out_fh, context->buffer, used);
	snprintf(timeline, sizeof(timeline), "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u%s",
		 h1, m1, s1, ms1, h2, m2, s2, ms2, context->encoded_crlf);
	used = encode_line(context, context->buffer, (unsigned char *)timeline);
	dbg_print(CCX_DMT_DECODER_608, "\n- - - SRT caption - - -\n");
	dbg_print(CCX_DMT_DECODER_608, "%s", timeline);

	write_wrapped(out_fh, context->buffer, used);
	int len = strlen(string);
	unsigned char *unescaped = (unsigned char *)malloc(len + 1);
	if (!unescaped)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In write_stringz_as_srt() - not enough memory for unescaped buffer.\n");
	unsigned char *el = (unsigned char *)malloc(len * 3 + 1); // Be generous
	if (!el)
	{
		free(unescaped);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In write_stringz_as_srt() - not enough memory for el buffer.\n");
	}
	int pos_r = 0;
	int pos_w = 0;
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
		write_wrapped(out_fh, el, u);
		write_wrapped(out_fh, context->encoded_crlf, context->encoded_crlf_length);
		begin += strlen((const char *)begin) + 1;
	}

	dbg_print(CCX_DMT_DECODER_608, "- - - - - - - - - - - -\r\n");

	write_wrapped(out_fh, context->encoded_crlf, context->encoded_crlf_length);
	free(el);
	free(unescaped);

	return 0;
}

/* The timing here is not PTS based, but output based, i.e. user delay must be accounted for
   if there is any */
int write_stringz_as_srt(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end)
{
	return write_stringz_as_srt_to_output(string, context, ms_start, ms_end,
					      context->out->fh, &context->srt_counter);
}

int write_cc_bitmap_as_srt(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
#ifdef ENABLE_OCR
	struct cc_bitmap *rect;
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	char timeline[128];
	int len = 0;
	int used;
	int i = 0;
	char *str;

	if (sub->nb_data == 0)
		return 0;

	if (sub->flags & SUB_EOD_MARKER)
		context->prev_start = sub->start_time;

	str = paraof_ocrtext(sub, context);
	if (str)
	{
		if (context->is_mkv == 1)
		{
			// Save recognized string for later use in matroska.c
			context->last_str = str;
		}
		else
		{
			if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
			{
				millis_to_time(sub->start_time, &h1, &m1, &s1, &ms1);
				millis_to_time(sub->end_time - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.
				context->srt_counter++;
				snprintf(timeline, sizeof(timeline), "%u%s", context->srt_counter, context->encoded_crlf);
				used = encode_line(context, context->buffer, (unsigned char *)timeline);
				write_wrapped(context->out->fh, context->buffer, used);
				snprintf(timeline, sizeof(timeline), "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u%s",
					 h1, m1, s1, ms1, h2, m2, s2, ms2, context->encoded_crlf);
				used = encode_line(context, context->buffer, (unsigned char *)timeline);
				write_wrapped(context->out->fh, context->buffer, used);
				len = strlen(str);
				write_wrapped(context->out->fh, str, len);
				write_wrapped(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
			}
			freep(&str);
		}
	}
	for (i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		if (rect)
		{
			freep(&rect->data0);
			freep(&rect->data1);
		}
	}
#endif
	sub->nb_data = 0;
	freep(&sub->data);
	return ret;
}

int write_cc_subtitle_as_srt(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
	struct cc_subtitle *osub = sub;
	struct cc_subtitle *lsub = sub;

	while (sub)
	{
		if (sub->type == CC_TEXT)
		{
			// For teletext multi-page extraction (issue #665), use page-specific output
			struct ccx_s_write *out = get_teletext_output(context, sub->teletext_page);
			unsigned int *counter = get_teletext_srt_counter(context, sub->teletext_page);
			if (out && counter)
			{
				ret = write_stringz_as_srt_to_output(sub->data, context, sub->start_time, sub->end_time,
								     out->fh, counter);
			}
			else
			{
				ret = write_stringz_as_srt(sub->data, context, sub->start_time, sub->end_time);
			}
			freep(&sub->data);
			sub->nb_data = 0;
			ret = 1;
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

int write_cc_buffer_as_srt(struct eia608_screen *data, struct encoder_ctx *context)
{
	int used;
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	int wrote_something = 0;

	int prev_line_start = -1, prev_line_end = -1;	    // Column in which the previous line started and ended, for autodash
	int prev_line_center1 = -1, prev_line_center2 = -1; // Center column of previous line text

	int empty_buf = 1;
	for (int i = 0; i < 15; i++)
	{
		if (data->row_used[i])
		{
			empty_buf = 0;
			break;
		}
	}
	if (empty_buf) // Prevent writing empty screens. Not needed in .srt
		return 0;

	millis_to_time(data->start_time, &h1, &m1, &s1, &ms1);
	millis_to_time(data->end_time - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.

	char timeline[128];

	++context->srt_counter;
	snprintf(timeline, sizeof(timeline), "%u%s", context->srt_counter, context->encoded_crlf);
	used = encode_line(context, context->buffer, (unsigned char *)timeline);
	write_wrapped(context->out->fh, context->buffer, used);

	snprintf(timeline, sizeof(timeline), "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u%s",
		 h1, m1, s1, ms1, h2, m2, s2, ms2, context->encoded_crlf);
	used = encode_line(context, context->buffer, (unsigned char *)timeline);
	write_wrapped(context->out->fh, context->buffer, used);

	dbg_print(CCX_DMT_DECODER_608, "\n- - - SRT caption ( %d) - - -\n", context->srt_counter);
	dbg_print(CCX_DMT_DECODER_608, "%s", timeline);

	for (int i = 0; i < 15; i++)
	{
		if (data->row_used[i])
		{
			if (context->autodash && context->trim_subs)
			{
				int first = 0, last = 31, center1 = -1, center2 = -1;
				unsigned char *line = data->characters[i];
				int do_dash = 1, colon_pos = -1;
				find_limit_characters(line, &first, &last, CCX_DECODER_608_SCREEN_WIDTH);
				if (first == -1 || last == -1) // Probably a bug somewhere though
					break;
				// Is there a speaker named, for example: TOM: What are you doing?
				for (int j = first; j <= last; j++)
				{
					if (line[j] == ':')
					{
						colon_pos = j;
						break;
					}
					if (!isupper(line[j]))
						break;
				}
				if (prev_line_start == -1)
					do_dash = 0;
				if (first == prev_line_start) // Case of left alignment
					do_dash = 0;
				if (last == prev_line_end) // Right align
					do_dash = 0;
				if (first > prev_line_start && last < prev_line_end) // Fully contained
					do_dash = 0;
				if ((first > prev_line_start && first < prev_line_end) || // Overlap
				    (last > prev_line_start && last < prev_line_end))
					do_dash = 0;

				center1 = (first + last) / 2;
				if (colon_pos != -1)
				{
					while (colon_pos < CCX_DECODER_608_SCREEN_WIDTH &&
					       (line[colon_pos] == ':' ||
						line[colon_pos] == ' ' ||
						line[colon_pos] == 0x89))
						colon_pos++; // Find actual text
					center2 = (colon_pos + last) / 2;
				}
				else
					center2 = center1;

				if (center1 >= prev_line_center1 - 1 && center1 <= prev_line_center1 + 1 && center1 != -1) // Center align
					do_dash = 0;
				if (center2 >= prev_line_center2 - 2 && center1 <= prev_line_center2 + 2 && center1 != -1) // Center align
					do_dash = 0;

				if (do_dash)
					write_wrapped(context->out->fh, "- ", 2);
				prev_line_start = first;
				prev_line_end = last;
				prev_line_center1 = center1;
				prev_line_center2 = center2;
			}
			int length = get_decoder_line_encoded(context, context->subline, i, data);
			if (context->encoding != CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r");
				dbg_print(CCX_DMT_DECODER_608, "%s\n", context->subline);
			}
			write_wrapped(context->out->fh, context->subline, length);
			write_wrapped(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
			wrote_something = 1;
			// fprintf (wb->fh,context->encoded_crlf);
		}
	}
	dbg_print(CCX_DMT_DECODER_608, "- - - - - - - - - - - -\r\n");

	// fprintf (wb->fh, context->encoded_crlf);
	write_wrapped(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
	// printf("$ = %s\n",context->encoded_crlf);
	return wrote_something;
}
