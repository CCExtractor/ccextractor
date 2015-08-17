#include "ccx_decoders_708_output.h"
#include "ccx_common_common.h"


int _dtvcc_is_row_empty(dtvcc_service_decoder *decoder, int row_index)
{
	for (int j = 0; j < DTVCC_SCREENGRID_COLUMNS; j++)
	{
		if (decoder->tv->chars[row_index][j] != ' ')
			return 0;
	}
	return 1;
}

int _dtvcc_is_caption_empty(dtvcc_service_decoder *decoder)
{
	for (int i = 0; i < DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!_dtvcc_is_row_empty(decoder, i))
			return 0;
	}
	return 1;
}

void _dtvcc_get_write_interval(dtvcc_service_decoder *decoder, int row_index, int *first, int *last)
{
	for (*first = 0; *first < DTVCC_SCREENGRID_COLUMNS; (*first)++)
		if (decoder->tv->chars[row_index][*first] != ' ')
			break;
	for (*last = DTVCC_SCREENGRID_COLUMNS - 1; *last > 0; (*last)--)
		if (decoder->tv->chars[row_index][*last] != ' ')
			break;
}

void _dtvcc_color_to_hex(int color, unsigned *hR, unsigned *hG, unsigned *hB)
{
	*hR = (unsigned) (color >> 4);
	*hG = (unsigned) ((color >> 2) & 0x3);
	*hB = (unsigned) (color & 0x3);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Color: %d [%06x] %u %u %u\n",
								 color, color, *hR, *hG, *hB);
}

void _dtvcc_write_tag_open(dtvcc_service_decoder *decoder)
{
	struct encoder_ctx *encoder = ccx_dtvcc_ctx.encoder;
	dtvcc_window *window = &decoder->windows[decoder->current_window];
	char *buf = (char *) encoder->buffer;
	size_t buf_len = 0;

	if (window->pen.italic)
		buf_len += sprintf(buf + buf_len, "<i>");

	if (window->pen.underline)
		buf_len += sprintf(buf + buf_len, "<u>");

	if (window->pen_color.fg_color != 0x3f && !encoder->no_font_color) //assuming white is default
	{
		unsigned red, green, blue;
		_dtvcc_color_to_hex(window->pen_color.fg_color, &red, &green, &blue);
		red = (255 / 3) * red;
		green = (255 / 3) * green;
		blue = (255 / 3) * blue;
		buf_len += sprintf(buf + buf_len, "<font color=\"%02x%02x%02x\">", red, green, blue);
	}
	write(decoder->fh, buf, buf_len);
}

void _dtvcc_write_tag_close(dtvcc_service_decoder *decoder)
{
	struct encoder_ctx *encoder = ccx_dtvcc_ctx.encoder;
	dtvcc_window *window = &decoder->windows[decoder->current_window];
	char *buf = (char *) encoder->buffer;
	size_t buf_len = 0;

	if (window->pen_color.fg_color != 0x3f && !encoder->no_font_color)
		buf_len += sprintf(buf + buf_len, "</font>");

	if (window->pen.underline)
		buf_len += sprintf(buf + buf_len, "</u>");

	if (window->pen.italic)
		buf_len += sprintf(buf + buf_len, "</i>");

	write(decoder->fh, buf, buf_len);
}

void _dtvcc_write_row(dtvcc_service_decoder *decoder, int row_index)
{
	struct encoder_ctx *encoder = ccx_dtvcc_ctx.encoder;
	char *buf = (char *) encoder->buffer;
	size_t buf_len = 0;
	memset(buf, 0, INITIAL_ENC_BUFFER_CAPACITY);
	int first, last;

	_dtvcc_get_write_interval(decoder, row_index, &first, &last);
	for (int j = first; j <= last; j++)
		buf[buf_len++] = decoder->tv->chars[row_index][j];

	write(decoder->fh, buf, buf_len);
}

