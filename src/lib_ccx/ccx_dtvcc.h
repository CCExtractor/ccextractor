#ifndef CCEXTRACTOR_CCX_DTVCC_H
#define CCEXTRACTOR_CCX_DTVCC_H

#include "ccx_decoders_708.h"
#include "ccx_common_option.h"

void dtvcc_process_data(struct lib_cc_decode *ctx,
							const unsigned char *data,
							int data_length);

dtvcc_ctx *dtvcc_init(ccx_decoder_dtvcc_settings *opts);
void dtvcc_free(dtvcc_ctx **);

#endif //CCEXTRACTOR_CCX_DTVCC_H
