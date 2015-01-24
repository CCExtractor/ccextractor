#ifndef ISDB_H
#define ISDB_H
#include "ccx_common_platform.h"
#include "ccx_common_structs.h"

int isdb_parse_data_group(void *codec_ctx, uint8_t *buf, struct cc_subtitle *sub);

void delete_isdb_caption(void *isdb_ctx);
void *init_isdb_caption(void);


#endif
