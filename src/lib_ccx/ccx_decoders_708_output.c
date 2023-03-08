#include "ccx_decoders_708_output.h"
#include "ccx_decoders_708.h"
#include "ccx_encoders_common.h"
#include "utility.h"
#include "ccx_common_common.h"

#if !defined(DISABLE_RUST) && defined(WIN32)
extern void ccxr_close_handle(void *handle);
#endif

int dtvcc_is_row_empty(dtvcc_tv_screen *tv, int row_index)
{
	for (int j = 0; j < CCX_DTVCC_SCREENGRID_COLUMNS; j++)
	{
		if (CCX_DTVCC_SYM_IS_SET(tv->chars[row_index][j]))
			return 0;
	}
	return 1;
}

int dtvcc_is_screen_empty(dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!dtvcc_is_row_empty(tv, i))
		{
			// we will write subtitle
			encoder->cea_708_counter++;
			return 0;
		}
	}
	return 1;
}

void dtvcc_get_write_interval(dtvcc_tv_screen *tv, int row_index, int *first, int *last)
{
	for (*first = 0; *first < CCX_DTVCC_SCREENGRID_COLUMNS; (*first)++)
		if (CCX_DTVCC_SYM_IS_SET(tv->chars[row_index][*first]))
			break;
	for (*last = CCX_DTVCC_SCREENGRID_COLUMNS - 1; *last > 0; (*last)--)
		if (CCX_DTVCC_SYM_IS_SET(tv->chars[row_index][*last]))
			break;
}

void dtvcc_color_to_hex(int color, unsigned *hR, unsigned *hG, unsigned *hB)
{
	*hR = (unsigned)(color >> 4);
	*hG = (unsigned)((color >> 2) & 0x3);
	*hB = (unsigned)(color & 0x3);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Color: %d [%06x] %u %u %u\n",
				     color, color, *hR, *hG, *hB);
}

void dtvcc_change_pen_colors(dtvcc_tv_screen *tv, dtvcc_pen_color pen_color, int row_index, int column_index, struct encoder_ctx *encoder, size_t *buf_len, int open)
{
	if (encoder->no_font_color)
		return;

	char *buf = (char *)encoder->buffer;

	dtvcc_pen_color new_pen_color;
	if (column_index >= CCX_DTVCC_SCREENGRID_COLUMNS)
		new_pen_color = dtvcc_default_pen_color;
	else
		new_pen_color = tv->pen_colors[row_index][column_index];
	if (pen_color.fg_color != new_pen_color.fg_color)
	{
		if (pen_color.fg_color != 0x3f && !open)
			(*buf_len) += sprintf(buf + (*buf_len), "</font>"); // should close older non-white color

		if (new_pen_color.fg_color != 0x3f && open)
		{
			unsigned red, green, blue;
			dtvcc_color_to_hex(new_pen_color.fg_color, &red, &green, &blue);
			red = (255 / 3) * red;
			green = (255 / 3) * green;
			blue = (255 / 3) * blue;
			(*buf_len) += sprintf(buf + (*buf_len), "<font color=\"#%02x%02x%02x\">", red, green, blue);
		}
	}
}

void dtvcc_change_pen_attribs(dtvcc_tv_screen *tv, dtvcc_pen_attribs pen_attribs, int row_index, int column_index, struct encoder_ctx *encoder, size_t *buf_len, int open)
{
	if (encoder->no_font_color)
		return;

	char *buf = (char *)encoder->buffer;

	dtvcc_pen_attribs new_pen_attribs;
	if (column_index >= CCX_DTVCC_SCREENGRID_COLUMNS)
		new_pen_attribs = dtvcc_default_pen_attribs;
	else
		new_pen_attribs = tv->pen_attribs[row_index][column_index];
	if (pen_attribs.italic != new_pen_attribs.italic)
	{
		if (pen_attribs.italic && !open)
			(*buf_len) += sprintf(buf + (*buf_len), "</i>");
		if (!pen_attribs.italic && open)
			(*buf_len) += sprintf(buf + (*buf_len), "<i>");
	}
	if (pen_attribs.underline != new_pen_attribs.underline)
	{
		if (pen_attribs.underline && !open)
			(*buf_len) += sprintf(buf + (*buf_len), "</u>");
		if (!pen_attribs.underline && open)
			(*buf_len) += sprintf(buf + (*buf_len), "<u>");
	}
}

