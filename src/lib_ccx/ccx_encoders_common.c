#include "ccx_decoders_common.h"
#include "ccx_encoders_common.h"
#include "spupng_encoder.h"
#include "608_spupng.h"
#include "utility.h"
#include "ocr.h"
#include "ccx_decoders_608.h"
#include "ccx_decoders_xds.h"

// These are the default settings for plain transcripts. No times, no CC or caption mode, and no XDS.
ccx_encoders_transcript_format ccx_encoders_default_transcript_settings =
{
	.showStartTime = 0,
	.showEndTime = 0,
	.showMode = 0,
	.showCC = 0,
	.relativeTimestamp = 1,
	.xds = 0,
	.useColors = 1,
	.isFinal = 0
};

static const char *sami_header= // TODO: Revise the <!-- comments
"<SAMI>\n\
<HEAD>\n\
<STYLE TYPE=\"text/css\">\n\
<!--\n\
P {margin-left: 16pt; margin-right: 16pt; margin-bottom: 16pt; margin-top: 16pt;\n\
text-align: center; font-size: 18pt; font-family: arial; font-weight: bold; color: #f0f0f0;}\n\
.UNKNOWNCC {Name:Unknown; lang:en-US; SAMIType:CC;}\n\
-->\n\
</STYLE>\n\
</HEAD>\n\n\
<BODY>\n";

static const char *smptett_header = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
"<tt xmlns:ttm=\"http://www.w3.org/ns/ttml#metadata\" xmlns:tts=\"http://www.w3.org/ns/ttml#styling\" xmlns=\"http://www.w3.org/ns/ttml\" xml:lang=\"en\">\n"
"  <head>\n"
"    <styling>\n"
"      <style xml:id=\"speakerStyle\" tts:fontFamily=\"proportionalSansSerif\" tts:fontSize=\"150%\" tts:textAlign=\"center\" tts:displayAlign=\"center\" tts:color=\"white\" tts:textOutline=\"black 1px\"/>\n"
"    </styling>\n"
"    <layout>\n"
"      <region xml:id=\"speaker\" tts:origin=\"10% 80%\" tts:extent=\"80% 10%\" style=\"speakerStyle\"/>\n"
"    </layout>\n"
"  </head>\n"
"  <body>\n"
"    <div>\n";
void write_subtitle_file_footer(struct encoder_ctx *ctx,struct ccx_s_write *out)
{
	int used;
	switch (ctx->write_format)
	{
		case CCX_OF_SAMI:
			sprintf ((char *) str,"</BODY></SAMI>\n");
			if (ctx->encoding!=CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
			}
			used=encode_line (ctx->buffer,(unsigned char *) str);
			write(out->fh, ctx->buffer, used);
			break;
		case CCX_OF_SMPTETT:
			sprintf ((char *) str,"    </div>\n  </body>\n</tt>\n");
			if (ctx->encoding!=CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
			}
			used=encode_line (ctx->buffer,(unsigned char *) str);
			write (out->fh, ctx->buffer,used);
			break;
		case CCX_OF_SPUPNG:
			write_spumux_footer(out);
			break;
		default: // Nothing to do, no footer on this format
			break;
	}
}

void write_subtitle_file_header(struct encoder_ctx *ctx,struct ccx_s_write *out)
{
	int used;
	switch (ctx->write_format)
	{
		case CCX_OF_SRT: // Subrip subtitles have no header
			break;
		case CCX_OF_SAMI: // This header brought to you by McPoodle's CCASDI
			//fprintf_encoded (wb->fh, sami_header);
			REQUEST_BUFFER_CAPACITY(ctx,strlen (sami_header)*3);
			used=encode_line (ctx->buffer,(unsigned char *) sami_header);
			write (out->fh, ctx->buffer,used);
			break;
		case CCX_OF_SMPTETT: // This header brought to you by McPoodle's CCASDI
			//fprintf_encoded (wb->fh, sami_header);
			REQUEST_BUFFER_CAPACITY(ctx,strlen (smptett_header)*3);
			used=encode_line (ctx->buffer,(unsigned char *) smptett_header);
			write(out->fh, ctx->buffer, used);
			break;
		case CCX_OF_RCWT: // Write header
			if (ctx->teletext_mode == CCX_TXT_IN_USE)
				rcwt_header[7] = 2; // sets file format version

			if (ctx->send_to_srv)
				net_send_header(rcwt_header, sizeof(rcwt_header));
			else
				write(out->fh, rcwt_header, sizeof(rcwt_header));

			break;
		case CCX_OF_SPUPNG:
			write_spumux_header(out);
			break;
		case CCX_OF_TRANSCRIPT: // No header. Fall thru
		default:
			break;
	}
}


