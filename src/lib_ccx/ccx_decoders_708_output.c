#include "ccx_decoders_708_output.h"
#include "ccx_encoders_common.h"
#include "utility.h"
#include "ccx_common_common.h"

int _dtvcc_is_row_empty(dtvcc_tv_screen *tv, int row_index)
{
	for (int j = 0; j < CCX_DTVCC_SCREENGRID_COLUMNS; j++)
	{
		if (CCX_DTVCC_SYM_IS_SET(tv->chars[row_index][j]))
			return 0;
	}
	return 1;
}

int _dtvcc_is_screen_empty(dtvcc_tv_screen *tv)
{
	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!_dtvcc_is_row_empty(tv, i))
			return 0;
	}
	return 1;
}

void _dtvcc_get_write_interval(dtvcc_tv_screen *tv, int row_index, int *first, int *last)
{
	for (*first = 0; *first < CCX_DTVCC_SCREENGRID_COLUMNS; (*first)++)
		if (CCX_DTVCC_SYM_IS_SET(tv->chars[row_index][*first]))
			break;
	for (*last = CCX_DTVCC_SCREENGRID_COLUMNS - 1; *last > 0; (*last)--)
		if (CCX_DTVCC_SYM_IS_SET(tv->chars[row_index][*last]))
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

void _dtvcc_write_tag_open(dtvcc_tv_screen *tv, struct encoder_ctx *encoder, int row_index)
{
	char *buf = (char *) encoder->buffer;
	size_t buf_len = 0;

	if (tv->pen_attribs[row_index].italic)
		buf_len += sprintf(buf + buf_len, "<i>");

	if (tv->pen_attribs[row_index].underline)
		buf_len += sprintf(buf + buf_len, "<u>");

	if (tv->pen_colors[row_index].fg_color != 0x3f && !encoder->no_font_color) //assuming white is default
	{
		unsigned red, green, blue;
		_dtvcc_color_to_hex(tv->pen_colors[row_index].fg_color, &red, &green, &blue);
		red = (255 / 3) * red;
		green = (255 / 3) * green;
		blue = (255 / 3) * blue;
		buf_len += sprintf(buf + buf_len, "<font color=\"%02x%02x%02x\">", red, green, blue);
	}
	write(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, buf_len);
}

void _dtvcc_write_tag_close(dtvcc_tv_screen *tv, struct encoder_ctx *encoder, int row_index)
{
	char *buf = (char *) encoder->buffer;
	size_t buf_len = 0;

	if (tv->pen_colors[row_index].fg_color != 0x3f && !encoder->no_font_color)
		buf_len += sprintf(buf + buf_len, "</font>");

	if (tv->pen_attribs[row_index].underline)
		buf_len += sprintf(buf + buf_len, "</u>");

	if (tv->pen_attribs[row_index].italic)
		buf_len += sprintf(buf + buf_len, "</i>");

	write(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, buf_len);
}

void _dtvcc_write_row(ccx_dtvcc_writer_ctx *writer, dtvcc_tv_screen *tv, int row_index, struct encoder_ctx *encoder)
{
	char *buf = (char *) encoder->buffer;
	size_t buf_len = 0;
	memset(buf, 0, INITIAL_ENC_BUFFER_CAPACITY * sizeof(char));
	int first, last;

	_dtvcc_get_write_interval(tv, row_index, &first, &last);
	for (int j = first; j <= last; j++)
	{
		if (CCX_DTVCC_SYM_IS_16(tv->chars[row_index][j]))
		{
			buf[buf_len++] = CCX_DTVCC_SYM_16_FIRST(tv->chars[row_index][j]);
			buf[buf_len++] = CCX_DTVCC_SYM_16_SECOND(tv->chars[row_index][j]);
		}
		else
		{
			buf[buf_len++] = CCX_DTVCC_SYM(tv->chars[row_index][j]);
		}
	}

	int fd = encoder->dtvcc_writers[tv->service_number - 1].fd;

	if (writer->cd != (iconv_t) -1)
	{
		char *encoded_buf = calloc(INITIAL_ENC_BUFFER_CAPACITY, sizeof(char));
		if (!encoded_buf)
			ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "_dtvcc_write_row");

		char *encoded_buf_start = encoded_buf;

		size_t in_bytes_left = buf_len;
		size_t out_bytes_left = INITIAL_ENC_BUFFER_CAPACITY * sizeof(char);

		size_t result = iconv(writer->cd, &buf, &in_bytes_left, &encoded_buf, &out_bytes_left);

		if (result == -1)
			ccx_common_logging.log_ftn("[CEA-708] _dtvcc_write_row: "
					"conversion failed: %s\n", strerror(errno));

		write(fd, encoded_buf_start, encoded_buf - encoded_buf_start);

		free(encoded_buf_start);
	}
	else
	{
		write(fd, buf, buf_len);
	}
}