size_t write_utf16_char(unsigned short utf16_char, char *out)
{
	if ((utf16_char >> 8) != 0)
	{
		out[0] = (unsigned char)(utf16_char >> 8);
		out[1] = (unsigned char)(utf16_char & 0xff);
		return 2;
	}
	else
	{
		out[0] = (unsigned char)(utf16_char);
		return 1;
	}
}

void dtvcc_write_row(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, int row_index, struct encoder_ctx *encoder, int use_colors)
{
	dtvcc_tv_screen *tv = decoder->tv;
	char *buf = (char *)encoder->buffer;
	size_t buf_len = 0;
	memset(buf, 0, INITIAL_ENC_BUFFER_CAPACITY * sizeof(char));
	int first, last;

	int fd = encoder->dtvcc_writers[tv->service_number - 1].fd;

	dtvcc_pen_color pen_color = dtvcc_default_pen_color;
	dtvcc_pen_attribs pen_attribs = dtvcc_default_pen_attribs;
	dtvcc_get_write_interval(tv, row_index, &first, &last);

	for (int i = 0; i < last + 1; i++)
	{

		if (use_colors)
			dtvcc_change_pen_colors(tv, pen_color, row_index, i, encoder, &buf_len, 0);
		dtvcc_change_pen_attribs(tv, pen_attribs, row_index, i, encoder, &buf_len, 0);
		dtvcc_change_pen_attribs(tv, pen_attribs, row_index, i, encoder, &buf_len, 1);
		if (use_colors)
			dtvcc_change_pen_colors(tv, pen_color, row_index, i, encoder, &buf_len, 1);

		pen_color = tv->pen_colors[row_index][i];
		pen_attribs = tv->pen_attribs[row_index][i];
		if (i < first)
		{
			size_t size = write_utf16_char(' ', buf + buf_len);
			buf_len += size;
		}
		else
		{
			size_t size = write_utf16_char(tv->chars[row_index][i].sym, buf + buf_len);
			buf_len += size;
		}
	}

	// there can be unclosed tags or colors after the last symbol in a row
	if (use_colors)
		dtvcc_change_pen_colors(tv, pen_color, row_index, CCX_DTVCC_SCREENGRID_COLUMNS, encoder, &buf_len, 0);
	dtvcc_change_pen_attribs(tv, pen_attribs, row_index, CCX_DTVCC_SCREENGRID_COLUMNS, encoder, &buf_len, 0);
	// Tags can still be crossed e.g <f><i>text</f></i>, but testing HTML code has shown that they still are handled correctly.
	// In case of errors fix it once and for all.

	if (writer->cd != (iconv_t)-1)
	{
		char *encoded_buf = calloc(INITIAL_ENC_BUFFER_CAPACITY, sizeof(char));
		if (!encoded_buf)
			ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "dtvcc_write_row");

		char *encoded_buf_start = encoded_buf;

		size_t in_bytes_left = buf_len;
		size_t out_bytes_left = INITIAL_ENC_BUFFER_CAPACITY * sizeof(char);

		size_t result = iconv(writer->cd, &buf, &in_bytes_left, &encoded_buf, &out_bytes_left);

		if (result == -1)
			ccx_common_logging.log_ftn("[CEA-708] dtvcc_write_row: "
						   "conversion failed: %s\n",
						   strerror(errno));

		write_wrapped(fd, encoded_buf_start, encoded_buf - encoded_buf_start);

		free(encoded_buf_start);
	}
	else
	{
		write_wrapped(fd, buf, buf_len);
	}
}

void dtvcc_write_srt(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder)
{
	dtvcc_tv_screen *tv = decoder->tv;
	if (dtvcc_is_screen_empty(tv, encoder))
		return;

	if (tv->time_ms_show + encoder->subs_delay < 0)
		return;

	char *buf = (char *)encoder->buffer;
	memset(buf, 0, INITIAL_ENC_BUFFER_CAPACITY);

	sprintf(buf, "%u%s", encoder->cea_708_counter, encoder->encoded_crlf);
	print_mstime_buff(tv->time_ms_show + encoder->subs_delay,
			  "%02u:%02u:%02u,%03u", buf + strlen(buf));
	sprintf(buf + strlen(buf), " --> ");
	print_mstime_buff(tv->time_ms_hide + encoder->subs_delay,
			  "%02u:%02u:%02u,%03u", buf + strlen(buf));
	sprintf(buf + strlen(buf), "%s", (char *)encoder->encoded_crlf);

	write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, strlen(buf));

	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!dtvcc_is_row_empty(tv, i))
		{
			dtvcc_write_row(writer, decoder, i, encoder, 1);
			write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd,
				      encoder->encoded_crlf, encoder->encoded_crlf_length);
		}
	}
	write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd,
		      encoder->encoded_crlf, encoder->encoded_crlf_length);
}

