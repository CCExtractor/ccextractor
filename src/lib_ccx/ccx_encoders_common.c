#include "ccx_decoders_common.h"
#include "ccx_encoders_common.h"
#include "utility.h"
#include "ocr.h"
#include "ccx_decoders_608.h"
#include "ccx_decoders_708.h"
#include "ccx_decoders_708_output.h"
#include "ccx_encoders_xds.h"
#include "ccx_encoders_helpers.h"
#include "../ccextractor.h"
#ifdef ENABLE_SHARING
#include "ccx_share.h"
#endif //ENABLE_SHARING

#ifdef WIN32
int fsync(int fd)
{
	return FlushFileBuffers((HANDLE)_get_osfhandle(fd)) ? 0 : -1;
}
#endif

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

static const char *ssa_header=
"[Script Info]\n\
Title: Default file\n\
ScriptType: v4.00+\n\
\n\
[V4+ Styles]\n\
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n\
Style: Default,Arial,20,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,1,1,2,10,10,10,0\n\
\n\
[Events]\n\
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n\
\n";

/*	

TODO -- Set correct values for "ttp:dropMode", "ttp:frameRate" .Using common for now.

 Extra reference, 

	24 frame/sec (film, ATSC, 2k, 4k, 6k)
	25 frame/sec (PAL (Europe, Uruguay, Argentina, Australia), SECAM, DVB, ATSC)
	29.97 (30 ÷ 1.001) frame/sec (NTSC American System (US, Canada, Mexico, Colombia, etc.), ATSC, PAL-M (Brazil))
	30 frame/sec (ATSC)
	
	Find framerate, then use it find dropmode.
	
*/
static const char *smptett_header = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
			"  <tt xmlns=\"http://www.w3.org/ns/ttml\" xmlns:ttp=\"http://www.w3.org/ns/ttml#parameter\" ttp:dropMode=\"dropNTSC\" ttp:frameRate=\"30\" ttp:frameRateMultiplier=\"1000 1001\" ttp:timeBase=\"smpte\" xmlns:m608=\"http://www.smpte-ra.org/schemas/2052-1/2010/smpte-tt#cea608\" xmlns:smpte=\"http://www.smpte-ra.org/schemas/2052-1/2010/smpte-tt\" xmlns:ttm=\"http://www.w3.org/ns/ttml#metadata\" xmlns:tts=\"http://www.w3.org/ns/ttml#styling\">\n"
//			"  <tt xmlns=\"http://www.w3.org/ns/ttml\" xmlns:ttp=\"http://www.w3.org/ns/ttml#parameter\" xmlns:m608=\"http://www.smpte-ra.org/schemas/2052-1/2010/smpte-tt#cea608\" xmlns:smpte=\"http://www.smpte-ra.org/schemas/2052-1/2010/smpte-tt\" xmlns:ttm=\"http://www.w3.org/ns/ttml#metadata\" xmlns:tts=\"http://www.w3.org/ns/ttml#styling\">\n"
			"  <head>\n"
			"    <styling>\n"
			"      <style tts:color=\"white\" tts:fontFamily=\"monospace\" tts:fontWeight=\"normal\" tts:textAlign=\"left\" xml:id=\"basic\"/>\n"
			"    </styling>\n"
			"    <layout>\n"
			"      <region tts:backgroundColor=\"transparent\" xml:id=\"pop1\"/>\n"
			"      <region tts:backgroundColor=\"transparent\" xml:id=\"paint\"/>\n"
			"      <region tts:backgroundColor=\"transparent\" xml:id=\"rollup2\"/>\n"
			"      <region tts:backgroundColor=\"transparent\" xml:id=\"rollup3\"/>\n"
			"      <region tts:backgroundColor=\"transparent\" xml:id=\"rollup4\"/>\n"
			"    </layout>\n"
			"    <metadata/>\n"
			"    <smpte:information m608:captionService=\"F1C1CC\" m608:channel=\"cc1\"/>\n"
			"  </head>\n"
			"  <body>\n"
			"    <div>\n";

static const char *webvtt_header[] = {"WEBVTT","\r\n","\r\n","STYLE","\r\n","\r\n",NULL};

static const char *simple_xml_header = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<captions>\r\n";

void find_limit_characters(unsigned char *line, int *first_non_blank, int *last_non_blank, int max_len)
{
	*last_non_blank = -1;
	*first_non_blank = -1;
	for (int i = 0; i < max_len; i++)
	{
		unsigned char c = line[i];
		if (c != ' ' && c != 0x89 )
		{
			if (*first_non_blank == -1)
				*first_non_blank = i;
			*last_non_blank = i;
		}
		if (c == '\0' || c == '\n' || c == '\r')
			break;
	}
}

