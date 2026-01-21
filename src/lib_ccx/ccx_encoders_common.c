#include "ccx_decoders_common.h"
#include "ccx_encoders_common.h"
#include "utility.h"
#include "ocr.h"
#include "ccx_decoders_608.h"
#include "ccx_decoders_708.h"
#include "ccx_decoders_708_output.h"
#include "ccx_encoders_xds.h"
#include "ccx_encoders_helpers.h"
#include "ccextractor.h"

#ifdef WIN32
int fsync(int fd)
{
	return FlushFileBuffers((HANDLE)_get_osfhandle(fd)) ? 0 : -1;
}
#endif

int ccxr_get_str_basic(unsigned char *out_buffer, unsigned char *in_buffer, int trim_subs,
		       enum ccx_encoding_type in_enc, enum ccx_encoding_type out_enc, int max_len);
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
	.isFinal = 0};

// TODO sami header doesn't care about CRLF/LF option
static const char *sami_header = // TODO: Revise the <!-- comments
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

static const char *ssa_header =
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

static const char *webvtt_header[] = {"WEBVTT", "\r\n", NULL};

static const char *simple_xml_header = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<captions>\r\n";

static const char CCD_HEADER[] = "SCC_disassembly V1.2";
static const char SCC_HEADER[] = "Scenarist_SCC V1.0";

void find_limit_characters(const unsigned char *line, int *first_non_blank, int *last_non_blank, int max_len)
{
	*last_non_blank = -1;
	*first_non_blank = -1;
	for (int i = 0; i < max_len; i++)
	{
		unsigned char c = line[i];
		if (c != ' ' && c != 0x89)
		{
			if (*first_non_blank == -1)
				*first_non_blank = i;
			*last_non_blank = i;
		}
		if (c == '\0' || c == '\n' || c == '\r')
			break;
	}
}

int get_str_basic(unsigned char *out_buffer, unsigned char *in_buffer, int trim_subs,
		  enum ccx_encoding_type in_enc, enum ccx_encoding_type out_enc, int max_len)
{
	return ccxr_get_str_basic(out_buffer, in_buffer, trim_subs, in_enc, out_enc, max_len);
}

int write_subtitle_file_footer(struct encoder_ctx *ctx, struct ccx_s_write *out)
{
	int used;
	int ret = 0;
	char str[1024];

	switch (ctx->write_format)
	{
		case CCX_OF_SAMI:
			snprintf((char *)str, sizeof(str), "</BODY></SAMI>\n");
			if (ctx->encoding != CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
			}
			used = encode_line(ctx, ctx->buffer, (unsigned char *)str);
			ret = write(out->fh, ctx->buffer, used);
			if (ret != used)
			{
				mprint("WARNING: loss of data\n");
			}
			break;
		case CCX_OF_SMPTETT:
			snprintf((char *)str, sizeof(str), "    </div>\n  </body>\n</tt>\n");
			if (ctx->encoding != CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
			}
			used = encode_line(ctx, ctx->buffer, (unsigned char *)str);
			ret = write(out->fh, ctx->buffer, used);
			if (ret != used)
			{
				mprint("WARNING: loss of data\n");
			}
			break;
		case CCX_OF_SPUPNG:
			write_spumux_footer(out);
			break;
		case CCX_OF_SIMPLE_XML:
			snprintf((char *)str, sizeof(str), "</captions>\n");
			if (ctx->encoding != CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
			}
			used = encode_line(ctx, ctx->buffer, (unsigned char *)str);
			ret = write(out->fh, ctx->buffer, used);
			if (ret != used)
			{
				mprint("WARNING: loss of data\n");
			}
			break;
		case CCX_OF_SCC:
		case CCX_OF_CCD:
			ret = write(out->fh, ctx->encoded_crlf, ctx->encoded_crlf_length);
			break;
		default: // Nothing to do, no footer on this format
			break;
	}

	return ret;
}

static int write_bom(struct encoder_ctx *ctx, struct ccx_s_write *out)
{
	int ret = 0;
	if (!ctx->no_bom)
	{
		if (ctx->encoding == CCX_ENC_UTF_8)
		{ // Write BOM
			ret = write(out->fh, UTF8_BOM, sizeof(UTF8_BOM));
			if (ret < sizeof(UTF8_BOM))
			{
				mprint("WARNING: Unable to write UTF BOM\n");
				return -1;
			}
		}
		if (ctx->encoding == CCX_ENC_UNICODE)
		{ // Write BOM
			ret = write(out->fh, LITTLE_ENDIAN_BOM, sizeof(LITTLE_ENDIAN_BOM));
			if (ret < sizeof(LITTLE_ENDIAN_BOM))
			{
				mprint("WARNING: Unable to write LITTLE_ENDIAN_BOM \n");
				return -1;
			}
		}
	}
	return ret;
}

