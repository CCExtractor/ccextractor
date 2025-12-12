#ifndef CCEXTRACTOR_CCX_DTVCC_H
#define CCEXTRACTOR_CCX_DTVCC_H

#include "ccx_decoders_708.h"
#include "ccx_common_option.h"

void dtvcc_process_data(struct dtvcc_ctx *dtvcc,
			const unsigned char *data);

dtvcc_ctx *dtvcc_init(ccx_decoder_dtvcc_settings *opts);
void dtvcc_free(dtvcc_ctx **);

#ifndef DISABLE_RUST
// Rust FFI functions for persistent CEA-708 decoder
extern void *ccxr_dtvcc_init(struct ccx_decoder_dtvcc_settings *settings_dtvcc);
extern void ccxr_dtvcc_free(void *dtvcc_rust);
extern void ccxr_dtvcc_process_data(void *dtvcc_rust, const unsigned char cc_valid,
	const unsigned char cc_type, const unsigned char data1, const unsigned char data2);
extern int ccxr_dtvcc_is_active(void *dtvcc_rust);
#endif

#endif //CCEXTRACTOR_CCX_DTVCC_H