void ccx_dtvcc_write_srt(dtvcc_service_decoder *decoder)
{
	struct encoder_ctx *encoder = ccx_dtvcc_ctx.encoder;

	if (_dtvcc_is_caption_empty(decoder))
		return;

	LLONG ms_start = decoder->current_visible_start_ms + decoder->subs_delay;
	LLONG ms_end = get_visible_end() + decoder->subs_delay - 1; //-1 to prevent overlap

	if (ms_start < 0) // Drop screens that because of subs_delay start too early
		return;

	decoder->cc_count++;

	char *buf = (char *) encoder->buffer;
	memset(buf, 0, INITIAL_ENC_BUFFER_CAPACITY);

	sprintf(buf, "%u\r\n", decoder->cc_count);
	mstime_sprintf(ms_start, "%02u:%02u:%02u,%03u", buf + strlen(buf));
	sprintf(buf + strlen(buf), " --> ");
	mstime_sprintf(ms_end, "%02u:%02u:%02u,%03u", buf + strlen(buf));
	sprintf(buf + strlen(buf), (char *) encoder->encoded_crlf);

	write(decoder->fh, buf, strlen(buf));

	for (int i = 0; i < DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!_dtvcc_is_row_empty(decoder, i))
		{
			_dtvcc_write_tag_open(decoder);
			_dtvcc_write_row(decoder, i);
			_dtvcc_write_tag_close(decoder);
			write(decoder->fh, encoder->encoded_crlf, encoder->encoded_crlf_length);
		}
	}
	write(decoder->fh, encoder->encoded_crlf, encoder->encoded_crlf_length);
}

void ccx_dtvcc_write_debug(dtvcc_service_decoder *decoder)
{
	char tbuf1[SUBLINESIZE],
			tbuf2[SUBLINESIZE];

	print_mstime2buf(decoder->current_visible_start_ms, tbuf1);
	print_mstime2buf(get_visible_end(), tbuf2);

	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\r%s --> %s\n", tbuf1, tbuf2);
	for (int i = 0; i < DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!_dtvcc_is_row_empty(decoder, i))
		{
			int first, last;
			_dtvcc_get_write_interval(decoder, i, &first, &last);
			for (int j = first; j <= last; j++)
				ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "%c", decoder->tv->chars[i][j]);
			ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\n");
		}
	}
}

void ccx_dtvcc_write_transcript(dtvcc_service_decoder *decoder)
{
	struct encoder_ctx *encoder = ccx_dtvcc_ctx.encoder;

	if (_dtvcc_is_caption_empty(decoder))
		return;

	LLONG ms_start = decoder->current_visible_start_ms + decoder->subs_delay;
	LLONG ms_end = get_visible_end() + decoder->subs_delay - 1; //-1 to prevent overlap

	if (ms_start < 0) // Drop screens that because of subs_delay start too early
		return;

	decoder->cc_count++;

	char *buf = (char *) encoder->buffer;

	for (int i = 0; i < DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!_dtvcc_is_row_empty(decoder, i))
		{
			buf[0] = 0;

			if (encoder->transcript_settings->showStartTime)
				mstime_sprintf(ms_start, "%02u:%02u:%02u,%03u|", buf + strlen(buf));

			if (encoder->transcript_settings->showEndTime)
				mstime_sprintf(ms_end, "%02u:%02u:%02u,%03u|", buf + strlen(buf));

			if (encoder->transcript_settings->showCC)
				sprintf(buf + strlen(buf), "CC1|"); //always CC1 because CEA-708 is field-independent

			if (encoder->transcript_settings->showMode)
				sprintf(buf + strlen(buf), "POP|"); //TODO caption mode(pop, rollup, etc.)

			if (strlen(buf))
				write(decoder->fh, buf, strlen(buf));

			_dtvcc_write_row(decoder, i);
			write(decoder->fh, encoder->encoded_crlf, encoder->encoded_crlf_length);
		}
	}
	write(decoder->fh, encoder->encoded_crlf, encoder->encoded_crlf_length);
}

