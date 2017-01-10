#ifndef _CCX_DECODERS_708_OUTPUT_H_
#define _CCX_DECODERS_708_OUTPUT_H_

#include "ccx_decoders_708.h"
#include "ccx_encoders_common.h"
#include "ccx_common_option.h"

void ccx_dtvcc_write_done(dtvcc_tv_screen *tv, struct encoder_ctx *encoder);

void ccx_dtvcc_writer_init(ccx_dtvcc_writer_ctx *writer,
						   char *base_filename,
						   int program_number,
						   int service_number,
						   enum ccx_output_format write_format,
						   struct encoder_cfg *cfg);
void ccx_dtvcc_writer_cleanup(ccx_dtvcc_writer_ctx *writer);
void ccx_dtvcc_writer_output(ccx_dtvcc_writer_ctx *writer, ccx_dtvcc_service_decoder *decoder, struct encoder_ctx *encoder);

#endif /*_CCX_DECODERS_708_OUTPUT_H_*/