unsigned int utf8_to_latin1_map(const unsigned int code)
{
    /* Code points 0 to U+00FF are the same in both. */
    if (code < 256U)
        return code;

    switch (code)
	{
    case 0x0152U:
		return 188U; /* U+0152 = 0xBC: OE ligature */
    case 0x0153U:
		return 189U; /* U+0153 = 0xBD: oe ligature */
    case 0x0160U:
		return 166U; /* U+0160 = 0xA6: S with caron */
    case 0x0161U: return 168U; /* U+0161 = 0xA8: s with caron */
    case 0x0178U: return 190U; /* U+0178 = 0xBE: Y with diaresis */
    case 0x017DU: return 180U; /* U+017D = 0xB4: Z with caron */
    case 0x017EU: return 184U; /* U+017E = 0xB8: z with caron */
    case 0x20ACU: return 164U; /* U+20AC = 0xA4: Euro */
    default:      return 256U;
    }
}

int change_utf8_encoding(unsigned char* dest, unsigned char* src, int len, enum ccx_encoding_type out_enc)
{
	unsigned char *orig = dest; // Keep for calculating length
	unsigned char *orig_src = src; // Keep for calculating length
	for (int i = 0; src < orig_src + len;)
	{
		unsigned char c = src[i];
		int c_len = 0;

		if ( c < 0x80 )
			c_len = 1;
		else if ( ( c & 0x20 ) == 0 )
			c_len = 2;
		else if ( ( c & 0x10 ) == 0 )
			c_len = 3;
		else if ( ( c & 0x08 ) == 0 )
			c_len = 4;
		else if ( ( c & 0x04 ) == 0 )
			c_len = 5;

		switch (out_enc)
		{
		case CCX_ENC_UTF_8:
			memcpy(dest, src, len);
			return len;
		case CCX_ENC_LATIN_1:
			if (c_len == 1)
				*dest++ = *src;
			else if (c_len == 2)
			{
				if ((src[1] & 0x40) == 0)
				{
					c = utf8_to_latin1_map((((unsigned int)(src[0] & 0x1F)) << 6)
							|  ((unsigned int)(src[1] & 0x3F)));
					if (c < 256)
						*dest++ = c;
					else
						*dest++ = '?';
				}
				else
					*dest++ = '?';
			}
			else if (c_len == 3)
			{
				if ((src[1] & 0x40) == 0 && (src[2] & 0x40) == 0 )
				{
					c = utf8_to_latin1_map( (((unsigned int)(src[0] & 0x0F)) << 12)
						| (((unsigned int)(src[1] & 0x3F)) << 6)
						|  ((unsigned int)(src[2] & 0x3F)) );
					if (c < 256)
						*dest++ = c;
					else
						*dest++ = '?';
				}
			}
			else if (c_len == 4)
			{
				if ((src[1] & 0x40) == 0 &&
					(src[2] & 0x40) == 0 &&
					(src[3] & 0x40) == 0)
				{
					c = utf8_to_latin1_map( (((unsigned int)(src[0] & 0x07)) << 18)
						| (((unsigned int)(src[1] & 0x3F)) << 12)
						| (((unsigned int)(src[2] & 0x3F)) << 6)
						|  ((unsigned int)(src[3] & 0x3F)) );
					if (c < 256)
						*(dest++) = c;
					else
						*dest++ = '?';
				}
				else
					*dest++ = '?';
			}
			else if (c_len == 5)
			{
				if ((src[1] & 0x40) == 0 &&
					(src[2] & 0x40) == 0 &&
					(src[3] & 0x40) == 0 &&
					(src[4] & 0x40) == 0)
				{
					c = utf8_to_latin1_map( (((unsigned int)(src[0] & 0x03)) << 24U)
						| (((unsigned int)(src[1] & 0x3F)) << 18U)
						| (((unsigned int)(src[2] & 0x3F)) << 12U)
						| (((unsigned int)(src[3] & 0x3F)) << 6U)
						|  ((unsigned int)(src[4] & 0x3FU)) );
					if (c < 256)
						*(dest++) = c;
					else
						*dest++ = '?';
				}
				else
					*dest++ = '?';
			}
			else
				*dest++ = '?';
			break;
		case CCX_ENC_UNICODE:
			return CCX_ENOSUPP;
		case CCX_ENC_ASCII:
			if (c_len == 1)
				*dest++ = *src;
			else
				*dest++ = '?';
		default:
			return CCX_ENOSUPP;
		}
		src += c_len;
	}
	*dest = 0;
	return (dest - orig); // Return length
}

int change_latin1_encoding(unsigned char* dest, unsigned char* src, int len, enum ccx_encoding_type out_enc)
{
	return CCX_ENOSUPP;
}

