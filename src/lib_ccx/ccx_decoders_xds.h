#ifndef CCX_DECODER_XDS_H
#define CCX_DECODER_XDS_H

#include "ccx_decoders_common.h"

#define NUM_BYTES_PER_PACKET 35 // Class + type (repeated for convenience) + data + zero
#define NUM_XDS_BUFFERS 9  // CEA recommends no more than one level of interleaving. Play it safe

struct ccx_decoders_xds_context;
void process_xds_bytes (struct ccx_decoders_xds_context *ctx, const unsigned char hi, int lo);
void do_end_of_xds (struct cc_subtitle *sub, struct ccx_decoders_xds_context *ctx, unsigned char expected_checksum);

struct ccx_decoders_xds_context *ccx_decoders_xds_init_library(struct ccx_common_timing_ctx *timing, int xds_write_to_file);

void xds_cea608_test();

struct xds_buffer
{
	unsigned in_use;
	int xds_class;
	int xds_type;
	unsigned char bytes[NUM_BYTES_PER_PACKET]; // Class + type (repeated for convenience) + data + zero
	unsigned char used_bytes;
};

typedef struct ccx_decoders_xds_context
{
	// Program Identification Number (Start Time) for current program
	int current_xds_min;
	int current_xds_hour;
	int current_xds_date;
	int current_xds_month;
	int current_program_type_reported; // No.
	int xds_start_time_shown;
	int xds_program_length_shown;
	char xds_program_description[8][33];

	char current_xds_network_name[33];
	char current_xds_program_name[33];
	char current_xds_call_letters[7];
	char current_xds_program_type[33];

	struct xds_buffer xds_buffers[NUM_XDS_BUFFERS];
	int cur_xds_buffer_idx;
	int cur_xds_packet_class;
	unsigned char *cur_xds_payload;
	int cur_xds_payload_length;
	int cur_xds_packet_type;
	struct ccx_common_timing_ctx *timing;

	unsigned current_ar_start;
	unsigned current_ar_end;

	int xds_write_to_file; // Set to 1 if XDS data is to be written to file

} ccx_decoders_xds_context_t;

#endif
