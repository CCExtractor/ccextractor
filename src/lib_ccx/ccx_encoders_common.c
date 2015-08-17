#include "ccx_decoders_common.h"
#include "ccx_encoders_common.h"
#include "spupng_encoder.h"
#include "608_spupng.h"
#include "utility.h"
#include "ocr.h"
#include "ccx_decoders_608.h"
#include "ccx_decoders_xds.h"
#include "ccx_encoders_helpers.h"

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

//TODO sami header doesn't carry about CRLF/LF option
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

void find_limit_characters(unsigned char *line, int *first_non_blank, int *last_non_blank)
{
	*last_non_blank = -1;
	*first_non_blank = -1;
	for (int i = 0; i<CCX_DECODER_608_SCREEN_WIDTH; i++)
	{
		unsigned char c = line[i];
		if (c != ' ' && c != 0x89)
		{
			if (*first_non_blank == -1)
				*first_non_blank = i;
			*last_non_blank = i;
		}
	}
}


unsigned int get_str_basic(unsigned char *buffer, unsigned char *line, int trim_subs, enum ccx_encoding_type encoding)
{
	int last_non_blank = -1;
	int first_non_blank = -1;
	unsigned char *orig = buffer; // Keep for debugging
	find_limit_characters(line, &first_non_blank, &last_non_blank);
	if (!trim_subs)
		first_non_blank = 0;

	if (first_non_blank == -1)
	{
		*buffer = 0;
		return 0;
	}

	int bytes = 0;
	for (int i = first_non_blank; i <= last_non_blank; i++)
	{
		char c = line[i];
		switch (encoding)
		{
		case CCX_ENC_UTF_8:
			bytes = get_char_in_utf_8(buffer, c);
			break;
		case CCX_ENC_LATIN_1:
			get_char_in_latin_1(buffer, c);
			bytes = 1;
			break;
		case CCX_ENC_UNICODE:
			get_char_in_unicode(buffer, c);
			bytes = 2;
			break;
		}
		buffer += bytes;
	}
	*buffer = 0;
	return (unsigned)(buffer - orig); // Return length
}

int write_subtitle_file_footer(struct encoder_ctx *ctx,struct ccx_s_write *out)
{
	int used;
	int ret = 0;
	switch (ctx->write_format)
	{
		case CCX_OF_SAMI:
			sprintf ((char *) str,"</BODY></SAMI>\n");
			if (ctx->encoding!=CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
			}
			used = encode_line (ctx, ctx->buffer,(unsigned char *) str);
			ret = write(out->fh, ctx->buffer, used);
			if (ret != used)
			{
				mprint("WARNING: loss of data\n");
			}
			break;
		case CCX_OF_SMPTETT:
			sprintf ((char *) str,"    </div>\n  </body>\n</tt>\n");
			if (ctx->encoding!=CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
			}
			used=encode_line (ctx, ctx->buffer,(unsigned char *) str);
			ret = write (out->fh, ctx->buffer, used);
			if (ret != used)
			{
				mprint("WARNING: loss of data\n");
			}
			break;
		case CCX_OF_SPUPNG:
			write_spumux_footer(out);
			break;
		default: // Nothing to do, no footer on this format
			break;
	}

	return ret;
}