void write_cc_line_as_transcript2(struct eia608_screen *data, struct encoder_ctx *context, int line_number)
{
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	LLONG start_time = data->start_time;
	LLONG end_time = data->end_time;
	if (context->sentence_cap)
	{
		capitalize (line_number,data);
		correct_case(line_number,data);
	}
	int length = get_decoder_line_basic (subline, line_number, data, context->trim_subs, context->encoding);
	if (context->encoding!=CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r");
		dbg_print(CCX_DMT_DECODER_608, "%s\n",subline);
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
				mstotime(start_time + context->subs_delay, &h1, &m1, &s1, &ms1);
				time_t start_time_int = (start_time + context->subs_delay) / 1000;
				int start_time_dec = (start_time + context->subs_delay) % 1000;
				struct tm *start_time_struct = gmtime(&start_time_int);
				strftime(buf1, sizeof(buf1), "%Y%m%d%H%M%S", start_time_struct);
				fdprintf(context->out->fh, "%s%c%03d|", buf1,context->millis_separator,start_time_dec);
			}
		}

		if (context->transcript_settings->showEndTime){
			char buf2[80];
			if (context->transcript_settings->relativeTimestamp){
				millis_to_date(end_time, buf2, context->date_format, context->millis_separator);
				fdprintf(context->out->fh, "%s|", buf2);
			}
			else {
				mstotime(get_fts() + context->subs_delay, &h2, &m2, &s2, &ms2);
				time_t end_time_int = (end_time + context->subs_delay) / 1000;
				int end_time_dec = (end_time + context->subs_delay) % 1000;
				struct tm *end_time_struct = gmtime(&end_time_int);
				strftime(buf2, sizeof(buf2), "%Y%m%d%H%M%S", end_time_struct);
				fdprintf(context->out->fh, "%s%c%03d|", buf2,context->millis_separator,end_time_dec);
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

		write(context->out->fh, subline, length);
		write(context->out->fh, encoded_crlf, encoded_crlf_length);
	}
	// fprintf (wb->fh,encoded_crlf);
}

