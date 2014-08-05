#include "ccextractor.h"
#include "cc_decoders_common.h"
#include "cc_encoders_common.h"
#include "spupng_encoder.h"
#include "608_spupng.h"
#include "utility.h"
#include "xds.h"
#include "ocr.h"

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

static const char *smptett_header =
"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\
<tt xmlns=\"http://www.w3.org/ns/ttml\" xml:lang=\"en\">\n\
<body>\n<div>\n" ;

void write_subtitle_file_footer(struct encoder_ctx *ctx,struct ccx_s_write *out)
{
	int used;
	switch (ccx_options.write_format)
	{
		case CCX_OF_SAMI:
			sprintf ((char *) str,"</BODY></SAMI>\n");
			if (ccx_options.encoding!=CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_608, "\r%s\n", str);
			}
			used=encode_line (ctx->buffer,(unsigned char *) str);
			write(out->fh, ctx->buffer, used);
			break;
		case CCX_OF_SMPTETT:
			sprintf ((char *) str,"</div></body></tt>\n");
			if (ccx_options.encoding!=CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_608, "\r%s\n", str);
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
	switch (ccx_options.write_format)
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
			write(out->fh, rcwt_header, sizeof(rcwt_header));

			if (ccx_options.send_to_srv)
				net_send_header(rcwt_header, sizeof(rcwt_header));

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
	if (ccx_options.sentence_cap)
	{
		capitalize (line_number,data);
		correct_case(line_number,data);
	}
	int length = get_decoder_line_basic (subline, line_number, data);
	if (ccx_options.encoding!=CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_608, "\r");
		dbg_print(CCX_DMT_608, "%s\n",subline);
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

		if (ccx_options.transcript_settings.showStartTime){
			char buf1[80];
			if (ccx_options.transcript_settings.relativeTimestamp){
				millis_to_date(start_time + subs_delay, buf1);
				fdprintf(context->out->fh, "%s|", buf1);
			}
			else {
				mstotime(start_time + subs_delay, &h1, &m1, &s1, &ms1);
				time_t start_time_int = (start_time + subs_delay) / 1000;
				int start_time_dec = (start_time + subs_delay) % 1000;
				struct tm *start_time_struct = gmtime(&start_time_int);
				strftime(buf1, sizeof(buf1), "%Y%m%d%H%M%S", start_time_struct);
				fdprintf(context->out->fh, "%s%c%03d|", buf1,ccx_options.millis_separator,start_time_dec);
			}
		}

		if (ccx_options.transcript_settings.showEndTime){
			char buf2[80];
			if (ccx_options.transcript_settings.relativeTimestamp){
				millis_to_date(end_time, buf2);
				fdprintf(context->out->fh, "%s|", buf2);
			}
			else {
				mstotime(get_fts() + subs_delay, &h2, &m2, &s2, &ms2);
				time_t end_time_int = end_time / 1000;
				int end_time_dec = end_time % 1000;
				struct tm *end_time_struct = gmtime(&end_time_int);
				strftime(buf2, sizeof(buf2), "%Y%m%d%H%M%S", end_time_struct);
				fdprintf(context->out->fh, "%s%c%03d|", buf2,ccx_options.millis_separator,end_time_dec);
			}
		}

		if (ccx_options.transcript_settings.showCC){
			fdprintf(context->out->fh, "CC%d|", data->my_field == 1 ? data->channel : data->channel + 2); // Data from field 2 is CC3 or 4
		}
		if (ccx_options.transcript_settings.showMode){
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
	dbg_print(CCX_DMT_608, "\n- - - TRANSCRIPT caption - - -\n");

	for (int i=0;i<15;i++)
	{
		if (data->row_used[i])
		{
			write_cc_line_as_transcript2 (data, context, i);
		}
		wrote_something=1;
	}
	dbg_print(CCX_DMT_608, "- - - - - - - - - - - -\r\n");
	return wrote_something;
}
int write_cc_bitmap_as_transcript(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	struct spupng_t *sp = (struct spupng_t *)context->out->spupng_data;
	int x_pos, y_pos, width, height, i;
	int x, y, y_off, x_off, ret;
	uint8_t *pbuf;
	char *filename;
	struct cc_bitmap* rect;
	png_color *palette = NULL;
	png_byte *alpha = NULL;
#ifdef ENABLE_OCR
	char*str = NULL;
#endif
	int used;
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	LLONG start_time, end_time;
	char timeline[128];
	int len = 0;

        x_pos = -1;
        y_pos = -1;
        width = 0;
        height = 0;

	if (context->prev_start != -1 && (sub->flags & SUB_EOD_MARKER))
	{
		start_time = context->prev_start + subs_delay;
		end_time = sub->start_time - 1;
	}
	else if ( !(sub->flags & SUB_EOD_MARKER))
	{
		start_time = sub->start_time + subs_delay;
		end_time = sub->end_time - 1;
	}

	if(sub->nb_data == 0 )
		return 0;
	rect = sub->data;
	for(i = 0;i < sub->nb_data;i++)
	{
		if(x_pos == -1)
		{
			x_pos = rect[i].x;
			y_pos = rect[i].y;
			width = rect[i].w;
			height = rect[i].h;
		}
		else
		{
			if(x_pos > rect[i].x)
			{
				width += (x_pos - rect[i].x);
				x_pos = rect[i].x;
			}

                        if (rect[i].y < y_pos)
                        {
                                height += (y_pos - rect[i].y);
                                y_pos = rect[i].y;
                        }

                        if (rect[i].x + rect[i].w > x_pos + width)
                        {
                                width = rect[i].x + rect[i].w - x_pos;
                        }

                        if (rect[i].y + rect[i].h > y_pos + height)
                        {
                                height = rect[i].y + rect[i].h - y_pos;
                        }

		}
	}
	if ( sub->flags & SUB_EOD_MARKER )
		context->prev_start =  sub->start_time;
	pbuf = (uint8_t*) malloc(width * height);
	memset(pbuf, 0x0, width * height);

	for(i = 0;i < sub->nb_data;i++)
	{
		x_off = rect[i].x - x_pos;
		y_off = rect[i].y - y_pos;
		for (y = 0; y < rect[i].h; y++)
		{
			for (x = 0; x < rect[i].w; x++)
				pbuf[((y + y_off) * width) + x_off + x] = rect[i].data[0][y * rect[i].w + x];

		}
	}
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
	/* TODO do rectangle, wise one color table should not be used for all rectangle */
        mapclut_paletee(palette, alpha, (uint32_t *)rect[0].data[1],rect[0].nb_colors);
	quantize_map(alpha, palette, pbuf, width*height, 3, rect[0].nb_colors);
#ifdef ENABLE_OCR
	str = ocr_bitmap(palette,alpha,pbuf,width,height);
	if(str && str[0])
	{
		if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
		{
			char *token = NULL;
			token = strtok(str,"\r\n");
			while (token)
			{

				if (ccx_options.transcript_settings.showStartTime)
				{
					char buf1[80];
					if (ccx_options.transcript_settings.relativeTimestamp)
					{
						millis_to_date(start_time + subs_delay, buf1);
						fdprintf(context->out->fh, "%s|", buf1);
					}
					else
					{
						mstotime(start_time + subs_delay, &h1, &m1, &s1, &ms1);
						time_t start_time_int = (start_time + subs_delay) / 1000;
						int start_time_dec = (start_time + subs_delay) % 1000;
						struct tm *start_time_struct = gmtime(&start_time_int);
						strftime(buf1, sizeof(buf1), "%Y%m%d%H%M%S", start_time_struct);
						fdprintf(context->out->fh, "%s%c%03d|", buf1,ccx_options.millis_separator,start_time_dec);
					}
				}

				if (ccx_options.transcript_settings.showEndTime)
				{
					char buf2[80];
					if (ccx_options.transcript_settings.relativeTimestamp)
					{
						millis_to_date(end_time, buf2);
						fdprintf(context->out->fh, "%s|", buf2);
					}
					else
					{
						mstotime(get_fts() + subs_delay, &h2, &m2, &s2, &ms2);
						time_t end_time_int = end_time / 1000;
						int end_time_dec = end_time % 1000;
						struct tm *end_time_struct = gmtime(&end_time_int);
						strftime(buf2, sizeof(buf2), "%Y%m%d%H%M%S", end_time_struct);
						fdprintf(context->out->fh, "%s%c%03d|", buf2,ccx_options.millis_separator,end_time_dec);
					}
				}
				if (ccx_options.transcript_settings.showCC)
				{
					fdprintf(context->out->fh,"%s|",language[sub->lang_index]);
				}
				if (ccx_options.transcript_settings.showMode)
				{
					fdprintf(context->out->fh,"DVB|");
				}
				fdprintf(context->out->fh,"%s\n",token);
				token = strtok(NULL,"\r\n");

			}

		}
	}
#endif

end:
	sub->nb_data = 0;
	freep(&sub->data);
	freep(&palette);
	freep(&alpha);
	return ret;

}
void try_to_add_end_credits(struct encoder_ctx *context, struct ccx_s_write *out)
{
	LLONG window, length, st, end;
	if (out->fh == -1)
		return;
	window=get_fts()-last_displayed_subs_ms-1;
	if (window<ccx_options.endcreditsforatleast.time_in_ms) // Won't happen, window is too short
		return;
	length=ccx_options.endcreditsforatmost.time_in_ms > window ?
		window : ccx_options.endcreditsforatmost.time_in_ms;

	st=get_fts()-length-1;
	end=get_fts();

	switch (ccx_options.write_format)
	{
		case CCX_OF_SRT:
			write_stringz_as_srt(ccx_options.end_credits_text, context, st, end);
			break;
		case CCX_OF_SAMI:
			write_stringz_as_sami(ccx_options.end_credits_text, context, st, end);
			break;
		case CCX_OF_SMPTETT:
			write_stringz_as_smptett(ccx_options.end_credits_text, context, st, end);
			break ;
		default:
			// Do nothing for the rest
			break;
	}
}

void try_to_add_start_credits(struct encoder_ctx *context,LLONG start_ms)
{
	LLONG st, end, window, length;
	LLONG l = start_ms + subs_delay;
    // We have a windows from last_displayed_subs_ms to l - we need to see if it fits

    if (l<ccx_options.startcreditsnotbefore.time_in_ms) // Too early
        return;

    if (last_displayed_subs_ms+1 > ccx_options.startcreditsnotafter.time_in_ms) // Too late
        return;

    st = ccx_options.startcreditsnotbefore.time_in_ms>(last_displayed_subs_ms+1) ?
        ccx_options.startcreditsnotbefore.time_in_ms : (last_displayed_subs_ms+1); // When would credits actually start

    end = ccx_options.startcreditsnotafter.time_in_ms<(l-1) ?
        ccx_options.startcreditsnotafter.time_in_ms : (l-1);

    window = end-st; // Allowable time in MS

    if (ccx_options.startcreditsforatleast.time_in_ms>window) // Window is too short
        return;

    length=ccx_options.startcreditsforatmost.time_in_ms > window ?
        window : ccx_options.startcreditsforatmost.time_in_ms;

    dbg_print(CCX_DMT_VERBOSE, "Last subs: %lld   Current position: %lld\n",
        last_displayed_subs_ms, l);
    dbg_print(CCX_DMT_VERBOSE, "Not before: %lld   Not after: %lld\n",
        ccx_options.startcreditsnotbefore.time_in_ms,
		ccx_options.startcreditsnotafter.time_in_ms);
    dbg_print(CCX_DMT_VERBOSE, "Start of window: %lld   End of window: %lld\n",st,end);

    if (window>length+2)
    {
        // Center in time window
        LLONG pad=window-length;
        st+=(pad/2);
    }
    end=st+length;
    switch (ccx_options.write_format)
    {
        case CCX_OF_SRT:
            write_stringz_as_srt(ccx_options.start_credits_text,context,st,end);
            break;
        case CCX_OF_SAMI:
			write_stringz_as_sami(ccx_options.start_credits_text, context, st, end);
            break;
        case CCX_OF_SMPTETT:
			write_stringz_as_smptett(ccx_options.start_credits_text, context, st, end);
            break;
        default:
            // Do nothing for the rest
            break;
    }
    startcredits_displayed=1;
    return;


}
int init_encoder(struct encoder_ctx *ctx,struct ccx_s_write *out)
{
	ctx->buffer = (unsigned char *) malloc (INITIAL_ENC_BUFFER_CAPACITY);
	if (ctx->buffer==NULL)
		return -1;
	ctx->capacity=INITIAL_ENC_BUFFER_CAPACITY;
	ctx->srt_counter = 0;
	ctx->out = out;
	/** used in case of SUB_EOD_MARKER */
	ctx->prev_start = -1;
	write_subtitle_file_header(ctx,out);

	return 0;

}

void dinit_encoder(struct encoder_ctx *ctx)
{

	if (ccx_options.end_credits_text!=NULL)
		try_to_add_end_credits(ctx,ctx->out);
	write_subtitle_file_footer(ctx,ctx->out);
	freep(&ctx->buffer);
	ctx->capacity = 0;
}

int encode_sub(struct encoder_ctx *context, struct cc_subtitle *sub)
{
	int wrote_something = 0 ;

	if (ccx_options.extract!=1)
		context++;

	if (sub->type == CC_608)
	{
		struct eia608_screen *data = NULL;
		for(data = sub->data; sub->nb_data ; sub->nb_data--,data++)
		{
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
			switch (ccx_options.write_format)
			{
			case CCX_OF_SRT:
				if (!startcredits_displayed && ccx_options.start_credits_text!=NULL)
					try_to_add_start_credits(context, data->start_time);
				wrote_something = write_cc_buffer_as_srt(data, context);
				break;
			case CCX_OF_SAMI:
				if (!startcredits_displayed && ccx_options.start_credits_text!=NULL)
					try_to_add_start_credits(context, data->start_time);
				wrote_something = write_cc_buffer_as_sami(data, context);
				break;
			case CCX_OF_SMPTETT:
				if (!startcredits_displayed && ccx_options.start_credits_text!=NULL)
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
				last_displayed_subs_ms=get_fts()+subs_delay;

			if (ccx_options.gui_mode_reports)
				write_cc_buffer_to_gui(sub->data, context);
		}
		freep(&sub->data);
	}
	if(sub->type == CC_BITMAP)
	{
		switch (ccx_options.write_format)
		{
		case CCX_OF_SRT:
			if (!startcredits_displayed && ccx_options.start_credits_text!=NULL)
				try_to_add_start_credits(context, sub->start_time);
			wrote_something = write_cc_bitmap_as_srt(sub, context);
		case CCX_OF_SAMI:
			if (!startcredits_displayed && ccx_options.start_credits_text!=NULL)
				try_to_add_start_credits(context, sub->start_time);
			wrote_something = write_cc_bitmap_as_sami(sub, context);
		case CCX_OF_SMPTETT:
			if (!startcredits_displayed && ccx_options.start_credits_text!=NULL)
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
