#ifndef CCX_GXF
#define CCX_GXF

#include "ccx_demuxer.h"
#include "lib_ccx.h"

#define STR_LEN 256u

struct ccx_gxf
{
    int nb_streams;

    /* Name of Media File  */
    char media_name[STR_LEN];

    /**
     *  The first field number shall represent the position on a playout
     *  time line of the first recorded field on a track
     */
    int32_t first_field_nb;

    /**
     * The last field number shall represent the position on a playout
     *  time line of the last recorded field plus one.
     */
    int32_t last_field_nb;

    /**
     * The mark in field number shall represent the position on a playout
     *  time line of the first field to be played from a track.
     */
    int32_t mark_in;

    /**
     * The mark out field number shall represent the position on a playout
     * time line of the last field to be played plus one
     */
    int32_t mark_out;

    /**
     * Estimated size in kb for bytes multiply by 1024
     */
    int32_t stream_size;

    struct ccx_gxf_ancillary_data_track *ad_track;

    struct ccx_gxf_video_track *vid_track;

    /**
     * cdp data buffer
     */
    unsigned char *cdp;
    size_t cdp_len;
};

int ccx_gxf_probe(unsigned char *buf, int len);
int ccx_gxf_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **data);
struct ccx_gxf *ccx_gxf_init(struct ccx_demuxer *arg);
#endif