int write_cc_buffer_as_transcript2(struct eia608_screen *data, struct encoder_ctx *context)
{
	int wrote_something = 0;
	dbg_print(CCX_DMT_DECODER_608, "\n- - - TRANSCRIPT caption - - -\n");

	for (int i=0;i<15;i++)
	{
		if (data->row_used[i])
		{
			write_cc_line_as_transcript2 (data, context, i);
		}
		wrote_something=1;
	}
	dbg_print(CCX_DMT_DECODER_608, "- - - - - - - - - - - -\r\n");
	return wrote_something;
}
int write_cc_bitmap_as_transcript(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
	struct cc_bitmap* rect;

#ifdef ENABLE_OCR
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
#endif
	LLONG start_time, end_time;

	if (context->prev_start != -1 && (sub->flags & SUB_EOD_MARKER))
	{
		start_time = context->prev_start + context->subs_delay;
		end_time = sub->start_time - 1;
	}
	else if ( !(sub->flags & SUB_EOD_MARKER))
	{
		start_time = sub->start_time + context->subs_delay;
		end_time = sub->end_time - 1;
	}

	if(sub->nb_data == 0 )
		return ret;
	rect = sub->data;

	if ( sub->flags & SUB_EOD_MARKER )
		context->prev_start =  sub->start_time;

#ifdef ENABLE_OCR
	if (rect[0].ocr_text && *(rect[0].ocr_text))
	{
		if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
		{
			char *token = NULL;
			token = strtok(rect[0].ocr_text ,"\r\n");
			while (token)
			{

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
						mstotime(start_time + context->subs_delay, &h1, &m1, &s1, &ms1);
						time_t start_time_int = (start_time + context->subs_delay) / 1000;
						int start_time_dec = (start_time + context->subs_delay) % 1000;
						struct tm *start_time_struct = gmtime(&start_time_int);
						strftime(buf1, sizeof(buf1), "%Y%m%d%H%M%S", start_time_struct);
						fdprintf(context->out->fh, "%s%c%03d|", buf1,context->millis_separator,start_time_dec);
					}
				}

				if (context->transcript_settings->showEndTime)
				{
					char buf2[80];
					if (context->transcript_settings->relativeTimestamp)
					{
						millis_to_date(end_time, buf2, context->date_format, context->millis_separator);
						fdprintf(context->out->fh, "%s|", buf2);
					}
					else
					{
						mstotime(get_fts() + context->subs_delay, &h2, &m2, &s2, &ms2);
						time_t end_time_int = end_time / 1000;
						int end_time_dec = end_time % 1000;
						struct tm *end_time_struct = gmtime(&end_time_int);
						strftime(buf2, sizeof(buf2), "%Y%m%d%H%M%S", end_time_struct);
						fdprintf(context->out->fh, "%s%c%03d|", buf2,context->millis_separator,end_time_dec);
					}
				}
				if (context->transcript_settings->showCC)
				{
					fdprintf(context->out->fh,"%s|",language[sub->lang_index]);
				}
				if (context->transcript_settings->showMode)
				{
					fdprintf(context->out->fh,"DVB|");
				}
				fdprintf(context->out->fh,"%s\n",token);
				token = strtok(NULL,"\r\n");

			}

		}
	}
#endif

	sub->nb_data = 0;
	freep(&sub->data);
	return ret;

}
void try_to_add_end_credits(struct encoder_ctx *context, struct ccx_s_write *out)
{
	LLONG window, length, st, end;
	if (out->fh == -1)
		return;
	window=get_fts()-context->last_displayed_subs_ms-1;
	if (window < context->endcreditsforatleast.time_in_ms) // Won't happen, window is too short
		return;
	length=context->endcreditsforatmost.time_in_ms > window ?
		window : context->endcreditsforatmost.time_in_ms;

	st=get_fts()-length-1;
	end=get_fts();

	switch (context->write_format)
	{
		case CCX_OF_SRT:
			write_stringz_as_srt(context->end_credits_text, context, st, end);
			break;
		case CCX_OF_SAMI:
			write_stringz_as_sami(context->end_credits_text, context, st, end);
			break;
		case CCX_OF_SMPTETT:
			write_stringz_as_smptett(context->end_credits_text, context, st, end);
			break ;
		default:
			// Do nothing for the rest
			break;
	}
}

void try_to_add_start_credits(struct encoder_ctx *context,LLONG start_ms)
{
	LLONG st, end, window, length;
	LLONG l = start_ms + context->subs_delay;
	// We have a windows from last_displayed_subs_ms to l - we need to see if it fits

	if (l < context->startcreditsnotbefore.time_in_ms) // Too early
		return;

	if (context->last_displayed_subs_ms+1 > context->startcreditsnotafter.time_in_ms) // Too late
		return;

	st = context->startcreditsnotbefore.time_in_ms>(context->last_displayed_subs_ms+1) ?
		context->startcreditsnotbefore.time_in_ms : (context->last_displayed_subs_ms+1); // When would credits actually start

	end = context->startcreditsnotafter.time_in_ms<(l-1) ?
		context->startcreditsnotafter.time_in_ms : (l-1);

	window = end-st; // Allowable time in MS

	if (context->startcreditsforatleast.time_in_ms>window) // Window is too short
		return;

	length=context->startcreditsforatmost.time_in_ms > window ?
		window : context->startcreditsforatmost.time_in_ms;

	dbg_print(CCX_DMT_VERBOSE, "Last subs: %lld   Current position: %lld\n",
			context->last_displayed_subs_ms, l);
	dbg_print(CCX_DMT_VERBOSE, "Not before: %lld   Not after: %lld\n",
			context->startcreditsnotbefore.time_in_ms,
			context->startcreditsnotafter.time_in_ms);
	dbg_print(CCX_DMT_VERBOSE, "Start of window: %lld   End of window: %lld\n",st,end);

	if (window>length+2)
	{
		// Center in time window
		LLONG pad=window-length;
		st+=(pad/2);
	}
	end=st+length;
	switch (context->write_format)
	{
		case CCX_OF_SRT:
			write_stringz_as_srt(context->start_credits_text,context,st,end);
			break;
		case CCX_OF_SAMI:
			write_stringz_as_sami(context->start_credits_text, context, st, end);
			break;
		case CCX_OF_SMPTETT:
			write_stringz_as_smptett(context->start_credits_text, context, st, end);
			break;
		default:
			// Do nothing for the rest
			break;
	}
	context->startcredits_displayed=1;
	return;


}

