#include "ccx_decoders_common.h"
#include "ccx_encoders_common.h"
#include "utility.h"
#include "ocr.h"
#include "ccx_decoders_608.h"
#include "ccx_decoders_708.h"
#include "ccx_decoders_708_output.h"
#include "ccx_encoders_xds.h"
#include "ccx_encoders_helpers.h"
#include "lib_ccx.h"

#ifdef ENABLE_SHARING
#include "ccx_share.h"
#endif //ENABLE_SHARING

int write_cc_bitmap_as_transcript(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
#ifdef ENABLE_OCR
	struct cc_bitmap* rect;

	LLONG start_time, end_time;
	
	start_time = sub->start_time;
	end_time = sub->end_time;

	if (sub->nb_data == 0)
		return ret;
	rect = sub->data;

	if (sub->flags & SUB_EOD_MARKER)
		context->prev_start = sub->start_time;


	if (rect[0].ocr_text && *(rect[0].ocr_text))
	{
		if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
		{
			char *token = NULL;
			token = paraof_ocrtext(sub, context->encoded_crlf, context->encoded_crlf_length);
			if (context->transcript_settings->showStartTime)
			{
				char buf1[80];
				if (context->transcript_settings->relativeTimestamp)
				{
					millis_to_date(start_time + context->subs_delay, buf1, context->date_format, context->millis_separator);
					fdprintf(context->out->fh, "%s|", buf1);
				}
				else
				{
					time_t start_time_int = (start_time + context->subs_delay) / 1000;
					int start_time_dec = (start_time + context->subs_delay) % 1000;
					struct tm *start_time_struct = gmtime(&start_time_int);
					strftime(buf1, sizeof(buf1), "%Y%m%d%H%M%S", start_time_struct);
					fdprintf(context->out->fh, "%s%c%03d|", buf1, context->millis_separator, start_time_dec);
				}
			}

			if (context->transcript_settings->showEndTime)
			{
				char buf2[80];
				if (context->transcript_settings->relativeTimestamp)
				{
					millis_to_date(end_time + context->subs_delay, buf2, context->date_format, context->millis_separator);
					fdprintf(context->out->fh, "%s|", buf2);
				}
				else
				{
					time_t end_time_int = (end_time + context->subs_delay) / 1000;
					int end_time_dec = (end_time + context->subs_delay) % 1000;
					struct tm *end_time_struct = gmtime(&end_time_int);
					strftime(buf2, sizeof(buf2), "%Y%m%d%H%M%S", end_time_struct);
					fdprintf(context->out->fh, "%s%c%03d|", buf2, context->millis_separator, end_time_dec);
				}
			}
			if (context->transcript_settings->showCC)
			{
				fdprintf(context->out->fh, "%s|", language[sub->lang_index]);
			}
			if (context->transcript_settings->showMode)
			{
				fdprintf(context->out->fh, "DVB|");
			}

			while (token)
			{
				char *newline_pos = strstr(token, context->encoded_crlf);
				if (!newline_pos)
				{
					fdprintf(context->out->fh, "%s", token);
					break;
				}
				else
				{
					while (token != newline_pos)
					{
						fdprintf(context->out->fh, "%c", *token);
						token++;
					}
					token += context->encoded_crlf_length;
					fdprintf(context->out->fh, "%c", ' ');
				}
			}

			write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);

		}
	}
#endif

	sub->nb_data = 0;
	freep(&sub->data);
	return ret;

}