void _dtvcc_write_sami_header(dtvcc_service_decoder *decoder)
{
	struct encoder_ctx *encoder = ccx_dtvcc_ctx.encoder;
	char *buf = (char *) encoder->buffer;
	memset(buf, 0, INITIAL_ENC_BUFFER_CAPACITY);
	size_t buf_len = 0;

	buf_len += sprintf(buf + buf_len, "<sami>%s", encoder->encoded_crlf);
	buf_len += sprintf(buf + buf_len, "<head>%s", encoder->encoded_crlf);
	buf_len += sprintf(buf + buf_len, "<style type=\"text/css\">%s", encoder->encoded_crlf);
	buf_len += sprintf(buf + buf_len, "<!--%s", encoder->encoded_crlf);
	buf_len += sprintf(buf + buf_len,
					   "p {margin-left: 16pt; margin-right: 16pt; margin-bottom: 16pt; margin-top: 16pt;%s",
					   encoder->encoded_crlf);
	buf_len += sprintf(buf + buf_len,
					   "text-align: center; font-size: 18pt; font-family: arial; font-weight: bold; color: #f0f0f0;}%s",
					   encoder->encoded_crlf);
	buf_len += sprintf(buf + buf_len, ".unknowncc {Name:Unknown; lang:en-US; SAMIType:CC;}%s", encoder->encoded_crlf);
	buf_len += sprintf(buf + buf_len, "-->%s", encoder->encoded_crlf);
	buf_len += sprintf(buf + buf_len, "</style>%s", encoder->encoded_crlf);
	buf_len += sprintf(buf + buf_len, "</head>%s%s", encoder->encoded_crlf, encoder->encoded_crlf);
	buf_len += sprintf(buf + buf_len, "<body>%s", encoder->encoded_crlf);

	write(decoder->fh, buf, buf_len);
}

void _dtvcc_write_sami_footer(dtvcc_service_decoder *decoder)
{
	struct encoder_ctx *encoder = ccx_dtvcc_ctx.encoder;
	char *buf = (char *) encoder->buffer;
	sprintf(buf, "</body></sami>");
	write(decoder->fh, buf, strlen(buf));
	write(decoder->fh, encoder->encoded_crlf, encoder->encoded_crlf_length);
}

void ccx_dtvcc_write_sami(dtvcc_service_decoder *decoder)
{
	struct encoder_ctx *encoder = ccx_dtvcc_ctx.encoder;

	if (_dtvcc_is_caption_empty(decoder))
		return;

	LLONG ms_start = decoder->current_visible_start_ms + decoder->subs_delay;
	LLONG ms_end = get_visible_end() + decoder->subs_delay - 1; //-1 to prevent overlap

	if (ms_start < 0) // Drop screens that because of subs_delay start too early
		return;

	if (!decoder->cc_count)
		_dtvcc_write_sami_header(decoder);

	decoder->cc_count++;

	char *buf = (char *) encoder->buffer;

	buf[0] = 0;
	sprintf(buf, "<sync start=%llu><p class=\"unknowncc\">%s",
			(unsigned long long) ms_start,
			encoder->encoded_crlf);
	write(decoder->fh, buf, strlen(buf));

	_dtvcc_write_tag_open(decoder);

	for (int i = 0; i < DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!_dtvcc_is_row_empty(decoder, i))
		{
			_dtvcc_write_row(decoder, i);
			write(decoder->fh, encoder->encoded_br, encoder->encoded_br_length);
			write(decoder->fh, encoder->encoded_crlf, encoder->encoded_crlf_length);
		}
	}

	_dtvcc_write_tag_close(decoder);

	sprintf(buf, "<sync start=%llu><p class=\"unknowncc\">&nbsp;</p></sync>%s%s",
			(unsigned long long) ms_end, encoder->encoded_crlf, encoder->encoded_crlf);
	write(decoder->fh, buf, strlen(buf));
}

void ccx_dtvcc_write(dtvcc_service_decoder *decoder)
{
	switch (decoder->output_format)
	{
		case CCX_OF_NULL:
			break;
		case CCX_OF_SRT:
			ccx_dtvcc_write_srt(decoder);
			break;
		case CCX_OF_TRANSCRIPT:
			ccx_dtvcc_write_transcript(decoder);
			break;
		case CCX_OF_SAMI:
			ccx_dtvcc_write_sami(decoder);
			break;
		default:
			ccx_dtvcc_write_debug(decoder);
			break;
	}
}

void ccx_dtvcc_write_done(dtvcc_service_decoder *decoder)
{
	switch (decoder->output_format)
	{
		case CCX_OF_SAMI:
			_dtvcc_write_sami_footer(decoder);
			break;
		default:
			ccx_common_logging.debug_ftn(
					CCX_DMT_708, "[CEA-708] ccx_dtvcc_write_done: no handling required\n");
			break;
	}
}
