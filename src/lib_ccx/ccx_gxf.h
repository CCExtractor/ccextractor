#ifndef CCX_GXF
#define CCX_GXF

#include "ccx_demuxer.h"

int ccx_gxf_probe(unsigned char *buf, int len);
int ccx_gxf_get_more_data(struct ccx_demuxer *ctx, struct demuxer_data **data);
struct ccx_gxf *ccx_gxf_init(struct ccx_demuxer *arg);
#endif
