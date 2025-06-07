#ifndef CCX_DEMUXER_MXF_H
#define CCX_DEMUXER_MXF_H

#include "ccx_demuxer.h"

typedef uint8_t UID[16];

enum MXFCaptionType
{
    MXF_CT_VBI,
    MXF_CT_ANC,
};

typedef struct
{
    int track_id;
    uint8_t track_number[4];
} MXFTrack;

typedef struct MXFContext
{
    enum MXFCaptionType type;
    int cap_track_id;
    UID cap_essence_key;
    MXFTrack tracks[32];
    int nb_tracks;
    int cap_count;
    struct ccx_rational edit_rate;
} MXFContext;

int ccx_probe_mxf(struct ccx_demuxer* ctx);
struct MXFContext* ccx_mxf_init(struct ccx_demuxer* demux);
#endif
