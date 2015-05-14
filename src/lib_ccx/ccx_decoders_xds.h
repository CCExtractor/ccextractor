#ifndef _XDS_H
#define _XDS_H

#include "ccx_decoders_common.h"
#include "ccx_encoders_common.h"

void process_xds_bytes(const unsigned char hi, int lo);
void do_end_of_xds(struct cc_subtitle *sub, unsigned char expected_checksum);

void ccx_decoders_xds_init_library(ccx_encoders_transcript_format *transcriptSettings, LLONG subs_delay, char millis_separator);

void xds_write_transcript_line_suffix (struct ccx_s_write *wb);
void xds_write_transcript_line_prefix (struct ccx_s_write *wb, LLONG start_time, LLONG end_time, int cur_xds_packet_class);

void xds_cea608_test();
#endif