int init_encoder(struct encoder_ctx *ctx, struct ccx_s_write *out, struct ccx_s_options *opt)
{
	ctx->buffer = (unsigned char *) malloc (INITIAL_ENC_BUFFER_CAPACITY);
	if (ctx->buffer==NULL)
		return -1;
	ctx->capacity=INITIAL_ENC_BUFFER_CAPACITY;
	ctx->srt_counter = 0;
	ctx->out = out;
	/** used in case of SUB_EOD_MARKER */
	ctx->prev_start = -1;

	ctx->encoding = opt->encoding;
	ctx->date_format = opt->date_format;
	ctx->millis_separator = opt->millis_separator;
	ctx->sentence_cap = opt->sentence_cap;
	ctx->autodash = opt->autodash;
	ctx->trim_subs = opt->trim_subs;
	ctx->write_format = opt->write_format;
	ctx->start_credits_text = opt->start_credits_text;
	ctx->end_credits_text = opt->end_credits_text;
	ctx->transcript_settings = &opt->transcript_settings;
	ctx->startcreditsnotbefore = opt->startcreditsnotbefore;
	ctx->startcreditsnotafter = opt->startcreditsnotafter;
	ctx->startcreditsforatleast = opt->startcreditsforatleast;
	ctx->startcreditsforatmost = opt->startcreditsforatmost;
	ctx->endcreditsforatleast = opt->endcreditsforatleast;
	ctx->endcreditsforatmost = opt->endcreditsforatmost;
	ctx->teletext_mode = opt->teletext_mode;
	ctx->send_to_srv = opt->send_to_srv;
	ctx->gui_mode_reports = opt->gui_mode_reports;
	write_subtitle_file_header(ctx,out);

	return 0;

}
void set_encoder_last_displayed_subs_ms(struct encoder_ctx *ctx, LLONG last_displayed_subs_ms)
{
	ctx->last_displayed_subs_ms = last_displayed_subs_ms;
}
void set_encoder_subs_delay(struct encoder_ctx *ctx, LLONG subs_delay)
{
	ctx->subs_delay = subs_delay;
}
void set_encoder_startcredits_displayed(struct encoder_ctx *ctx, int startcredits_displayed)
{
	ctx->startcredits_displayed = startcredits_displayed;
}
void dinit_encoder(struct encoder_ctx *ctx)
{

	if (ctx->end_credits_text!=NULL)
		try_to_add_end_credits(ctx,ctx->out);
	write_subtitle_file_footer(ctx,ctx->out);
	freep(&ctx->buffer);
	ctx->capacity = 0;
}