// Returns -1 on error and 0 on success
static int write_subtitle_file_header(struct encoder_ctx *ctx, struct ccx_s_write *out)
{
	int used;
	int header_size = 0;
	switch (ctx->write_format)
	{
		case CCX_OF_CCD:
			if (write(out->fh, CCD_HEADER, sizeof(CCD_HEADER) - 1) == -1 || write(out->fh, ctx->encoded_crlf, ctx->encoded_crlf_length) == -1)
			{
				mprint("Unable to write CCD header to file\n");
				return -1;
			}
			break;
		case CCX_OF_SCC:
			if (write(out->fh, SCC_HEADER, sizeof(SCC_HEADER) - 1) == -1)
			{
				mprint("Unable to write SCC header to file\n");
				return -1;
			}
			break;
		case CCX_OF_SRT: // Subrip subtitles have no header
		case CCX_OF_G608:
			if (write_bom(ctx, out) < 0)
				return -1;
			break;
		case CCX_OF_SSA:
			if (write_bom(ctx, out) < 0)
				return -1;
			REQUEST_BUFFER_CAPACITY(ctx, strlen(ssa_header) * 3);
			used = encode_line(ctx, ctx->buffer, (unsigned char *)ssa_header);
			if (write(out->fh, ctx->buffer, used) < used)
			{
				mprint("WARNING: Unable to write complete Buffer \n");
				return -1;
			}
			break;
		case CCX_OF_WEBVTT:
			if (write_bom(ctx, out) < 0)
				return -1;
			for (int i = 0; webvtt_header[i] != NULL; i++)
			{
				header_size += strlen(webvtt_header[i]); // Find total size of the header
			}
			REQUEST_BUFFER_CAPACITY(ctx, header_size * 3);
			for (int i = 0; webvtt_header[i] != NULL; i++)
			{
				if (ccx_options.enc_cfg.line_terminator_lf == 1 && strcmp(webvtt_header[i], "\r\n") == 0) // If -lf parameter passed, write LF instead of CRLF
				{
					used = encode_line(ctx, ctx->buffer, (unsigned char *)"\n");
				}
				else
				{
					used = encode_line(ctx, ctx->buffer, (unsigned char *)webvtt_header[i]);
				}
				if (write(out->fh, ctx->buffer, used) < used)
				{
					mprint("WARNING: Unable to write complete Buffer \n");
					return -1;
				}
			}
			break;
		case CCX_OF_SAMI: // This header brought to you by McPoodle's CCASDI
			// fprintf_encoded (wb->fh, sami_header);
			if (write_bom(ctx, out) < 0)
				return -1;
			REQUEST_BUFFER_CAPACITY(ctx, strlen(sami_header) * 3);
			used = encode_line(ctx, ctx->buffer, (unsigned char *)sami_header);
			if (write(out->fh, ctx->buffer, used) < used)
			{
				mprint("WARNING: Unable to write complete Buffer \n");
				return -1;
			}
			break;
		case CCX_OF_SMPTETT: // This header brought to you by McPoodle's CCASDI
			// fprintf_encoded (wb->fh, sami_header);
			if (write_bom(ctx, out) < 0)
				return -1;
			REQUEST_BUFFER_CAPACITY(ctx, strlen(smptett_header) * 3);
			used = encode_line(ctx, ctx->buffer, (unsigned char *)smptett_header);
			if (write(out->fh, ctx->buffer, used) < used)
			{
				mprint("WARNING: Unable to write complete Buffer \n");
				return -1;
			}
			break;
		case CCX_OF_RCWT:			     // Write header
			rcwt_header[7] = ctx->in_fileformat; // sets file format version

			if (ctx->send_to_srv)
				net_send_header(rcwt_header, sizeof(rcwt_header));
			else
			{
				if (write(out->fh, rcwt_header, sizeof(rcwt_header)) < 0)
				{
					mprint("Unable to write rcwt header\n");
					return -1;
				}
			}

			break;
		case CCX_OF_RAW:
			if (write(out->fh, BROADCAST_HEADER, sizeof(BROADCAST_HEADER)) < sizeof(BROADCAST_HEADER))
			{
				mprint("Unable to write Raw header\n");
				return -1;
			}
			break;
		case CCX_OF_SPUPNG:
			if (write_bom(ctx, out) < 0)
				return -1;
			write_spumux_header(ctx, out);
			break;
		case CCX_OF_TRANSCRIPT: // No header. Fall thru
			if (write_bom(ctx, out) < 0)
				return -1;
			break;
		case CCX_OF_SIMPLE_XML: // No header. Fall thru
			if (write_bom(ctx, out) < 0)
				return -1;
			REQUEST_BUFFER_CAPACITY(ctx, strlen(simple_xml_header) * 3);
			used = encode_line(ctx, ctx->buffer, (unsigned char *)simple_xml_header);
			if (write(out->fh, ctx->buffer, used) < used)
			{
				mprint("WARNING: Unable to write complete Buffer \n");
				return -1;
			}
			break;
		case CCX_OF_MCC:
			ctx->header_printed_flag = CCX_FALSE;
			break;
		default:
			break;
	}

	return 0;
}