int write_cc_subtitle_as_transcript(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int length;
	int ret = 0;
	LLONG start_time = -1;
	LLONG end_time = -1;
	char *str;
	char *save_str;
	struct cc_subtitle *osub = sub;
	struct cc_subtitle *lsub = sub;

	while (sub)
	{
		if (sub->type == CC_TEXT)
		{
			start_time = sub->start_time;
			end_time = sub->end_time;
		}
		if (context->sentence_cap)
		{
			//TODO capitalize (context, line_number,data);
			//TODO correct_case_with_dictionary(line_number, data);
		}

		if (start_time == -1)
		{
			// CFS: Means that the line has characters but we don't have a timestamp for the first one. Since the timestamp
			// is set for example by the write_char function, it possible that we don't have one in empty lines (unclear)
			// For now, let's not consider this a bug as before and just return.
			// fatal (EXIT_BUG_BUG, "Bug in timedtranscript (ts_start_of_current_line==-1). Please report.");
			return 0;
		}

		str = sub->data;

		str = strtok_r(str, "\r\n", &save_str);
		do
		{
			length = get_str_basic(context->subline, (unsigned char*)str, context->trim_subs, sub->enc_type, context->encoding, strlen(str));
			if (length <= 0)
			{
				continue;
			}

			if (context->transcript_settings->showStartTime)
			{
				char buf[80];
				if (context->transcript_settings->relativeTimestamp)
				{
					millis_to_date(start_time + context->subs_delay, buf, context->date_format, context->millis_separator);
					fdprintf(context->out->fh, "%s|", buf);
				}
				else
				{
					time_t start_time_int = (start_time + context->subs_delay) / 1000;
					int start_time_dec = (start_time + context->subs_delay) % 1000;
					struct tm *start_time_struct = gmtime(&start_time_int);
					strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", start_time_struct);
					fdprintf(context->out->fh, "%s%c%03d|", buf, context->millis_separator, start_time_dec);
				}
			}

			if (context->transcript_settings->showEndTime)
			{
				char buf[80];
				if (context->transcript_settings->relativeTimestamp)
				{
					millis_to_date(end_time + context->subs_delay, buf, context->date_format, context->millis_separator);
					fdprintf(context->out->fh, "%s|", buf);
				}
				else
				{
					time_t end_time_int = (end_time + context->subs_delay) / 1000;
					int end_time_dec = (end_time + context->subs_delay) % 1000;
					struct tm *end_time_struct = gmtime(&end_time_int);
					strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", end_time_struct);
					fdprintf(context->out->fh, "%s%c%03d|", buf, context->millis_separator, end_time_dec);
				}
			}

			if (context->transcript_settings->showCC)
			{
				if (!context->ucla || !strcmp(sub->mode, "TLT"))
					fdprintf(context->out->fh, sub->info);				
				else if (context->in_fileformat == 1)
					//TODO, data->my_field == 1 ? data->channel : data->channel + 2); // Data from field 2 is CC3 or 4
					fdprintf(context->out->fh, "CC?|");
			}
			if (context->transcript_settings->showMode)
			{
				if (context->ucla && strcmp(sub->mode, "TLT") == 0)
					fdprintf(context->out->fh, "|");
				else
					fdprintf(context->out->fh, "%s|", sub->mode);
			}
			ret = write(context->out->fh, context->subline, length);
			if (ret < length)
			{
				mprint("Warning:Loss of data\n");
			}

			ret = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
			if (ret <  context->encoded_crlf_length)
			{
				mprint("Warning:Loss of data\n");
			}

		} while ((str = strtok_r(NULL, "\r\n", &save_str)));

		freep(&sub->data);
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


//TODO Convert CC line to TEXT format and remove this function
void write_cc_line_as_transcript2(struct eia608_screen *data, struct encoder_ctx *context, int line_number)
{
	int ret = 0;
	LLONG start_time = data->start_time;
	LLONG end_time = data->end_time;
	if (context->sentence_cap)
	{
		if (clever_capitalize(context, line_number, data))
			correct_case_with_dictionary(line_number, data);
	}
	int length = get_str_basic(context->subline, data->characters[line_number],
		context->trim_subs, CCX_ENC_ASCII, context->encoding, CCX_DECODER_608_SCREEN_WIDTH);

	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r");
		dbg_print(CCX_DMT_DECODER_608, "%s\n", context->subline);
	}
	if (length>0)
	{
		if (data->start_time == -1)
		{
			// CFS: Means that the line has characters but we don't have a timestamp for the first one. Since the timestamp
			// is set for example by the write_char function, it possible that we don't have one in empty lines (unclear)
			// For now, let's not consider this a bug as before and just return.
			// fatal (EXIT_BUG_BUG, "Bug in timedtranscript (ts_start_of_current_line==-1). Please report.");
			return;
		}

		if (context->transcript_settings->showStartTime){
			char buf1[80];
			if (context->transcript_settings->relativeTimestamp){
				millis_to_date(start_time + context->subs_delay, buf1, context->date_format, context->millis_separator);
				fdprintf(context->out->fh, "%s|", buf1);
			}
			else {
				time_t start_time_int = (start_time + context->subs_delay) / 1000;
				int start_time_dec = (start_time + context->subs_delay) % 1000;
				struct tm *start_time_struct = gmtime(&start_time_int);
				strftime(buf1, sizeof(buf1), "%Y%m%d%H%M%S", start_time_struct);
				fdprintf(context->out->fh, "%s%c%03d|", buf1, context->millis_separator, start_time_dec);
			}
		}

		if (context->transcript_settings->showEndTime){
			char buf2[80];
			if (context->transcript_settings->relativeTimestamp){
				millis_to_date(end_time, buf2, context->date_format, context->millis_separator);
				fdprintf(context->out->fh, "%s|", buf2);
			}
			else {
				time_t end_time_int = end_time / 1000;
				int end_time_dec = end_time % 1000;
				struct tm *end_time_struct = gmtime(&end_time_int);
				strftime(buf2, sizeof(buf2), "%Y%m%d%H%M%S", end_time_struct);
				fdprintf(context->out->fh, "%s%c%03d|", buf2, context->millis_separator, end_time_dec);
			}
		}

		if (context->transcript_settings->showCC){
			fdprintf(context->out->fh, "CC%d|", data->my_field == 1 ? data->channel : data->channel + 2); // Data from field 2 is CC3 or 4
		}
		if (context->transcript_settings->showMode){
			const char *mode = "???";
			switch (data->mode)
			{
				case MODE_POPON:
					mode = "POP";
					break;
				case MODE_FAKE_ROLLUP_1:
					mode = "RU1";
					break;
				case MODE_ROLLUP_2:
					mode = "RU2";
					break;
				case MODE_ROLLUP_3:
					mode = "RU3";
					break;
				case MODE_ROLLUP_4:
					mode = "RU4";
					break;
				case MODE_TEXT:
					mode = "TXT";
					break;
				case MODE_PAINTON:
					mode = "PAI";
					break;
			}
			fdprintf(context->out->fh, "%s|", mode);
		}

		ret = write(context->out->fh, context->subline, length);
		if (ret < length)
		{
			mprint("Warning:Loss of data\n");
		}

		ret = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
		if (ret < context->encoded_crlf_length)
		{
			mprint("Warning:Loss of data\n");
		}
	}
	// fprintf (wb->fh,encoded_crlf);
}

int write_cc_buffer_as_transcript2(struct eia608_screen *data, struct encoder_ctx *context)
{
	int wrote_something = 0;
	dbg_print(CCX_DMT_DECODER_608, "\n- - - TRANSCRIPT caption - - -\n");

	for (int i = 0; i<15; i++)
	{
		if (data->row_used[i])
		{
			write_cc_line_as_transcript2(data, context, i);
		}
		wrote_something = 1;
	}
	dbg_print(CCX_DMT_DECODER_608, "- - - - - - - - - - - -\r\n");
	return wrote_something;
}
