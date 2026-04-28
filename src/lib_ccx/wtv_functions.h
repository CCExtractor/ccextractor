#ifndef WTV_FUNCTIONS_H
#define WTV_FUNCTIONS_H

#include "ccx_demuxer.h"
#include "wtv_constants.h"

int get_sized_buffer(struct ccx_demuxer *ctx, struct wtv_chunked_buffer *cb, uint32_t size);
void skip_sized_buffer(struct ccx_demuxer *ctx, struct wtv_chunked_buffer *cb, uint32_t size);
int read_header(struct ccx_demuxer *ctx, struct wtv_chunked_buffer *cb);

#endif