int change_unicode_encoding(unsigned char* dest, unsigned char* src, int len, enum ccx_encoding_type out_enc)
{
	return CCX_ENOSUPP;
}

int change_ascii_encoding(unsigned char* dest, unsigned char* src, int len, enum ccx_encoding_type out_enc)
{
	unsigned char *orig = dest; // Keep for calculating length
	int bytes = 0;
	for (int i = 0; i < len; i++)
	{
		char c = src[i];
		switch (out_enc)
		{
		case CCX_ENC_UTF_8:
			bytes = get_char_in_utf_8(dest, c);
			break;
		case CCX_ENC_LATIN_1:
			get_char_in_latin_1(dest, c);
			bytes = 1;
			break;
		case CCX_ENC_UNICODE:
			get_char_in_unicode(dest, c);
			bytes = 2;
			break;
		case CCX_ENC_ASCII:
			memcpy(dest, src, len);
			return len;
		default:
			return CCX_ENOSUPP;
		}
		dest += bytes;
	}
	*dest = 0;
	return (dest - orig); // Return length
}

int get_str_basic(unsigned char *out_buffer, unsigned char *in_buffer, int trim_subs,
	enum ccx_encoding_type in_enc, enum ccx_encoding_type out_enc, int max_len)
{
	int last_non_blank = -1;
	int first_non_blank = -1;
	int len = 0;
	find_limit_characters(in_buffer, &first_non_blank, &last_non_blank, max_len);
	if (!trim_subs)
		first_non_blank = 0;

	if (first_non_blank == -1)
	{
		*out_buffer = 0;
		return 0;
	}


	// change encoding only when required
	switch (in_enc)
	{
	case CCX_ENC_UTF_8:
		len = change_utf8_encoding(out_buffer, in_buffer + first_non_blank, last_non_blank-first_non_blank+1, out_enc);
		break;
	case CCX_ENC_LATIN_1:
		len = change_latin1_encoding(out_buffer, in_buffer + first_non_blank, last_non_blank-first_non_blank+1, out_enc);
		break;
	case CCX_ENC_UNICODE:
		len = change_unicode_encoding(out_buffer, in_buffer + first_non_blank, last_non_blank-first_non_blank+1, out_enc);
		break;
	case CCX_ENC_ASCII:
		len = change_ascii_encoding(out_buffer, in_buffer + first_non_blank, last_non_blank-first_non_blank+1, out_enc);
		break;
	}
	if (len < 0)
		mprint("WARNING: Could not encode in specified format\n");
	else if (len == CCX_ENOSUPP)
	// we only support ASCII to other encoding std
		mprint("WARNING: Encoding is not yet supported\n");
	else
		return (unsigned)len; // Return length

	return 0; // Return length
}

