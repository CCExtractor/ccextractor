#ifndef ISDB_H
#define ISDB_H
#include "ccx_common_platform.h"
#include "ccx_common_structs.h"
#include "ccx_decoders_structs.h"

int isdb_set_global_time(struct lib_cc_decode *dec_ctx, uint64_t timestamp);
int isdbsub_decode(struct lib_cc_decode *dec_ctx, const uint8_t *buf, size_t buf_size, struct cc_subtitle *sub);
void delete_isdb_decoder(void **isdb_ctx);
void *init_isdb_decoder(void);

#ifndef DISABLE_RUST
extern void *ccxr_init_isdb_decoder(void);
extern void ccxr_delete_isdb_decoder(void **ctx);
extern int ccxr_isdb_set_global_time(void *ctx, uint64_t timestamp);
extern int ccxr_isdbsub_decode(struct lib_cc_decode *dec_ctx, const uint8_t *buf, size_t buf_size, struct cc_subtitle *sub);
#endif

#endif
