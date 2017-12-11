#ifndef CCX_DEMUXER_MXF_H
#define CCX_DEMUXER_MXF_H

#include "ccx_demuxer.h"

int ccx_probe_mxf(struct ccx_demuxer *ctx);
struct MXFContext *ccx_mxf_init(struct ccx_demuxer *demux);
#endif
