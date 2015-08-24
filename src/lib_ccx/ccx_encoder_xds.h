#ifndef _CCX_ENCODER_XDS_H
#define _CCX_ENCODER_XDS_H

void xds_write_transcript_line_prefix (struct encoder_ctx *context, struct ccx_s_write *wb, LLONG start_time, LLONG end_time, int cur_xds_packet_class);
#endif
