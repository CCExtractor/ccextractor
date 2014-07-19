#ifndef _XDS_H
#define _XDS_H


void xds_write_transcript_line_suffix (struct ccx_s_write *wb);
void xds_write_transcript_line_prefix (struct ccx_s_write *wb, LLONG start_time, LLONG end_time, int cur_xds_packet_class);

#endif