static int write_bom(struct encoder_ctx *ctx, struct ccx_s_write *out)
{
	int ret = 0;
	if (!ctx->no_bom){
		if (ctx->encoding == CCX_ENC_UTF_8){ // Write BOM
			ret = write(out->fh, UTF8_BOM, sizeof(UTF8_BOM));
			if ( ret < sizeof(UTF8_BOM)) {
				mprint("WARNING: Unable tp write UTF BOM\n");
				return -1;
			}
				
		}
		if (ctx->encoding == CCX_ENC_UNICODE){ // Write BOM
			ret = write(out->fh, LITTLE_ENDIAN_BOM, sizeof(LITTLE_ENDIAN_BOM));
			if ( ret < sizeof(LITTLE_ENDIAN_BOM)) {
				mprint("WARNING: Unable to write LITTLE_ENDIAN_BOM \n");
				return -1;
			}
		}
	}
	return ret;
}
static int write_subtitle_file_header(struct encoder_ctx *ctx, struct ccx_s_write *out)
{
	int used;
	int ret = 0;

	switch (ctx->write_format)
	{
		case CCX_OF_SRT: // Subrip subtitles have no header
			ret = write_bom(ctx, out);
			if(ret < 0)
				return -1;
			break;
		case CCX_OF_SAMI: // This header brought to you by McPoodle's CCASDI
			//fprintf_encoded (wb->fh, sami_header);
			ret = write_bom(ctx, out);
			if(ret < 0)
				return -1;
			REQUEST_BUFFER_CAPACITY(ctx,strlen (sami_header)*3);
			used = encode_line (ctx, ctx->buffer,(unsigned char *) sami_header);
			ret = write (out->fh, ctx->buffer,used);
			break;
		case CCX_OF_SMPTETT: // This header brought to you by McPoodle's CCASDI
			//fprintf_encoded (wb->fh, sami_header);
			ret = write_bom(ctx, out);
			if(ret < 0)
				return -1;
			REQUEST_BUFFER_CAPACITY(ctx,strlen (smptett_header)*3);
			used=encode_line (ctx, ctx->buffer,(unsigned char *) smptett_header);
			ret = write(out->fh, ctx->buffer, used);
			if(ret < used)
			{
				mprint("WARNING: Unable to write complete Buffer \n");
				return -1;
			}
			break;
		case CCX_OF_RCWT: // Write header
			rcwt_header[7] = ctx->in_fileformat; // sets file format version

			if (ctx->send_to_srv)
				net_send_header(rcwt_header, sizeof(rcwt_header));
			else
			{
				ret = write(out->fh, rcwt_header, sizeof(rcwt_header));
				if(ret < 0)
				{
					mprint("Unable to write rcwt header\n");
					return -1;
				}
			}

			break;
		case CCX_OF_RAW:
			ret = write(out->fh,BROADCAST_HEADER, sizeof(BROADCAST_HEADER));
			if(ret < sizeof(BROADCAST_HEADER))
			{
				mprint("Unable to write Raw header\n");
				return -1;
			}
		case CCX_OF_SPUPNG:
			ret = write_bom(ctx, out);
			if(ret < 0)
				return -1;
			write_spumux_header(ctx, out);
			break;
		case CCX_OF_TRANSCRIPT: // No header. Fall thru
			ret = write_bom(ctx, out);
			if(ret < 0)
				return -1;
		default:
			break;
	}

	return ret;
}

