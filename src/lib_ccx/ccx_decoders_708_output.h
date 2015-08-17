#ifndef _CCX_DECODERS_708_OUTPUT_H_
#define _CCX_DECODERS_708_OUTPUT_H_

#include "ccx_decoders_708.h"

void ccx_dtvcc_write_srt(dtvcc_service_decoder *decoder);
void ccx_dtvcc_write_debug(dtvcc_service_decoder *decoder);
void ccx_dtvcc_write_transcript(dtvcc_service_decoder *decoder);
void ccx_dtvcc_write_sami(dtvcc_service_decoder *decoder);

void ccx_dtvcc_write(dtvcc_service_decoder *decoder);
void ccx_dtvcc_write_done(dtvcc_service_decoder *decoder);

#endif /*_CCX_DECODERS_708_OUTPUT_H_*/