int write_cc_subtitle_as_simplexml(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int length;
	int ret = 0;
	char *str;
	char *save_str;
	struct cc_subtitle *osub = sub;
	struct cc_subtitle *lsub = sub;

	while (sub)
	{
		str = sub->data;

		str = strtok_r(str, "\r\n", &save_str);
		do
		{
			length = get_str_basic(context->subline, (unsigned char *)str, context->trim_subs, sub->enc_type, context->encoding, strlen(str));
			if (length <= 0)
			{
				continue;
			}
			ret = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
			if (ret < context->encoded_crlf_length)
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

void write_cc_line_as_simplexml(struct eia608_screen *data, struct encoder_ctx *context, int line_number)
{
	int ret;
	int length = 0;
	char *cap = "<caption>";
	char *cap1 = "</caption>";

	length = get_str_basic(context->subline, data->characters[line_number],
			       context->trim_subs, CCX_ENC_ASCII, context->encoding, CCX_DECODER_608_SCREEN_WIDTH);

	ret = write(context->out->fh, cap, strlen(cap));
	ret = write(context->out->fh, context->subline, length);
	if (ret < length)
	{
		mprint("Warning:Loss of data\n");
	}
	ret = write(context->out->fh, cap1, strlen(cap1));
	ret = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
}

int write_cc_buffer_as_simplexml(struct eia608_screen *data, struct encoder_ctx *context)
{
	int wrote_something = 0;

	for (int i = 0; i < 15; i++)
	{
		if (data->row_used[i])
		{
			write_cc_line_as_simplexml(data, context, i);
			wrote_something = 1;
		}
	}
	return wrote_something;
}

// Dummy Function for support DVB in simple xml
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
	length = context->endcreditsforatmost.time_in_ms > window ? window : context->endcreditsforatmost.time_in_ms;

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
			break;
		default:
			// Do nothing for the rest
			break;
	}
}

void try_to_add_start_credits(struct encoder_ctx *context, LLONG start_ms)
{
	LLONG st, end, window, length;
	// We have a windows from last_displayed_subs_ms to start_ms - we need to see if it fits

	if (start_ms < context->startcreditsnotbefore.time_in_ms) // Too early
		return;

	if (context->last_displayed_subs_ms + 1 > context->startcreditsnotafter.time_in_ms) // Too late
		return;

	st = context->startcreditsnotbefore.time_in_ms > (context->last_displayed_subs_ms + 1) ? context->startcreditsnotbefore.time_in_ms : (context->last_displayed_subs_ms + 1); // When would credits actually start

	end = context->startcreditsnotafter.time_in_ms < start_ms - 1 ? context->startcreditsnotafter.time_in_ms : start_ms - 1;

	window = end - st; // Allowable time in MS

	if (context->startcreditsforatleast.time_in_ms > window) // Window is too short
		return;

	length = context->startcreditsforatmost.time_in_ms > window ? window : context->startcreditsforatmost.time_in_ms;

	dbg_print(CCX_DMT_VERBOSE, "Last subs: %lld   Current position: %lld\n",
		  context->last_displayed_subs_ms, start_ms);
	dbg_print(CCX_DMT_VERBOSE, "Not before: %lld   Not after: %lld\n",
		  context->startcreditsnotbefore.time_in_ms,
		  context->startcreditsnotafter.time_in_ms);
	dbg_print(CCX_DMT_VERBOSE, "Start of window: %lld   End of window: %lld\n", st, end);

	if (window > length + 2)
	{
		// Center in time window
		LLONG pad = window - length;
		st += (pad / 2);
	}
	end = st + length;
	switch (context->write_format)
	{
		case CCX_OF_SRT:
			write_stringz_as_srt(context->start_credits_text, context, st, end);
			break;
		case CCX_OF_SSA:
			write_stringz_as_ssa(context->start_credits_text, context, st, end);
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
			mprint("Format doesn't support credits");
			break;
	}
	context->startcredits_displayed = 1;
	return;
}

static void dinit_output_ctx(struct encoder_ctx *ctx)
{
	int i;
	for (i = 0; i < ctx->nb_out; i++)
	{
		dinit_write(ctx->out + i);
	}
	freep(&ctx->out);

	if (ctx->dtvcc_extract)
	{
		for (i = 0; i < CCX_DTVCC_MAX_SERVICES; i++)
			dtvcc_writer_cleanup(&ctx->dtvcc_writers[i]);
	}
}

static int init_output_ctx(struct encoder_ctx *ctx, struct encoder_cfg *cfg)
{
	int ret = EXIT_OK;
	int nb_lang;
	const char *extension; // Input filename without the extension

#define check_ret(filename)                                                                                                               \
	if (ret != EXIT_OK)                                                                                                               \
	{                                                                                                                                 \
		fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Failed to open output file: %s\nDetails : %s\n", filename, strerror(errno)); \
		return ret;                                                                                                               \
	}

	if (cfg->cc_to_stdout == CCX_FALSE && cfg->send_to_srv == CCX_FALSE && cfg->extract == 12 && cfg->write_format != CCX_OF_MCC)
		nb_lang = 2;
	else
		nb_lang = 1;

	ctx->out = malloc(sizeof(struct ccx_s_write) * nb_lang);
	if (!ctx->out)
		return -1;
	memset(ctx->out, 0, sizeof(struct ccx_s_write) * nb_lang);
	ctx->nb_out = nb_lang;
	ctx->keep_output_closed = cfg->keep_output_closed;
	ctx->force_flush = cfg->force_flush;
	ctx->ucla = cfg->ucla;
	ctx->force_dropframe = cfg->force_dropframe;

	if (ctx->generates_file && cfg->cc_to_stdout == CCX_FALSE && cfg->send_to_srv == CCX_FALSE && cfg->extract_only_708 == CCX_FALSE)
	{
		if (cfg->output_filename != NULL)
		{
			extension = get_file_extension(cfg->write_format);

			if ((cfg->extract == 12) && (cfg->write_format != CCX_OF_MCC))
			{
				char *basefilename = get_basename(cfg->output_filename);
				char *out_filename = ensure_output_extension(cfg->output_filename, extension);

				ret = init_write(&ctx->out[0], out_filename, cfg->with_semaphore);
				check_ret(out_filename);
				ret = init_write(&ctx->out[1], create_outfilename(basefilename, "_2", extension), cfg->with_semaphore);
				check_ret(ctx->out[1].filename);
				free(basefilename);
			}
			else
			{
				char *out_filename = ensure_output_extension(cfg->output_filename, extension);
				ret = init_write(ctx->out, out_filename, cfg->with_semaphore);
				check_ret(out_filename);
			}
		}
		else if (cfg->write_format != CCX_OF_NULL)
		{
			char *basefilename = get_basename(ctx->first_input_file);
			extension = get_file_extension(cfg->write_format);
			if (basefilename == NULL)
			{
				basefilename = get_basename("untitled");
			}

			if ((cfg->extract == 12) && (cfg->write_format != CCX_OF_MCC))
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
			free(basefilename);
		}
	}

	if (cfg->cc_to_stdout == CCX_TRUE)
	{
		ctx->out[0].fh = STDOUT_FILENO;
		ctx->out[0].filename = NULL;
		ctx->out[0].with_semaphore = 0;
		ctx->out[0].semaphore_filename = NULL;
		mprint("Sending captions to stdout.\n");
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
				ctx->dtvcc_writers[i].fhandle = NULL;
				ctx->dtvcc_writers[i].charset = NULL;
				ctx->dtvcc_writers[i].filename = NULL;
				ctx->dtvcc_writers[i].cd = (iconv_t)-1;
				continue;
			}

			if (cfg->cc_to_stdout)
			{
#ifdef WIN32
				ctx->dtvcc_writers[i].fd = -1;
				ctx->dtvcc_writers[i].fhandle = GetStdHandle(STD_OUTPUT_HANDLE);
#else
				ctx->dtvcc_writers[i].fd = STDOUT_FILENO;
				ctx->dtvcc_writers[i].fhandle = NULL;
#endif
				ctx->dtvcc_writers[i].charset = NULL;
				ctx->dtvcc_writers[i].filename = NULL;
				ctx->dtvcc_writers[i].cd = (iconv_t)-1;
			}
			else
			{
				char *basefilename;
				if (cfg->output_filename)
					basefilename = get_basename(cfg->output_filename);
				else
					basefilename = get_basename(ctx->first_input_file);

				dtvcc_writer_init(&ctx->dtvcc_writers[i], basefilename,
						  ctx->program_number, i + 1, cfg->write_format, cfg);
				free(basefilename);
			}
		}
	}

	if (ret)
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
	if (!ctx)
		return;
	for (i = 0; i < ctx->nb_out; i++)
	{
		if (ctx->end_credits_text != NULL)
			try_to_add_end_credits(ctx, ctx->out + i, current_fts);
		write_subtitle_file_footer(ctx, ctx->out + i);
	}

	// Clean up teletext multi-page output files (issue #665)
	dinit_teletext_outputs(ctx);

	free_encoder_context(ctx->prev);
	ctx->prev = NULL; // Ensure it's nulled after freeing
	freep(&ctx->last_str);
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
	struct encoder_ctx *ctx = calloc(1, sizeof(struct encoder_ctx));
	if (!ctx)
		return NULL;

	ctx->buffer = (unsigned char *)malloc(INITIAL_ENC_BUFFER_CAPACITY);
	if (!ctx->buffer)
	{
		free(ctx);
		return NULL;
	}

	ctx->capacity = INITIAL_ENC_BUFFER_CAPACITY;
	ctx->srt_counter = 0;
	ctx->cea_708_counter = 0;
	ctx->wrote_webvtt_header = 0;
	ctx->wrote_ccd_channel_header = false;

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
	ctx->is_pal = (opt->in_format == 2);

	/** used in case of SUB_EOD_MARKER */
	ctx->prev_start = -1;
	ctx->subs_delay = opt->subs_delay;
	ctx->last_displayed_subs_ms = 0;
	ctx->date_format = opt->date_format;
	ctx->millis_separator = opt->millis_separator;
	ctx->startcredits_displayed = 0;

	ctx->encoding = opt->encoding;
	ctx->write_format = opt->write_format;

	ctx->last_str = NULL;

	ctx->transcript_settings = &opt->transcript_settings;
	ctx->no_bom = opt->no_bom;
	ctx->sentence_cap = opt->sentence_cap;
	ctx->filter_profanity = opt->filter_profanity;
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

	ctx->subline = (unsigned char *)malloc(SUBLINESIZE);
	if (!ctx->subline)
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
		ctx->encoded_crlf_length = encode_line(ctx, ctx->encoded_crlf, (unsigned char *)"\n");
	else
		ctx->encoded_crlf_length = encode_line(ctx, ctx->encoded_crlf, (unsigned char *)"\r\n");

	ctx->encoded_br_length = encode_line(ctx, ctx->encoded_br, (unsigned char *)"<br>");

	for (i = 0; i < ctx->nb_out; i++)
		write_subtitle_file_header(ctx, ctx->out + i);

	ctx->dtvcc_extract = opt->dtvcc_extract;

	ctx->segment_pending = 0;
	ctx->segment_last_key_frame = 0;
	ctx->nospupngocr = opt->nospupngocr;
	ctx->scc_framerate = opt->scc_framerate;
	ctx->scc_accurate_timing = opt->scc_accurate_timing;
	ctx->scc_last_transmission_end = 0;
	ctx->scc_last_display_end = 0;

	// Initialize teletext multi-page output arrays (issue #665)
	ctx->tlt_out_count = 0;
	for (int i = 0; i < MAX_TLT_PAGES_EXTRACT; i++)
	{
		ctx->tlt_out[i] = NULL;
		ctx->tlt_out_pages[i] = 0;
		ctx->tlt_srt_counter[i] = 0;
	}

	ctx->prev = NULL;
	return ctx;
}