void ccx_dtvcc_write_srt(ccx_dtvcc_writer_ctx *writer, dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	if (_dtvcc_is_screen_empty(tv))
		return;

	if (tv->time_ms_show + encoder->subs_delay < 0)
		return;

	char *buf = (char *) encoder->buffer;
	memset(buf, 0, INITIAL_ENC_BUFFER_CAPACITY);

	sprintf(buf, "%u%s", tv->cc_count, encoder->encoded_crlf);
	mstime_sprintf(tv->time_ms_show + encoder->subs_delay,
				   "%02u:%02u:%02u,%03u", buf + strlen(buf));
	sprintf(buf + strlen(buf), " --> ");
	mstime_sprintf(tv->time_ms_hide + encoder->subs_delay,
				   "%02u:%02u:%02u,%03u", buf + strlen(buf));
	sprintf(buf + strlen(buf), (char *) encoder->encoded_crlf);

	write(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, strlen(buf));

	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!_dtvcc_is_row_empty(tv, i))
		{
			_dtvcc_write_tag_open(tv, encoder, i);
			_dtvcc_write_row(writer, tv, i, encoder);
			_dtvcc_write_tag_close(tv, encoder, i);
			write(encoder->dtvcc_writers[tv->service_number - 1].fd,
				  encoder->encoded_crlf, encoder->encoded_crlf_length);
		}
	}
	write(encoder->dtvcc_writers[tv->service_number - 1].fd,
		  encoder->encoded_crlf, encoder->encoded_crlf_length);
}

void ccx_dtvcc_write_debug(dtvcc_tv_screen *tv)
{
	char tbuf1[SUBLINESIZE],
			tbuf2[SUBLINESIZE];

	print_mstime2buf(tv->time_ms_show, tbuf1);
	print_mstime2buf(tv->time_ms_hide, tbuf2);

	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\r%s --> %s\n", tbuf1, tbuf2);
	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!_dtvcc_is_row_empty(tv, i))
		{
			int first, last;
			_dtvcc_get_write_interval(tv, i, &first, &last);
			for (int j = first; j <= last; j++)
				ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "%c", tv->chars[i][j]);
			ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\n");
		}
	}
}

void ccx_dtvcc_write_transcript(ccx_dtvcc_writer_ctx *writer, dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	if (_dtvcc_is_screen_empty(tv))
		return;

	if (tv->time_ms_show + encoder->subs_delay < 0) // Drop screens that because of subs_delay start too early
		return;

	char *buf = (char *) encoder->buffer;

	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!_dtvcc_is_row_empty(tv, i))
		{
			buf[0] = 0;

			if (encoder->transcript_settings->showStartTime)
				mstime_sprintf(tv->time_ms_show + encoder->subs_delay,
							   "%02u:%02u:%02u,%03u|", buf + strlen(buf));

			if (encoder->transcript_settings->showEndTime)
				mstime_sprintf(tv->time_ms_hide + encoder->subs_delay,
							   "%02u:%02u:%02u,%03u|", buf + strlen(buf));

			if (encoder->transcript_settings->showCC)
				sprintf(buf + strlen(buf), "CC1|"); //always CC1 because CEA-708 is field-independent

			if (encoder->transcript_settings->showMode)
				sprintf(buf + strlen(buf), "POP|"); //TODO caption mode(pop, rollup, etc.)

			if (strlen(buf))
				write(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, strlen(buf));

			_dtvcc_write_row(writer, tv, i, encoder);
			write(encoder->dtvcc_writers[tv->service_number - 1].fd,
				  encoder->encoded_crlf, encoder->encoded_crlf_length);
		}
	}
}

void _dtvcc_write_sami_header(dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
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

	write(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, buf_len);
}

void _dtvcc_write_sami_footer(dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	char *buf = (char *) encoder->buffer;
	sprintf(buf, "</body></sami>");
	write(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, strlen(buf));
	write(encoder->dtvcc_writers[tv->service_number - 1].fd,
		  encoder->encoded_crlf, encoder->encoded_crlf_length);
}