void dtvcc_write_debug(dtvcc_tv_screen *tv)
{
	char tbuf1[SUBLINESIZE],
	    tbuf2[SUBLINESIZE];

	print_mstime_buff(tv->time_ms_show, "%02u:%02u:%02u:%03u", tbuf1);
	print_mstime_buff(tv->time_ms_hide, "%02u:%02u:%02u:%03u", tbuf2);

	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\r%s --> %s\n", tbuf1, tbuf2);
	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!dtvcc_is_row_empty(tv, i))
		{
			int first, last;
			dtvcc_get_write_interval(tv, i, &first, &last);
			for (int j = first; j <= last; j++)
				ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "%c", tv->chars[i][j]);
			ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\n");
		}
	}
}

void dtvcc_write_transcript(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder)
{
	dtvcc_tv_screen *tv = decoder->tv;
	if (dtvcc_is_screen_empty(tv, encoder))
		return;

	if (tv->time_ms_show + encoder->subs_delay < 0) // Drop screens that because of subs_delay start too early
		return;

	char *buf = (char *)encoder->buffer;

	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!dtvcc_is_row_empty(tv, i))
		{
			buf[0] = 0;

			if (encoder->transcript_settings->showStartTime)
				print_mstime_buff(tv->time_ms_show + encoder->subs_delay,
						  "%02u:%02u:%02u,%03u|", buf + strlen(buf));

			if (encoder->transcript_settings->showEndTime)
				print_mstime_buff(tv->time_ms_hide + encoder->subs_delay,
						  "%02u:%02u:%02u,%03u|", buf + strlen(buf));

			if (encoder->transcript_settings->showCC)
				sprintf(buf + strlen(buf), "CC1|"); // always CC1 because CEA-708 is field-independent

			if (encoder->transcript_settings->showMode)
				sprintf(buf + strlen(buf), "POP|"); // TODO caption mode(pop, rollup, etc.)

			const size_t buf_len = strlen(buf);
			if (buf_len != 0)
				write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, buf_len);

			dtvcc_write_row(writer, decoder, i, encoder, 0);
			write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd,
				      encoder->encoded_crlf, encoder->encoded_crlf_length);
		}
	}
}

void dtvcc_write_sami_header(dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	char *buf = (char *)encoder->buffer;
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

	write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, buf_len);
}

void dtvcc_write_sami_footer(dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	char *buf = (char *)encoder->buffer;
	sprintf(buf, "</body></sami>");
	write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, strlen(buf));
	write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd,
		      encoder->encoded_crlf, encoder->encoded_crlf_length);
}

void dtvcc_write_sami(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder)
{
	dtvcc_tv_screen *tv = decoder->tv;
	if (dtvcc_is_screen_empty(tv, encoder))
		return;

	if (tv->time_ms_show + encoder->subs_delay < 0)
		return;

	if (tv->cc_count == 1)
		dtvcc_write_sami_header(tv, encoder);

	char *buf = (char *)encoder->buffer;

	buf[0] = 0;
	sprintf(buf, "<sync start=%llu><p class=\"unknowncc\">%s",
		(unsigned long long)tv->time_ms_show + encoder->subs_delay,
		encoder->encoded_crlf);
	write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, strlen(buf));

	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
	{
		if (!dtvcc_is_row_empty(tv, i))
		{
			dtvcc_write_row(writer, decoder, i, encoder, 1);
			write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd,
				      encoder->encoded_br, encoder->encoded_br_length);
			write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd,
				      encoder->encoded_crlf, encoder->encoded_crlf_length);
		}
	}

	sprintf(buf, "<sync start=%llu><p class=\"unknowncc\">&nbsp;</p></sync>%s%s",
		(unsigned long long)tv->time_ms_hide + encoder->subs_delay,
		encoder->encoded_crlf, encoder->encoded_crlf);
	write_wrapped(encoder->dtvcc_writers[tv->service_number - 1].fd, buf, strlen(buf));
}

