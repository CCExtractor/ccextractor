#ifndef CCEXTRACTOR_CCX_DTVCC_H
#define CCEXTRACTOR_CCX_DTVCC_H

#include "ccx_decoders_708.h"
#include "ccx_common_option.h"

void ccx_dtvcc_process_data(struct lib_cc_decode *ctx,
							const unsigned char *data,
							int data_length,
							struct cc_subtitle *sub);

ccx_dtvcc_ctx_t *ccx_dtvcc_init(ccx_decoder_dtvcc_settings_t *opts);
void ccx_dtvcc_free(ccx_dtvcc_ctx_t **);

#endif //CCEXTRACTOR_CCX_DTVCC_H