void ccx_dtvcc_write_sami(ccx_dtvcc_writer_ctx *writer, dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	if (_dtvcc_is_screen_empty(tv))
		return;

	if (tv->time_ms_show + encoder->subs_delay < 0)
		return;

	if (tv->cc_count == 1)
		_dtvcc_write_sami_header(tv, encoder);

	char *buf = (char *) encoder->buffer;

	buf[0] = 0;
	sprintf(buf, "<sync start=%llu><p class=\"unknowncc\">%s",
			(unsigned long long) tv->time_ms_show + encoder->subs_delay,
			encoder->encoded_crlf);
	write(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, strlen(buf));

	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!_dtvcc_is_row_empty(tv, i))
		{
			_dtvcc_write_tag_open(tv, encoder, i);
			_dtvcc_write_row(writer, tv, i, encoder);
			_dtvcc_write_tag_close(tv, encoder, i);
			write(encoder->dtvcc_writers[tv->service_number - 1].fd,
				  encoder->encoded_br, encoder->encoded_br_length);
			write(encoder->dtvcc_writers[tv->service_number - 1].fd,
				  encoder->encoded_crlf, encoder->encoded_crlf_length);
		}
	}

	sprintf(buf, "<sync start=%llu><p class=\"unknowncc\">&nbsp;</p></sync>%s%s",
			(unsigned long long) tv->time_ms_hide + encoder->subs_delay,
			encoder->encoded_crlf, encoder->encoded_crlf);
	write(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, strlen(buf));
}

void _ccx_dtvcc_write(ccx_dtvcc_writer_ctx *writer, dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	switch (encoder->write_format)
	{
		case CCX_OF_NULL:
			break;
		case CCX_OF_SRT:
			ccx_dtvcc_write_srt(writer, tv, encoder);
			break;
		case CCX_OF_TRANSCRIPT:
			ccx_dtvcc_write_transcript(writer, tv, encoder);
			break;
		case CCX_OF_SAMI:
			ccx_dtvcc_write_sami(writer, tv, encoder);
			break;
		default:
			ccx_dtvcc_write_debug(tv);
			break;
	}
}

void ccx_dtvcc_write_done(dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	switch (encoder->write_format)
	{
		case CCX_OF_SAMI:
			_dtvcc_write_sami_footer(tv, encoder);
			break;
		default:
			ccx_common_logging.debug_ftn(
					CCX_DMT_708, "[CEA-708] ccx_dtvcc_write_done: no handling required\n");
			break;
	}
}

void ccx_dtvcc_writer_init(ccx_dtvcc_writer_ctx *writer,
						   char *base_filename,
						   int program_number,
						   int service_number,
						   enum ccx_output_format write_format,
						   struct encoder_cfg *cfg)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] ccx_dtvcc_writer_init\n");
	writer->fd = -1;
	writer->cd = (iconv_t) -1;
	if (write_format == CCX_OF_NULL)
	{
		writer->filename = NULL;
		return;
	}

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] ccx_dtvcc_writer_init: "
			"[%s][%d][%d]\n", base_filename, program_number, service_number);

	char *ext = get_file_extension(write_format);
	char suffix[32];
	sprintf(suffix, CCX_DTVCC_FILENAME_TEMPLATE, program_number, service_number);

	writer->filename = create_outfilename(base_filename, suffix, ext);
	if (!writer->filename)
		ccx_common_logging.fatal_ftn(
				EXIT_NOT_ENOUGH_MEMORY, "[CEA-708] _dtvcc_decoder_init_write: not enough memory");

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] ccx_dtvcc_writer_init: inited [%s]\n", writer->filename);

	char *charset = cfg->all_services_charset ?
					cfg->all_services_charset :
					cfg->services_charsets[service_number - 1];

	if (charset)
	{
		writer->cd = iconv_open("UTF-8", charset);
		if (writer->cd == (iconv_t) -1)
		{
			ccx_common_logging.fatal_ftn(EXIT_FAILURE, "[CEA-708] dtvcc_init: "
												 "can't create iconv for charset \"%s\": %s\n",
										 charset, strerror(errno));
		}
	}

	free(ext);
}

void ccx_dtvcc_writer_cleanup(ccx_dtvcc_writer_ctx *writer)
{
	if (writer->fd >= 0 && writer->fd != STDOUT_FILENO)
		close(writer->fd);
	free(writer->filename);
	if (writer->cd == (iconv_t) -1)
	{
		//TODO nothing to do here
	}
	else
		iconv_close(writer->cd);
}

void ccx_dtvcc_writer_output(ccx_dtvcc_writer_ctx *writer, dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] ccx_dtvcc_writer_output: "
			"writing... [%s][%d]\n", writer->filename, writer->fd);

	if (!writer->filename && writer->fd < 0)
		return;

	if (writer->filename && writer->fd < 0) //first request to write
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
				"ccx_dtvcc_writer_output: creating %s\n", writer->filename);
		writer->fd = open(writer->filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
		if (writer->fd == -1)
		{
			ccx_common_logging.fatal_ftn(
					CCX_COMMON_EXIT_FILE_CREATION_FAILED, "[CEA-708] Failed to open a file\n");
		}
		if (!encoder->no_bom)
			write(writer->fd, UTF8_BOM, sizeof(UTF8_BOM));
	}

	_ccx_dtvcc_write(writer, tv, encoder);
}