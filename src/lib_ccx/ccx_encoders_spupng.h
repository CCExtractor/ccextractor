#ifndef __608_SPUPNG_H__
#define __608_SPUPNG_H__

#include "lib_ccx.h"
#include "spupng_encoder.h"
#include "ccx_encoders_common.h"

int write_cc_buffer_as_spupng(struct eia608_screen *data,struct encoder_ctx *context);

#endif /* __608_SPUPNG_H__ */