int encode_sub(struct encoder_ctx *context, struct cc_subtitle *sub)
{
	int wrote_something = 0;

	if (sub->type == CC_608)
	{
		struct eia608_screen *data = NULL;
		for(data = sub->data; sub->nb_data ; sub->nb_data--,data++)
		{
			// Determine context based on channel. This replaces the code that was above, as this was incomplete (for cases where -12 was used for example)
			if (data->my_field == 2)
				context++;

			new_sentence=1;

			if(data->format == SFORMAT_XDS)
			{
				xds_write_transcript_line_prefix (context->out, data->start_time, data->end_time,data->cur_xds_packet_class);
				if(data->xds_len > 0)
					write (context->out->fh, data->xds_str,data->xds_len);
				freep (&data->xds_str);
				xds_write_transcript_line_suffix (context->out);
				continue;
			}

			if(!data->start_time)
				break;
			switch (context->write_format)
			{
			case CCX_OF_SRT:
				if (!context->startcredits_displayed && context->start_credits_text!=NULL)
					try_to_add_start_credits(context, data->start_time);
				wrote_something = write_cc_buffer_as_srt(data, context);
				break;
			case CCX_OF_SAMI:
				if (!context->startcredits_displayed && context->start_credits_text!=NULL)
					try_to_add_start_credits(context, data->start_time);
				wrote_something = write_cc_buffer_as_sami(data, context);
				break;
			case CCX_OF_SMPTETT:
				if (!context->startcredits_displayed && context->start_credits_text!=NULL)
					try_to_add_start_credits(context, data->start_time);
				wrote_something = write_cc_buffer_as_smptett(data, context);
				break;
			case CCX_OF_TRANSCRIPT:
				wrote_something = write_cc_buffer_as_transcript2(data, context);
				break;
			case CCX_OF_SPUPNG:
				wrote_something = write_cc_buffer_as_spupng(data, context);
				break;
			default:
				break;
			}
			if (wrote_something)
				context->last_displayed_subs_ms=get_fts() + context->subs_delay;

			if (context->gui_mode_reports)
				write_cc_buffer_to_gui(sub->data, context);
		}
		freep(&sub->data);
	}
	if(sub->type == CC_BITMAP)
	{
		switch (context->write_format)
		{
		case CCX_OF_SRT:
			if (!context->startcredits_displayed && context->start_credits_text!=NULL)
				try_to_add_start_credits(context, sub->start_time);
			wrote_something = write_cc_bitmap_as_srt(sub, context);
		case CCX_OF_SAMI:
			if (!context->startcredits_displayed && context->start_credits_text!=NULL)
				try_to_add_start_credits(context, sub->start_time);
			wrote_something = write_cc_bitmap_as_sami(sub, context);
		case CCX_OF_SMPTETT:
			if (!context->startcredits_displayed && context->start_credits_text!=NULL)
				try_to_add_start_credits(context, sub->start_time);
			wrote_something = write_cc_bitmap_as_smptett(sub, context);
		case CCX_OF_TRANSCRIPT:
			wrote_something = write_cc_bitmap_as_transcript(sub, context);
			break;
		case CCX_OF_SPUPNG:
			wrote_something = write_cc_bitmap_as_spupng(sub, context);
			break;
		default:
			break;
		}

	}
	if (!sub->nb_data)
		freep(&sub->data);
	return wrote_something;
}

void write_cc_buffer_to_gui(struct eia608_screen *data, struct encoder_ctx *context)
{
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	LLONG ms_start;
	int with_data = 0;

	for (int i = 0; i<15; i++)
	{
		if (data->row_used[i])
			with_data = 1;
	}
	if (!with_data)
		return;

	ms_start = data->start_time;

	ms_start += context->subs_delay;
	if (ms_start<0) // Drop screens that because of subs_delay start too early
		return;
	int time_reported = 0;
	for (int i = 0; i<15; i++)
	{
		if (data->row_used[i])
		{
			fprintf(stderr, "###SUBTITLE#");
			if (!time_reported)
			{
				LLONG ms_end = data->end_time;
				mstotime(ms_start, &h1, &m1, &s1, &ms1);
				mstotime(ms_end - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.
				// Note, only MM:SS here as we need to save space in the preview window
				fprintf(stderr, "%02u:%02u#%02u:%02u#",
					h1 * 60 + m1, s1, h2 * 60 + m2, s2);
				time_reported = 1;
			}
			else
				fprintf(stderr, "##");

			// We don't capitalize here because whatever function that was used
			// before to write to file already took care of it.
			int length = get_decoder_line_encoded_for_gui(subline, i, data);
			fwrite(subline, 1, length, stderr);
			fwrite("\n", 1, 1, stderr);
		}
	}
	fflush(stderr);
}