int write_subtitle_file_footer(struct encoder_ctx *ctx,struct ccx_s_write *out)
{
	int used;
	int ret = 0;
	char str[1024];

	switch (ctx->write_format)
	{
		case CCX_OF_SAMI:
			sprintf ((char *) str,"</BODY></SAMI>\n");
			if (ctx->encoding != CCX_ENC_UNICODE)
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
			if (ctx->encoding != CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
			}
			used = encode_line (ctx, ctx->buffer,(unsigned char *) str);
			ret = write (out->fh, ctx->buffer, used);
			if (ret != used)
			{
				mprint("WARNING: loss of data\n");
			}
			break;
		case CCX_OF_SPUPNG:
			write_spumux_footer(out);
			break;
		case CCX_OF_SIMPLE_XML:
			sprintf ((char *) str,"</captions>\n");
			if (ctx->encoding != CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
			}
			used = encode_line (ctx, ctx->buffer,(unsigned char *) str);
			ret = write (out->fh, ctx->buffer, used);
			if (ret != used)
			{
				mprint("WARNING: loss of data\n");
			}
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
	int header_size = 0;
	switch (ctx->write_format)
	{
		case CCX_OF_SRT: // Subrip subtitles have no header
		case CCX_OF_G608:
			ret = write_bom(ctx, out);
			if(ret < 0)
				return -1;
			break;
		case CCX_OF_SSA:
			ret = write_bom(ctx, out);
			if(ret < 0)
				return -1;
			REQUEST_BUFFER_CAPACITY(ctx,strlen (ssa_header)*3);
			used = encode_line (ctx, ctx->buffer,(unsigned char *) ssa_header);
			ret = write (out->fh, ctx->buffer, used);
			if(ret < used)
			{
				mprint("WARNING: Unable to write complete Buffer \n");
				return -1;
			}
			break;
		case CCX_OF_WEBVTT:
			ret = write_bom(ctx, out);
			if (ret < 0)
				return -1;
			for(int i = 0; webvtt_header[i]!=NULL ;i++)
			{
				header_size += strlen(webvtt_header[i]); // Find total size of the header
			}
			REQUEST_BUFFER_CAPACITY(ctx, header_size*3);
			for(int i = 0; webvtt_header[i]!=NULL;i++)
			{
				if(ccx_options.enc_cfg.line_terminator_lf == 1 && strcmp(webvtt_header[i],"\r\n")==0) // If -lf parameter passed, write LF instead of CRLF
				{
					used = encode_line (ctx, ctx->buffer,(unsigned char *) "\n");
				} 
				else
				{
					used = encode_line (ctx, ctx->buffer,(unsigned char *) webvtt_header[i]);
				}
				ret = write (out->fh, ctx->buffer,used);
				if(ret < used)
				{
					mprint("WARNING: Unable to write complete Buffer \n");
					return -1;
				}
			}
			break;
		case CCX_OF_SAMI: // This header brought to you by McPoodle's CCASDI
			//fprintf_encoded (wb->fh, sami_header);
			ret = write_bom(ctx, out);
			if(ret < 0)
				return -1;
			REQUEST_BUFFER_CAPACITY(ctx,strlen (sami_header)*3);
			used = encode_line (ctx, ctx->buffer,(unsigned char *) sami_header);
			ret = write (out->fh, ctx->buffer, used);
			if(ret < used)
			{
				mprint("WARNING: Unable to write complete Buffer \n");
				return -1;
			}
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
			break;
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
			break;
		case CCX_OF_SIMPLE_XML: // No header. Fall thru
			ret = write_bom(ctx, out);
			if(ret < 0)
				return -1;
			REQUEST_BUFFER_CAPACITY(ctx,strlen (simple_xml_header)*3);
			used=encode_line (ctx, ctx->buffer,(unsigned char *) simple_xml_header);
			ret = write(out->fh, ctx->buffer, used);
			if(ret < used)
			{
				mprint("WARNING: Unable to write complete Buffer \n");
				return -1;
			}
			break;
		default:
			break;
	}

	return ret;
}

int write_cc_subtitle_as_simplexml(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int length;
	int ret = 0;
	char *str;
	char *save_str;
	struct cc_subtitle *osub = sub;
	struct cc_subtitle *lsub = sub;

	while(sub)
	{
		str = sub->data;

		str = strtok_r(str, "\r\n", &save_str);
		do
		{
			length = get_str_basic(context->subline, (unsigned char*)str, context->trim_subs, sub->enc_type, context->encoding, strlen(str));
			if (length <= 0)
			{
				continue;
			}
			ret = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
			if(ret <  context->encoded_crlf_length)
			{
				mprint("Warning:Loss of data\n");
			}

		} while ( (str = strtok_r(NULL, "\r\n", &save_str) ));

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

void write_cc_line_as_simplexml(struct eia608_screen *data, struct encoder_ctx *context, int line_number)
{
	int ret;
	int length = 0;
	char *cap = "<caption>";
	char *cap1 = "</caption>";
	if (context->sentence_cap)
	{
		if (clever_capitalize (context, line_number, data))
			correct_case_with_dictionary(line_number, data);
	}
	length = get_str_basic (context->subline, data->characters[line_number],
			context->trim_subs, CCX_ENC_ASCII, context->encoding, CCX_DECODER_608_SCREEN_WIDTH);

	ret = write(context->out->fh, cap, strlen(cap));
	ret = write(context->out->fh, context->subline, length);
	if(ret < length)
	{
		mprint("Warning:Loss of data\n");
	}
	ret = write(context->out->fh, cap1, strlen(cap1));
	ret = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);

}


int write_cc_buffer_as_simplexml(struct eia608_screen *data, struct encoder_ctx *context)
{
	int wrote_something = 0;

	for (int i=0;i<15;i++)
	{
		if (data->row_used[i])
		{
			write_cc_line_as_simplexml(data, context, i);
		}
		wrote_something=1;
	}
	return wrote_something;
}


//Dummy Function for support DVB in simple xml
int write_cc_bitmap_as_simplexml(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;

	sub->nb_data = 0;
	freep(&sub->data);
	return ret;
}


/**
 * @brief Function to add credits at end of subtitles File
 *
 * @param context encoder context in which you want to add credits at end
 *
 * @param out output context which usually keeps file handler
 */
static void try_to_add_end_credits(struct encoder_ctx *context, struct ccx_s_write *out, LLONG current_fts)
{
	LLONG window, length, st, end;
	if (out->fh == -1)
		return;
	window = current_fts - context->last_displayed_subs_ms - 1;
	if (window < context->endcreditsforatleast.time_in_ms) // Won't happen, window is too short
		return;
	length = context->endcreditsforatmost.time_in_ms > window ?
		window : context->endcreditsforatmost.time_in_ms;

	st = current_fts - length - 1;
	end = current_fts;

	switch (context->write_format)
	{
		case CCX_OF_SRT:
			write_stringz_as_srt(context->end_credits_text, context, st, end);
			break;
		case CCX_OF_SSA:
			write_stringz_as_ssa(context->end_credits_text, context, st, end);
			break;
		case CCX_OF_WEBVTT:
			write_stringz_as_webvtt(context->end_credits_text, context, st, end);
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
		case CCX_OF_SSA:
			write_stringz_as_ssa(context->start_credits_text,context,st,end);
			break;
		case CCX_OF_WEBVTT:
			write_stringz_as_webvtt(context->start_credits_text, context, st, end);
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

static void dinit_output_ctx(struct encoder_ctx *ctx)
{
	int i;
	for(i = 0; i < ctx->nb_out; i++)
		dinit_write(ctx->out + i);
	freep(&ctx->out);

	if (ctx->dtvcc_extract)
	{
		for (i = 0; i < CCX_DTVCC_MAX_SERVICES; i++)
			ccx_dtvcc_writer_cleanup(&ctx->dtvcc_writers[i]);
	}
}

static int init_output_ctx(struct encoder_ctx *ctx, struct encoder_cfg *cfg)
{
	int ret = EXIT_OK;
	int nb_lang;
	char *basefilename = NULL; // Input filename without the extension
	char *extension = NULL; // Input filename without the extension

#define check_ret(filename) 	if (ret != EXIT_OK)	\
				{									\
					fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED,"Failed to open output file: %s\nDetails : %s\n", filename, strerror(errno)); \
					return ret;						\
				}

	if (cfg->cc_to_stdout == CCX_FALSE && cfg->send_to_srv == CCX_FALSE && cfg->extract == 12)
		nb_lang = 2;
	else
		nb_lang = 1;

	ctx->out = malloc(sizeof(struct ccx_s_write) * nb_lang);
	if(!ctx->out)
		return -1;
	ctx->nb_out = nb_lang;
	ctx->keep_output_closed = cfg->keep_output_closed;
	ctx->force_flush = cfg->force_flush;
	ctx->ucla = cfg->ucla;

	if(ctx->generates_file && cfg->cc_to_stdout == CCX_FALSE && cfg->send_to_srv == CCX_FALSE)
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

				ret = init_write(&ctx->out[0], strdup(cfg->output_filename), cfg->with_semaphore);
				check_ret(cfg->output_filename);
				ret = init_write(&ctx->out[1], create_outfilename(basefilename, "_2", extension), cfg->with_semaphore);
				check_ret(ctx->out[1].filename);
			}
			else
			{
				ret = init_write(ctx->out, strdup(cfg->output_filename), cfg->with_semaphore );
				check_ret(cfg->output_filename);
			}
		}
		else if (cfg->write_format != CCX_OF_NULL)
		{
            basefilename = get_basename(ctx->first_input_file);
            extension = get_file_extension(cfg->write_format);
			if (basefilename == NULL)
			{
				basefilename = get_basename("untitled");
			}

			if (cfg->extract == 12)
			{
				ret = init_write(&ctx->out[0], create_outfilename(basefilename, "_1", extension), cfg->with_semaphore);
				check_ret(ctx->out[0].filename);
				ret = init_write(&ctx->out[1], create_outfilename(basefilename, "_2", extension), cfg->with_semaphore);
				check_ret(ctx->out[1].filename);
			}
			else
			{
				ret = init_write(ctx->out, create_outfilename(basefilename, NULL, extension), cfg->with_semaphore);
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
		ctx->out[0].with_semaphore = 0;
		ctx->out[0].semaphore_filename = NULL;
		mprint ("Sending captions to stdout.\n");
	}

	if (cfg->send_to_srv == CCX_TRUE)
	{
		ctx->out[0].fh = -1;
		ctx->out[0].filename = NULL;

		connect_to_srv(ccx_options.srv_addr, ccx_options.srv_port, ccx_options.tcp_desc, ccx_options.tcp_password);
	}

	if (cfg->dtvcc_extract)
	{
		for (int i = 0; i < CCX_DTVCC_MAX_SERVICES; i++)
		{
			if (!cfg->services_enabled[i])
			{
				ctx->dtvcc_writers[i].fd = -1;
				ctx->dtvcc_writers[i].filename = NULL;
				ctx->dtvcc_writers[i].cd = (iconv_t) -1;
				continue;
			}

			if (cfg->cc_to_stdout)
			{
				ctx->dtvcc_writers[i].fd = STDOUT_FILENO;
				ctx->dtvcc_writers[i].filename = NULL;
				ctx->dtvcc_writers[i].cd = (iconv_t) -1;
			}
			else
			{
				if (cfg->output_filename)
					basefilename = get_basename(cfg->output_filename);
				else
					basefilename = get_basename(ctx->first_input_file);

				ccx_dtvcc_writer_init(&ctx->dtvcc_writers[i], basefilename,
									  ctx->program_number, i + 1, cfg->write_format, cfg);
				free(basefilename);
			}
		}
	}

	if(ret)
	{
		print_error(cfg->gui_mode_reports,
			"Output filename is same as one of input filenames. Check output parameters.\n");
		return CCX_COMMON_EXIT_FILE_CREATION_FAILED;
	}

	return EXIT_OK;
}

/**
 * @param current_fts used while calculating window for end credits
 */
void dinit_encoder(struct encoder_ctx **arg, LLONG current_fts)
{
	struct encoder_ctx *ctx = *arg;
	int i;
	if(!ctx)
		return;
	for (i = 0; i < ctx->nb_out; i++)
	{
		if (ctx->end_credits_text!=NULL)
			try_to_add_end_credits(ctx, ctx->out + i, current_fts);
		write_subtitle_file_footer(ctx, ctx->out + i);
	}

	free_encoder_context(ctx->prev);
	dinit_output_ctx(ctx);
	freep(&ctx->subline);
	freep(&ctx->buffer);
	ctx->capacity = 0;
	freep(arg);
}

int reset_output_ctx(struct encoder_ctx *ctx, struct encoder_cfg *cfg)
{
	dinit_output_ctx(ctx);
	return init_output_ctx(ctx, cfg);
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
	ctx->cea_708_counter = 0;
	ctx->wrote_webvtt_header = 0;

	ctx->program_number = opt->program_number;
	ctx->send_to_srv = opt->send_to_srv;
	ctx->multiple_files = opt->multiple_files;
	ctx->first_input_file = opt->first_input_file;

	if (opt->write_format == CCX_OF_NULL || opt->write_format == CCX_OF_CURL)
		ctx->generates_file = 0;
	else
		ctx->generates_file = 1;

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
	ctx->keep_output_closed = opt->keep_output_closed;
	ctx->force_flush = opt->force_flush;
	ctx->ucla = opt->ucla;

	ctx->sbs_enabled = opt->splitbysentence;

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

	ctx->new_sentence = 1; // Capitalize next letter?
	if (opt->line_terminator_lf)
		ctx->encoded_crlf_length = encode_line(ctx, ctx->encoded_crlf, (unsigned char *) "\n");
	else
		ctx->encoded_crlf_length = encode_line(ctx, ctx->encoded_crlf, (unsigned char *) "\r\n");

	ctx->encoded_br_length = encode_line(ctx, ctx->encoded_br, (unsigned char *) "<br>");

	for (i = 0; i < ctx->nb_out; i++)
	 	write_subtitle_file_header(ctx,ctx->out+i);

	ctx->dtvcc_extract = opt->dtvcc_extract;

	ctx->segment_pending = 0;
	ctx->segment_last_key_frame = 0;
	ctx->nospupngocr = opt->nospupngocr;

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

	context = change_filename(context);

#ifdef ENABLE_SHARING
	if (ccx_options.sharing_enabled)
		ccx_share_send(sub);
#endif //ENABLE_SHARING

	if (context->sbs_enabled)
	{
		// Write to a buffer that is later s+plit to generate split
		// in sentences
		if (sub->type == CC_BITMAP)
			sub = reformat_cc_bitmap_through_sentence_buffer(sub, context);

		if (NULL==sub)
			return wrote_something;
	}

	// Write subtitles as they come
		if (sub->type == CC_608)
		{
			struct eia608_screen *data = NULL;
			struct ccx_s_write *out;
			for (data = sub->data; sub->nb_data; sub->nb_data--, data++)
			{
				// Determine context based on channel. This replaces the code that was above, as this was incomplete (for cases where -12 was used for example)
				out = get_output_ctx(context, data->my_field);

				if (data->format == SFORMAT_XDS)
				{
					data->end_time = data->end_time + context->subs_delay;
					xds_write_transcript_line_prefix(context, out, data->start_time, data->end_time, data->cur_xds_packet_class);
					if (data->xds_len > 0)
					{
						ret = write(out->fh, data->xds_str, data->xds_len);
						if (ret < data->xds_len)
						{
							mprint("WARNING:Loss of data\n");
						}
					}
					freep(&data->xds_str);
					write_newline(context, 0);
					continue;
				}

				data->end_time = data->end_time + context->subs_delay;

				if (utc_refvalue != UINT64_MAX)
				{
					if (data->start_time != -1)
						data->start_time += utc_refvalue * 1000;
					data->end_time += utc_refvalue * 1000;
				}

#ifdef ENABLE_PYTHON
                //making a call to python_encoder so that if the call is from the api, no output is generated.
                if (signal_python_api)
                    wrote_something = pass_cc_buffer_to_python(data, context);
                else
#endif
                {
				    switch (context->write_format)
				    {
					case CCX_OF_SRT:
						if (!context->startcredits_displayed && context->start_credits_text != NULL)
							try_to_add_start_credits(context, data->start_time);
						wrote_something = write_cc_buffer_as_srt(data, context);
						break;
					case CCX_OF_SSA:
						if (!context->startcredits_displayed && context->start_credits_text != NULL)
							try_to_add_start_credits(context, data->start_time);
						wrote_something = write_cc_buffer_as_ssa(data, context);
						break;
					case CCX_OF_G608:
						wrote_something = write_cc_buffer_as_g608(data, context);
						break;
					case CCX_OF_WEBVTT:
						if (!context->startcredits_displayed && context->start_credits_text != NULL)
							try_to_add_start_credits(context, data->start_time);
						wrote_something = write_cc_buffer_as_webvtt(data, context);
						break;
					case CCX_OF_SAMI:
						if (!context->startcredits_displayed && context->start_credits_text != NULL)
							try_to_add_start_credits(context, data->start_time);
						wrote_something = write_cc_buffer_as_sami(data, context);
						break;
					case CCX_OF_SMPTETT:
						if (!context->startcredits_displayed && context->start_credits_text != NULL)
							try_to_add_start_credits(context, data->start_time);
						wrote_something = write_cc_buffer_as_smptett(data, context);
						break;
					case CCX_OF_TRANSCRIPT:
						wrote_something = write_cc_buffer_as_transcript2(data, context);
						break;
					case CCX_OF_SPUPNG:
						wrote_something = write_cc_buffer_as_spupng(data, context);
						break;
					case CCX_OF_SIMPLE_XML:
						if (ccx_options.keep_output_closed && context->out->temporarily_closed)
						{
							temporarily_open_output(context->out);
							write_subtitle_file_header(context, context->out);
						}
						wrote_something = write_cc_buffer_as_simplexml(data, context);
						if (ccx_options.keep_output_closed)
						{
							write_subtitle_file_footer(context, context->out);
							temporarily_close_output(context->out);
						}
						break;
					default:
						break;
				    }
                }
				if (wrote_something)
					context->last_displayed_subs_ms = data->end_time;

				if (context->gui_mode_reports)
					write_cc_buffer_to_gui(sub->data, context);
			}
			freep(&sub->data);
		}
		if (sub->type == CC_BITMAP)
		{
			switch (context->write_format)
			{
			case CCX_OF_SRT:
				if (!context->startcredits_displayed && context->start_credits_text != NULL)
					try_to_add_start_credits(context, sub->start_time);
				wrote_something = write_cc_bitmap_as_srt(sub, context);
				break;
            case CCX_OF_SSA:
				if (!context->startcredits_displayed && context->start_credits_text != NULL)
					try_to_add_start_credits(context, sub->start_time);
					wrote_something = write_cc_bitmap_as_ssa(sub, context);
				break;
			case CCX_OF_WEBVTT:
				if (!context->startcredits_displayed && context->start_credits_text != NULL)
					try_to_add_start_credits(context, sub->start_time);
				wrote_something = write_cc_bitmap_as_webvtt(sub, context);
				break;
			case CCX_OF_SAMI:
				if (!context->startcredits_displayed && context->start_credits_text != NULL)
					try_to_add_start_credits(context, sub->start_time);
				wrote_something = write_cc_bitmap_as_sami(sub, context);
				break;
			case CCX_OF_SMPTETT:
				if (!context->startcredits_displayed && context->start_credits_text != NULL)
					try_to_add_start_credits(context, sub->start_time);
				wrote_something = write_cc_bitmap_as_smptett(sub, context);
				break;
			case CCX_OF_TRANSCRIPT:
				wrote_something = write_cc_bitmap_as_transcript(sub, context);
				break;
			case CCX_OF_SPUPNG:
				wrote_something = write_cc_bitmap_as_spupng(sub, context);
				break;
			case CCX_OF_SIMPLE_XML:
				wrote_something = write_cc_bitmap_as_simplexml(sub, context);
				break;
#ifdef WITH_LIBCURL
			case CCX_OF_CURL:
				wrote_something = write_cc_bitmap_as_libcurl(sub, context);
				break;
#endif
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
				if (ret < sub->nb_data) {
					mprint("WARNING: Loss of data\n");
				}
			}
			sub->nb_data = 0;
		}
		if (sub->type == CC_TEXT)
		{
			switch (context->write_format)
			{
			case CCX_OF_SRT:
				if (!context->startcredits_displayed && context->start_credits_text != NULL)
					try_to_add_start_credits(context, sub->start_time);
				wrote_something = write_cc_subtitle_as_srt(sub, context);
				break;
			case CCX_OF_SSA:
				if (!context->startcredits_displayed && context->start_credits_text != NULL)
					try_to_add_start_credits(context, sub->start_time);
				wrote_something = write_cc_subtitle_as_ssa(sub, context);
				break;
			case CCX_OF_WEBVTT:
				if (!context->startcredits_displayed && context->start_credits_text != NULL)
					try_to_add_start_credits(context, sub->start_time);
				wrote_something = write_cc_subtitle_as_webvtt(sub, context);
				break;
			case CCX_OF_SAMI:
				if (!context->startcredits_displayed && context->start_credits_text != NULL)
					try_to_add_start_credits(context, sub->start_time);
				wrote_something = write_cc_subtitle_as_sami(sub, context);
				break;
			case CCX_OF_SMPTETT:
				if (!context->startcredits_displayed && context->start_credits_text != NULL)
					try_to_add_start_credits(context, sub->start_time);
				wrote_something = write_cc_subtitle_as_smptett(sub, context);
				break;
			case CCX_OF_TRANSCRIPT:
				wrote_something = write_cc_subtitle_as_transcript(sub, context);
				break;
			case CCX_OF_SPUPNG:
				wrote_something = write_cc_subtitle_as_spupng(sub, context);
				break;
			case CCX_OF_SIMPLE_XML:
				wrote_something = write_cc_subtitle_as_simplexml(sub, context);
				break;
			default:
				break;
			}
			sub->nb_data = 0;
		}

	if (!sub->nb_data)
		freep(&sub->data);
	if (wrote_something && context->force_flush)
		fsync(context->out->fh); // Don't buffer
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
				millis_to_time(ms_start, &h1, &m1, &s1, &ms1);
				millis_to_time(ms_end - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.
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

unsigned int get_line_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	unsigned char *orig = buffer; // Keep for debugging
	unsigned char *line = data->characters[line_num];
	for (int i = 0; i < 33; i++)
	{
		int bytes = 0;
		switch (ctx->encoding)
		{
		case CCX_ENC_UTF_8:
			bytes = get_char_in_utf_8(buffer, line[i]);
			break;
		case CCX_ENC_LATIN_1:
			get_char_in_latin_1(buffer, line[i]);
			bytes = 1;
			break;
		case CCX_ENC_UNICODE:
			get_char_in_unicode(buffer, line[i]);
			bytes = 2;
		case CCX_ENC_ASCII:
		    *buffer = line[i];
			bytes = 1;
			break;
		}
		buffer += bytes;
	}
	return (unsigned int)(buffer - orig); // Return length
}
unsigned int get_color_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	unsigned char *orig = buffer; // Keep for debugging
	for (int i = 0; i < 32; i++)
	{
		if (data->colors[line_num][i] < 10)
			*buffer++ = data->colors[line_num][i] + '0';
		else
			*buffer++ = 'E';
	}
	*buffer = 0;
	return (unsigned)(buffer - orig); // Return length
}
unsigned int get_font_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	unsigned char *orig = buffer; // Keep for debugging
	for (int i = 0; i < 32; i++)
	{
		if(data->fonts[line_num][i] == FONT_REGULAR)
			*buffer++ = 'R';
		else if(data->fonts[line_num][i] == FONT_UNDERLINED_ITALICS)
			*buffer++ = 'B';
		else if(data->fonts[line_num][i] == FONT_UNDERLINED)
			*buffer++ = 'U';
		else if(data->fonts[line_num][i] == FONT_ITALICS)
			*buffer++ = 'I';
		else
			*buffer++ = 'E';
	}
	return (unsigned)(buffer - orig); // Return length
}

void switch_output_file(struct lib_ccx_ctx *ctx, struct encoder_ctx *enc_ctx, int track_id) {
	if (enc_ctx->out->filename != NULL) { // Close and release the previous handle
		free(enc_ctx->out->filename);
		close(enc_ctx->out->fh);
	}
	char *ext = get_file_extension(ctx->write_format);
	char suffix[32];
	sprintf(suffix, "_%d", track_id);
	enc_ctx->out->filename = create_outfilename(get_basename(enc_ctx->first_input_file), suffix, ext);
	enc_ctx->out->fh = open(enc_ctx->out->filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);

	// Reset counters as we switch output file.
	enc_ctx->cea_708_counter = 0;
	enc_ctx->srt_counter = 0;
}