int write_cc_subtitle_as_transcript(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int length;
	int ret = 0;
	unsigned int h1,m1,s1,ms1;
	unsigned int h2,m2,s2,ms2;
	LLONG start_time;
	LLONG end_time;
	char *str;
	struct cc_subtitle *osub = sub;
	struct cc_subtitle *lsub = sub;

	while(sub)
	{
		if(sub->type == CC_TEXT)
		{
			start_time = sub->start_time;
			end_time = sub->end_time;
		}
		if (context->sentence_cap)
		{
			//TODO capitalize (line_number,data);
			//TODO correct_case(line_number,data);
		}

		str = sub->data;
		length = strlen(str);
		if (context->encoding!=CCX_ENC_UNICODE)
		{
			dbg_print(CCX_DMT_DECODER_608, "\r");
			dbg_print(CCX_DMT_DECODER_608, "%s\n", str);
		}

		if (length>0)
		{
			if (start_time == -1)
			{
				// CFS: Means that the line has characters but we don't have a timestamp for the first one. Since the timestamp
				// is set for example by the write_char function, it possible that we don't have one in empty lines (unclear)
				// For now, let's not consider this a bug as before and just return.
				// fatal (EXIT_BUG_BUG, "Bug in timedtranscript (ts_start_of_current_line==-1). Please report.");
				return 0;
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

			if (context->transcript_settings->showCC) {
				if(context->in_fileformat == 2 )
					fdprintf(context->out->fh, sub->info);
				else
					//TODO, data->my_field == 1 ? data->channel : data->channel + 2); // Data from field 2 is CC3 or 4
					fdprintf(context->out->fh, "CC?|");
			}
			if (context->transcript_settings->showMode){
				fdprintf(context->out->fh, "%s|", sub->mode);
			}
			ret = write(context->out->fh, str, length);
			if(ret < length)
			{
				mprint("Warning:Loss of data\n");
			}

			ret = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
			if(ret <  context->encoded_crlf_length)
			{
				mprint("Warning:Loss of data\n");
			}
		}

		freep(&sub->data);
		lsub = sub;
		sub = sub->next;
	}

	while(lsub != osub)
	{
		sub = lsub->prev;
		freep(&lsub);
		lsub = sub;
	}

	return ret;
}

void write_cc_line_as_transcript2(struct eia608_screen *data, struct encoder_ctx *context, int line_number)
{
	int ret = 0;
	unsigned int h1,m1,s1,ms1;
	unsigned int h2,m2,s2,ms2;
	LLONG start_time = data->start_time;
	LLONG end_time = data->end_time;
	if (context->sentence_cap)
	{
		capitalize (line_number,data);
		correct_case(line_number,data);
	}
	int length = get_str_basic (context->subline, data->characters[line_number], context->trim_subs, context->encoding);
	if (context->encoding!=CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r");
		dbg_print(CCX_DMT_DECODER_608, "%s\n",context->subline);
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

		ret = write(context->out->fh, context->subline, length);
		if(ret < length)
		{
			mprint("Warning:Loss of data\n");
		}

		ret = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
		if(ret <  context->encoded_crlf_length)
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
#ifdef ENABLE_OCR
	struct cc_bitmap* rect;

	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;

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

char *create_outfilename(const char *basename, const char *suffix, const char *extension)
{
	char *ptr = NULL;
	int blen, slen, elen;

	if(basename)
		blen = strlen(basename);
	else
		blen = 0;

	if(suffix)
		slen = strlen(suffix);
	else
		slen = 0;

	if(extension)
		elen = strlen(extension);
	else
		elen = 0;
	if ( (elen + slen + blen) <= 0)
		return NULL;

	ptr = malloc(elen + slen + blen + 1);
	if(!ptr)
		return NULL;

	ptr[0] = '\0';

	if(basename)
		strcat(ptr, basename);
	if(suffix)
		strcat(ptr, suffix);
	if(extension)
		strcat(ptr, extension);

	return ptr;
}

static void dinit_output_ctx(struct encoder_ctx *ctx)
{
	int i;
	for(i = 0; i < ctx->nb_out; i++)
		dinit_write(ctx->out + i);
	freep(&ctx->out);
}
static int init_output_ctx(struct encoder_ctx *ctx, struct encoder_cfg *cfg)
{
	int ret = EXIT_OK;
	int nb_lang;
	char *basefilename = NULL; // Input filename without the extension
	char *extension = NULL; // Input filename without the extension


#define check_ret(filename) 	if (ret != EXIT_OK)							\
				{									\
					print_error(cfg->gui_mode_reports,"Failed %s\n", filename);	\
					return ret;							\
				}

	if (cfg->cc_to_stdout == CCX_FALSE && cfg->send_to_srv == CCX_FALSE && cfg->extract == 12)
		nb_lang = 2;
	else
		nb_lang = 1;

	ctx->out = malloc(sizeof(struct ccx_s_write) * nb_lang);
	if(!ctx->out)
		return -1;
	ctx->nb_out = nb_lang;

	if(cfg->cc_to_stdout == CCX_FALSE && cfg->send_to_srv == CCX_FALSE)
	{
		if (cfg->output_filename != NULL)
		{
			// Use the given output file name for the field specified by
			// the -1, -2 switch. If -12 is used, the filename is used for
			// field 1.
			if (cfg->extract == 12)
			{
				basefilename = get_basename(cfg->output_filename);
				extension = get_file_extension(cfg->write_format);

				ret = init_write(&ctx->out[0], strdup(cfg->output_filename));
				check_ret(cfg->output_filename);
				ret = init_write(&ctx->out[1], create_outfilename(basefilename, "_2", extension));
				check_ret(ctx->out[1].filename);
			}
			else if (cfg->extract == 1)
			{
				ret = init_write(ctx->out, strdup(cfg->output_filename));
				check_ret(cfg->output_filename);
			}
			else
			{
				ret = init_write(ctx->out, strdup(cfg->output_filename));
				check_ret(cfg->output_filename);
			}
		}
		else
		{
			basefilename = get_basename(ctx->first_input_file);
			extension = get_file_extension(cfg->write_format);

			if (cfg->extract == 12)
			{
				ret = init_write(&ctx->out[0], create_outfilename(basefilename, "_1", extension));
				check_ret(ctx->out[0].filename);
				ret = init_write(&ctx->out[1], create_outfilename(basefilename, "_2", extension));
				check_ret(ctx->out[1].filename);
			}
			else if (cfg->extract == 1)
			{
				ret = init_write(ctx->out, create_outfilename(basefilename, NULL, extension));
				check_ret(ctx->out->filename);
			}
			else
			{
				ret = init_write(ctx->out, create_outfilename(basefilename, NULL, extension));
				check_ret(ctx->out->filename);
			}
		}

		freep(&basefilename);
		freep(&extension);
	}

	if (cfg->cc_to_stdout == CCX_TRUE)
	{
		ctx->out[0].fh = STDOUT_FILENO;
		ctx->out[0].filename = NULL;
		mprint ("Sending captions to stdout.\n");
	}


	if(ret)
	{
		print_error(cfg->gui_mode_reports,
			"Output filename is same as one of input filenames. Check output parameters.\n");
		return CCX_COMMON_EXIT_FILE_CREATION_FAILED;
	}

	return EXIT_OK;
}

void dinit_encoder(struct encoder_ctx **arg)
{
	struct encoder_ctx *ctx = *arg;
	int i;
	if(!ctx)
		return;
	for (i = 0; i < ctx->nb_out; i++)
	{
		if (ctx->end_credits_text!=NULL)
			try_to_add_end_credits(ctx,ctx->out);
		write_subtitle_file_footer(ctx,ctx->out+i);
	}

	dinit_output_ctx(ctx);
	freep(&ctx->subline);
	freep(&ctx->buffer);
	ctx->capacity = 0;
	freep(arg);
}

struct encoder_ctx *init_encoder(struct encoder_cfg *opt)
{
	int ret;
	int i;
	struct encoder_ctx *ctx = malloc(sizeof(struct encoder_ctx));
	if(!ctx)
		return NULL;

	ctx->buffer = (unsigned char *) malloc (INITIAL_ENC_BUFFER_CAPACITY);
	if (!ctx->buffer)
	{
		free(ctx);
		return NULL;
	}

	ctx->capacity=INITIAL_ENC_BUFFER_CAPACITY;
	ctx->srt_counter = 0;

	ctx->program_number = opt->program_number;
	ctx->send_to_srv = opt->send_to_srv;
	ctx->multiple_files = opt->multiple_files;
	ctx->first_input_file = opt->first_input_file;
	ret = init_output_ctx(ctx, opt);
	if (ret != EXIT_OK)
	{
		freep(&ctx->buffer);
		free(ctx);
		return NULL;
	}
	ctx->in_fileformat = opt->in_format;

	/** used in case of SUB_EOD_MARKER */
	ctx->prev_start = -1;
	ctx->subs_delay = opt->subs_delay;
	ctx->last_displayed_subs_ms = 0;
	ctx->date_format = opt->date_format;
	ctx->millis_separator = opt->millis_separator;
	ctx->startcredits_displayed = 0;

	ctx->encoding = opt->encoding;
	ctx->write_format = opt->write_format;
	ctx->transcript_settings = &opt->transcript_settings;
	ctx->no_bom = opt->no_bom;
	ctx->sentence_cap = opt->sentence_cap;
	ctx->trim_subs = opt->trim_subs;
	ctx->autodash = opt->autodash;
	ctx->no_font_color = opt->no_font_color;
	ctx->no_type_setting = opt->no_type_setting;
	ctx->gui_mode_reports = opt->gui_mode_reports;
	ctx->extract = opt->extract;
	ctx->subline = (unsigned char *) malloc (SUBLINESIZE);
	if(!ctx->subline)
	{
		freep(&ctx->out);
		freep(&ctx->buffer);
		free(ctx);
		return NULL;
	}


	ctx->start_credits_text = opt->start_credits_text;
	ctx->end_credits_text = opt->end_credits_text;
	ctx->startcreditsnotbefore = opt->startcreditsnotbefore;
	ctx->startcreditsnotafter = opt->startcreditsnotafter;
	ctx->startcreditsforatleast = opt->startcreditsforatleast;
	ctx->startcreditsforatmost = opt->startcreditsforatmost;
	ctx->endcreditsforatleast = opt->endcreditsforatleast;
	ctx->endcreditsforatmost = opt->endcreditsforatmost;

	if (opt->line_terminator_lf)
		ctx->encoded_crlf_length = encode_line(ctx, ctx->encoded_crlf, (unsigned char *) "\n");
	else
		ctx->encoded_crlf_length = encode_line(ctx, ctx->encoded_crlf, (unsigned char *) "\r\n");

	ctx->encoded_br_length = encode_line(ctx, ctx->encoded_br, (unsigned char *) "<br>");

	for (i = 0; i < ctx->nb_out; i++)
	 	write_subtitle_file_header(ctx,ctx->out+i);

	return ctx;

}

void set_encoder_rcwt_fileformat(struct encoder_ctx *ctx, short int format)
{
	if(ctx)
	{
		ctx->in_fileformat = format;
	}
}

static int write_newline(struct encoder_ctx *ctx, int lang)
{
	return write(ctx->out[lang].fh, ctx->encoded_crlf, ctx->encoded_crlf_length);
}

struct ccx_s_write *get_output_ctx(struct encoder_ctx *ctx, int lan)
{
	if (ctx->extract == 12 && lan == 2)
	{
		return ctx->out+1;
	}
	else
	{
		return ctx->out;
	}
}

int encode_sub(struct encoder_ctx *context, struct cc_subtitle *sub)
{
	int wrote_something = 0;
	int ret = 0;

	if(!context)
		return CCX_OK;

	if (sub->type == CC_608)
	{
		struct eia608_screen *data = NULL;
		struct ccx_s_write *out;
		for(data = sub->data; sub->nb_data ; sub->nb_data--,data++)
		{
			// Determine context based on channel. This replaces the code that was above, as this was incomplete (for cases where -12 was used for example)
			out = get_output_ctx(context, data->my_field);

			new_sentence=1;

			if(data->format == SFORMAT_XDS)
			{
				data->end_time = data->end_time + context->subs_delay;
				xds_write_transcript_line_prefix (out, data->start_time, data->end_time,data->cur_xds_packet_class);
				if(data->xds_len > 0)
				{
					ret = write (out->fh, data->xds_str,data->xds_len);
					if (ret < data->xds_len)
					{
						mprint("WARNING:Loss of data\n");
					}
				}
				freep (&data->xds_str);
				write_newline(context, 0);
				continue;
			}

			if(!data->start_time)
				break;

			data->end_time = data->end_time + context->subs_delay;
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
			break;
		case CCX_OF_SAMI:
			if (!context->startcredits_displayed && context->start_credits_text!=NULL)
				try_to_add_start_credits(context, sub->start_time);
			wrote_something = write_cc_bitmap_as_sami(sub, context);
			break;
		case CCX_OF_SMPTETT:
			if (!context->startcredits_displayed && context->start_credits_text!=NULL)
				try_to_add_start_credits(context, sub->start_time);
			wrote_something = write_cc_bitmap_as_smptett(sub, context);
			break;
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
	if (sub->type == CC_RAW)
	{
		if (context->send_to_srv)
			net_send_header(sub->data, sub->nb_data);
		else
		{
			ret = write(context->out->fh, sub->data, sub->nb_data);
			if ( ret < sub->nb_data) {
				mprint("WARNING: Loss of data\n");
			}
		}
		sub->nb_data = 0;
	}
	if(sub->type == CC_TEXT)
	{
		switch (context->write_format)
		{
		case CCX_OF_SRT:
			if (!context->startcredits_displayed && context->start_credits_text!=NULL)
				try_to_add_start_credits(context, sub->start_time);
			wrote_something = write_cc_subtitle_as_srt(sub, context);
			break;
		case CCX_OF_SAMI:
			if (!context->startcredits_displayed && context->start_credits_text!=NULL)
				try_to_add_start_credits(context, sub->start_time);
			wrote_something = write_cc_subtitle_as_sami(sub, context);
			break;
		case CCX_OF_SMPTETT:
			if (!context->startcredits_displayed && context->start_credits_text!=NULL)
				try_to_add_start_credits(context, sub->start_time);
			wrote_something = write_cc_subtitle_as_smptett(sub, context);
			break;
		case CCX_OF_TRANSCRIPT:
			wrote_something = write_cc_subtitle_as_transcript(sub, context);
			break;
		case CCX_OF_SPUPNG:
			wrote_something = write_cc_subtitle_as_spupng(sub, context);
			break;
		default:
			break;
		}
		sub->nb_data = 0;

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
			int length = get_decoder_line_encoded_for_gui(context->subline, i, data);
			fwrite(context->subline, 1, length, stderr);
			fwrite("\n", 1, 1, stderr);
		}
	}
	fflush(stderr);
}