void set_encoder_rcwt_fileformat(struct encoder_ctx *ctx, short int format)
{
	if (ctx)
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
		return ctx->out + 1;
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

	/* If there is no encoder context (e.g. -out=report), we must still free
	   any allocated subtitle data to avoid memory leaks. */
	if (!context)
	{
		if (sub)
		{
			/* DVB subtitles store bitmap planes inside cc_bitmap */
			if (sub->datatype == CC_DATATYPE_DVB)
			{
				struct cc_bitmap *bitmap = (struct cc_bitmap *)sub->data;
				if (bitmap)
				{
					freep(&bitmap->data0);
					freep(&bitmap->data1);
				}
			}

			/* Free generic subtitle payload buffer */
			freep(&sub->data);
			sub->nb_data = 0;
		}

		return CCX_OK;
	}

	context = change_filename(context);

	if (context->sbs_enabled)
	{
		// Write to a buffer that is later s+plit to generate split
		// in sentences
		if (sub->type == CC_BITMAP)
			sub = reformat_cc_bitmap_through_sentence_buffer(sub, context);

		if (NULL == sub)
			return wrote_something;
	}

	// Write subtitles as they come
	switch (sub->type)
	{
		case CC_608:;
			struct eia608_screen *data = NULL;
			struct ccx_s_write *out;
			for (data = sub->data; sub->nb_data; --sub->nb_data, ++data)
			{
				// Determine context based on channel. This replaces the code
				// that was above, as this was incomplete (for cases where -12
				// was used for example)
				out = get_output_ctx(context, data->my_field);

				data->end_time += context->subs_delay;
				data->start_time += context->subs_delay;

				// After adding delay, if start/end time is lower than 0, then continue with the next subtitle
				if (data->start_time < 0 || data->end_time <= 0)
				{
					// Free XDS string if skipping to avoid memory leak
					if (data->format == SFORMAT_XDS && data->xds_str)
					{
						freep(&data->xds_str);
					}
					continue;
				}

				if (data->format == SFORMAT_XDS)
				{
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

				if (utc_refvalue != UINT64_MAX)
				{
					if (data->start_time != -1)
						data->start_time += utc_refvalue * 1000;
					data->end_time += utc_refvalue * 1000;
				}

				for (int i = 0; i < CCX_DECODER_608_SCREEN_ROWS; ++i)
					correct_spelling_and_censor_words(context, data->characters[i], CCX_DECODER_608_SCREEN_WIDTH);

				switch (context->write_format)
				{
					case CCX_OF_CCD:
						// TODO: credits
						wrote_something = write_cc_buffer_as_ccd(data, context);
						break;
					case CCX_OF_SCC:
						// TODO: credits
						wrote_something = write_cc_buffer_as_scc(data, context);
						break;
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
						mprint("Output format not supported\n");
						break;
				}
				if (wrote_something)
					context->last_displayed_subs_ms = data->end_time;

				if (context->gui_mode_reports)
					write_cc_buffer_to_gui(sub->data, context);
			}
			freep(&sub->data);
			break;
		case CC_BITMAP:;
			// Apply subs_delay to bitmap subtitles (DVB, DVD, etc.)
			// This is the same as what's done for CC_608 above
			sub->start_time += context->subs_delay;
			sub->end_time += context->subs_delay;

			// After adding delay, if start/end time is lower than 0, skip this subtitle
			if (sub->start_time < 0 || sub->end_time <= 0)
			{
				// Free bitmap data to avoid memory leak
				if (sub->datatype == CC_DATATYPE_DVB)
				{
					struct cc_bitmap *bitmap_tmp = (struct cc_bitmap *)sub->data;
					if (bitmap_tmp)
					{
						freep(&bitmap_tmp->data0);
						freep(&bitmap_tmp->data1);
					}
				}
				freep(&sub->data);
				sub->nb_data = 0;
				break;
			}

#ifdef ENABLE_OCR
			struct cc_bitmap *rect;
			int i;
			for (i = 0, rect = sub->data; i < sub->nb_data; ++i, ++rect)
			{
				if (rect->ocr_text)
				{
					int len = strlen(rect->ocr_text);
					correct_spelling_and_censor_words(context, (unsigned char *)rect->ocr_text, len);
					for (int i = 0; i < len; ++i)
					{
						if ((unsigned char)rect->ocr_text[i] == 0x98) // asterisk in 608 encoding
							rect->ocr_text[i] = '*';
					}
				}
			}
#endif
			switch (context->write_format)
			{
				case CCX_OF_CCD:
					// TODO
					fatal(1, "CC_BITMAP not implemented for CCX_OF_CCD.");
					break;
				case CCX_OF_SCC:
					// TODO
					fatal(1, "CC_BITMAP not implemented for CCX_OF_SCC.");
					break;
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
			break;
		case CC_RAW:
			if (context->send_to_srv)
				net_send_header(sub->data, sub->nb_data);
			else
			{
				ret = write(context->out->fh, sub->data, sub->nb_data);
				if (ret < sub->nb_data)
				{
					mprint("WARNING: Loss of data\n");
				}
			}
			sub->nb_data = 0;
			break;
		case CC_TEXT:
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
			break;
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
	int with_data = 0;

	for (int i = 0; i < 15; i++)
	{
		if (data->row_used[i])
			with_data = 1;
	}
	if (!with_data)
		return;

	int time_reported = 0;
	for (int i = 0; i < 15; i++)
	{
		if (data->row_used[i])
		{
			if (!time_reported)
			{
				millis_to_time(data->start_time, &h1, &m1, &s1, &ms1);
				millis_to_time(data->end_time - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.
				// Note, only MM:SS here as we need to save space in the preview window
				fprintf(stderr, "###TIME###%02u:%02u-%02u:%02u\n###SUBTITLE###",
					h1 * 60 + m1, s1, h2 * 60 + m2, s2);
				time_reported = 1;
			}
			else
				fprintf(stderr, "###SUBTITLE###");

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
	for (int i = 0; i < 32; i++)
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
	return (unsigned)(buffer - orig); // Return length
}
unsigned int get_font_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	unsigned char *orig = buffer; // Keep for debugging
	for (int i = 0; i < 32; i++)
	{
		if (data->fonts[line_num][i] == FONT_REGULAR)
			*buffer++ = 'R';
		else if (data->fonts[line_num][i] == FONT_UNDERLINED_ITALICS)
			*buffer++ = 'B';
		else if (data->fonts[line_num][i] == FONT_UNDERLINED)
			*buffer++ = 'U';
		else if (data->fonts[line_num][i] == FONT_ITALICS)
			*buffer++ = 'I';
		else
			*buffer++ = 'E';
	}
	return (unsigned)(buffer - orig); // Return length
}

void switch_output_file(struct lib_ccx_ctx *ctx, struct encoder_ctx *enc_ctx, int track_id)
{
	// Do nothing when in "report" mode
	if (enc_ctx == NULL || enc_ctx->out == NULL)
	{
		return;
	}

	if (enc_ctx->out->filename != NULL)
	{ // Close and release the previous handle
		free(enc_ctx->out->filename);
		close(enc_ctx->out->fh);
	}
	const char *ext = get_file_extension(ctx->write_format);
	char suffix[32];
	snprintf(suffix, sizeof(suffix), "_%d", track_id);
	char *basename = get_basename(enc_ctx->out->original_filename);
	if (basename != NULL)
	{
		enc_ctx->out->filename = create_outfilename(basename, suffix, ext);
		enc_ctx->out->fh = open(enc_ctx->out->filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
		free(basename);
	}

	write_subtitle_file_header(enc_ctx, enc_ctx->out);

	// Reset counters as we switch output file.
	enc_ctx->cea_708_counter = 0;
	enc_ctx->srt_counter = 0;
}

/**
 * Get or create the output file for a specific teletext page (issue #665)
 * Creates output files on-demand with suffix _pNNN (e.g., output_p891.srt)
 * Returns NULL if we're in stdout mode or if too many pages are being extracted
 */
struct ccx_s_write *get_teletext_output(struct encoder_ctx *ctx, uint16_t teletext_page)
{
	// If teletext_page is 0, use the default output
	if (teletext_page == 0 || ctx->out == NULL)
		return ctx->out;

	// Check if we're sending to stdout - can't do multi-page in that case
	if (ctx->out[0].fh == STDOUT_FILENO)
		return ctx->out;

	// Check if we already have an output file for this page
	for (int i = 0; i < ctx->tlt_out_count; i++)
	{
		if (ctx->tlt_out_pages[i] == teletext_page)
			return ctx->tlt_out[i];
	}

	// If we only have one teletext page requested, use the default output
	// (no suffix needed for backward compatibility)
	extern struct ccx_s_teletext_config tlt_config;
	if (tlt_config.num_user_pages <= 1 && !tlt_config.extract_all_pages)
		return ctx->out;

	// Need to create a new output file for this page
	if (ctx->tlt_out_count >= MAX_TLT_PAGES_EXTRACT)
	{
		mprint("Warning: Too many teletext pages to extract (max %d), using default output for page %03d\n",
		       MAX_TLT_PAGES_EXTRACT, teletext_page);
		return ctx->out;
	}

	// Allocate the new write structure
	struct ccx_s_write *new_out = (struct ccx_s_write *)malloc(sizeof(struct ccx_s_write));
	if (!new_out)
	{
		mprint("Error: Memory allocation failed for teletext output\n");
		return ctx->out;
	}
	memset(new_out, 0, sizeof(struct ccx_s_write));

	// Create the filename with page suffix
	const char *ext = get_file_extension(ctx->write_format);
	char suffix[16];
	snprintf(suffix, sizeof(suffix), "_p%03d", teletext_page);

	char *basefilename = NULL;
	if (ctx->out[0].filename != NULL)
	{
		basefilename = get_basename(ctx->out[0].filename);
	}
	else if (ctx->first_input_file != NULL)
	{
		basefilename = get_basename(ctx->first_input_file);
	}
	else
	{
		basefilename = strdup("untitled");
	}

	if (basefilename == NULL)
	{
		free(new_out);
		return ctx->out;
	}

	char *filename = create_outfilename(basefilename, suffix, ext);
	free(basefilename);

	if (filename == NULL)
	{
		free(new_out);
		return ctx->out;
	}

	// Open the file
	new_out->filename = filename;
	new_out->fh = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
	if (new_out->fh == -1)
	{
		mprint("Error: Failed to open output file %s: %s\n", filename, strerror(errno));
		free(filename);
		free(new_out);
		return ctx->out;
	}

	mprint("Creating teletext output file: %s\n", filename);

	// Store in our array
	int idx = ctx->tlt_out_count;
	ctx->tlt_out[idx] = new_out;
	ctx->tlt_out_pages[idx] = teletext_page;
	ctx->tlt_srt_counter[idx] = 0;
	ctx->tlt_out_count++;

	// Write the subtitle file header
	write_subtitle_file_header(ctx, new_out);

	return new_out;
}

/**
 * Get the SRT counter for a specific teletext page (issue #665)
 * Returns pointer to the counter, or NULL if page not found
 */
unsigned int *get_teletext_srt_counter(struct encoder_ctx *ctx, uint16_t teletext_page)
{
	// If teletext_page is 0, use the default counter
	if (teletext_page == 0)
		return &ctx->srt_counter;

	// Check if we're using multi-page mode
	extern struct ccx_s_teletext_config tlt_config;
	if (tlt_config.num_user_pages <= 1 && !tlt_config.extract_all_pages)
		return &ctx->srt_counter;

	// Find the counter for this page
	for (int i = 0; i < ctx->tlt_out_count; i++)
	{
		if (ctx->tlt_out_pages[i] == teletext_page)
			return &ctx->tlt_srt_counter[i];
	}

	// Not found, use default counter
	return &ctx->srt_counter;
}

/**
 * Clean up all teletext output files (issue #665)
 */
void dinit_teletext_outputs(struct encoder_ctx *ctx)
{
	if (!ctx)
		return;

	for (int i = 0; i < ctx->tlt_out_count; i++)
	{
		if (ctx->tlt_out[i] != NULL)
		{
			// Write footer
			write_subtitle_file_footer(ctx, ctx->tlt_out[i]);

			// Close file
			if (ctx->tlt_out[i]->fh != -1)
			{
				close(ctx->tlt_out[i]->fh);
			}

			// Free filename
			if (ctx->tlt_out[i]->filename != NULL)
			{
				free(ctx->tlt_out[i]->filename);
			}

			free(ctx->tlt_out[i]);
			ctx->tlt_out[i] = NULL;
		}
	}
	ctx->tlt_out_count = 0;
}
