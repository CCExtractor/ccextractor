#ifndef _CCX_DECODERS_708_OUTPUT_H_
#define _CCX_DECODERS_708_OUTPUT_H_

#include "ccx_decoders_708.h"
#include "ccx_encoders_common.h"
#include "ccx_common_option.h"

void dtvcc_write_done(dtvcc_tv_screen *tv, struct encoder_ctx *encoder);

void dtvcc_writer_init(dtvcc_writer_ctx *writer,
						   char *base_filename,
						   int program_number,
						   int service_number,
						   enum ccx_output_format write_format,
						   struct encoder_cfg *cfg);
void dtvcc_writer_cleanup(dtvcc_writer_ctx *writer);
void dtvcc_writer_output(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder);

int dtvcc_is_row_empty(dtvcc_tv_screen *tv, int row_index);
int dtvcc_is_screen_empty(dtvcc_tv_screen *tv, struct encoder_ctx *encoder);
void dtvcc_get_write_interval(dtvcc_tv_screen *tv, int row_index, int *first, int *last);
void dtvcc_color_to_hex(int color, unsigned *hR, unsigned *hG, unsigned *hB);
void dtvcc_change_pen_colors(dtvcc_tv_screen *tv, dtvcc_pen_color pen_color, int row_index, int column_index, struct encoder_ctx *encoder, size_t *buf_len, int open);
void dtvcc_change_pen_attribs(dtvcc_tv_screen *tv, dtvcc_pen_attribs pen_attribs, int row_index, int column_index, struct encoder_ctx *encoder, size_t *buf_len, int open);
size_t write_utf16_char(unsigned short utf16_char, char *out);
void dtvcc_write_row(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, int row_index, struct encoder_ctx *encoder, int use_colors);
void dtvcc_write_srt(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder);
void dtvcc_write_debug(dtvcc_tv_screen *tv);
void dtvcc_write_transcript(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder);
void dtvcc_write_sami_header(dtvcc_tv_screen *tv, struct encoder_ctx *encoder);
void dtvcc_write_sami_footer(dtvcc_tv_screen *tv, struct encoder_ctx *encoder);
void dtvcc_write_sami(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder);
void dtvcc_write_scc_header(dtvcc_tv_screen *tv, struct encoder_ctx *encoder);
void dtvcc_write_scc(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder);
void dtvcc_write(dtvcc_writer_ctx *writer, dtvcc_service_decoder *decoder, struct encoder_ctx *encoder);

#endif /*_CCX_DECODERS_708_OUTPUT_H_*/