void dtvcc_write(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder)
{
	switch (encoder->write_format)
	{
		case CCX_OF_NULL:
			break;
		case CCX_OF_SRT:
			dtvcc_write_srt(writer, decoder, encoder);
			break;
		case CCX_OF_TRANSCRIPT:
			dtvcc_write_transcript(writer, decoder, encoder);
			break;
		case CCX_OF_SAMI:
			dtvcc_write_sami(writer, decoder, encoder);
			break;
		case CCX_OF_MCC:
			printf("REALLY BAD... [%s:%d]\n", __FILE__, __LINE__);
			break;
		default:
			dtvcc_write_debug(decoder->tv);
			break;
	}
}

void dtvcc_write_done(dtvcc_tv_screen *tv, struct encoder_ctx *encoder)
{
	switch (encoder->write_format)
	{
		case CCX_OF_SAMI:
			dtvcc_write_sami_footer(tv, encoder);
			break;
		default:
			ccx_common_logging.debug_ftn(
			    CCX_DMT_708, "[CEA-708] dtvcc_write_done: no handling required\n");
			break;
	}
}

void dtvcc_writer_init(dtvcc_writer_ctx *writer,
		       char *base_filename,
		       int program_number,
		       int service_number,
		       enum ccx_output_format write_format,
		       struct encoder_cfg *cfg)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_writer_init\n");
	writer->fd = -1;
	writer->cd = (iconv_t)-1;
	if ((write_format == CCX_OF_NULL) || (write_format == CCX_OF_MCC))
	{
		writer->filename = NULL;
		return;
	}

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_writer_init: "
						  "[%s][%d][%d]\n",
				     base_filename, program_number, service_number);

	const char *ext = get_file_extension(write_format);
	char suffix[32];
	sprintf(suffix, CCX_DTVCC_FILENAME_TEMPLATE, program_number, service_number);

	writer->filename = create_outfilename(base_filename, suffix, ext);
	if (!writer->filename)
		ccx_common_logging.fatal_ftn(
		    EXIT_NOT_ENOUGH_MEMORY, "[CEA-708] dtvcc_decoder_init_write: not enough memory");

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_writer_init: inited [%s]\n", writer->filename);

	char *charset = cfg->all_services_charset ? cfg->all_services_charset : cfg->services_charsets[service_number - 1];

#ifndef DISABLE_RUST
	writer->fhandle = NULL;
	writer->charset = charset;
#endif

	if (charset)
	{
		writer->cd = iconv_open("UTF-8", charset);
		if (writer->cd == (iconv_t)-1)
		{
			ccx_common_logging.fatal_ftn(EXIT_FAILURE, "[CEA-708] dtvcc_init: "
								   "can't create iconv for charset \"%s\": %s\n",
						     charset, strerror(errno));
		}
	}
}

void dtvcc_writer_cleanup(dtvcc_writer_ctx *writer)
{
	if (writer->fd >= 0 && writer->fd != STDOUT_FILENO)
		close(writer->fd);
#if !defined(DISABLE_RUST) && defined(WIN32)
	ccxr_close_handle(writer->fhandle);
	writer->charset = NULL;
#endif
	free(writer->filename);
	if (writer->cd == (iconv_t)-1)
	{
		// TODO nothing to do here
	}
	else
		iconv_close(writer->cd);
}

void dtvcc_writer_output(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_writer_output: "
						  "writing... [%s][%d]\n",
				     writer->filename, writer->fd);

	if (!writer->filename && writer->fd < 0)
		return;

	if (writer->filename && writer->fd < 0) // first request to write
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
							  "dtvcc_writer_output: creating %s\n",
					     writer->filename);
		writer->fd = open(writer->filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
		if (writer->fd == -1)
		{
			ccx_common_logging.fatal_ftn(
			    CCX_COMMON_EXIT_FILE_CREATION_FAILED, "[CEA-708] Failed to open a file\n");
		}
		if (!encoder->no_bom)
			write_wrapped(writer->fd, UTF8_BOM, sizeof(UTF8_BOM));
	}

	dtvcc_write(writer, decoder, encoder);
}
