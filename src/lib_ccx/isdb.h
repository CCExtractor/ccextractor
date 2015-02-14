#ifndef ISDB_H
#define ISDB_H
#include "ccx_common_platform.h"
#include "ccx_common_structs.h"


int isdb_set_global_time(void *codec_ctx, uint64_t  timestamp);
int isdbsub_decode(void *codec_ctx, const unsigned char *buf, int buf_size, struct cc_subtitle *sub);
void delete_isdb_caption(void *isdb_ctx);
void *init_isdb_caption(void);


#endif
