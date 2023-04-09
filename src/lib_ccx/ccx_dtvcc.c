#include "ccx_dtvcc.h"
#include "ccx_common_common.h"
#include "ccx_encoders_common.h"
#include "ccx_decoders_708_output.h"

void dtvcc_process_data(struct dtvcc_ctx *dtvcc,
			const unsigned char *data)
{
	/*
	 * Note: the data has following format:
	 * 1 byte for cc_valid
	 * 1 byte for cc_type
	 * 2 bytes for the actual data
	 */

	if (!dtvcc->is_active && !dtvcc->report_enabled)
		return;

	unsigned char cc_valid = data[0];
	unsigned char cc_type = data[1];

	switch (cc_type)
	{
		case 2:
			ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_data: DTVCC Channel Packet Data\n");
			if (cc_valid && dtvcc->is_current_packet_header_parsed)
			{
				if (dtvcc->current_packet_length > 253)
				{
					ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_data: "
										  "Warning: Legal packet size exceeded (1), data not added.\n");
				}
				else
				{
					dtvcc->current_packet[dtvcc->current_packet_length++] = data[2];
					dtvcc->current_packet[dtvcc->current_packet_length++] = data[3];
					int len = dtvcc->current_packet[0] & 0x3F; // 6 least significants bits

					if (len == 0) // This is well defined in EIA-708; no magic.
						len = 128;
					else
						len = len * 2;
					// Note that len here is the length including the header

					if (dtvcc->current_packet_length >= len)
						dtvcc_process_current_packet(dtvcc, len);
				}
			}
			break;
		case 3:
			ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_data: DTVCC Channel Packet Start\n");
			if (cc_valid)
			{
				if (dtvcc->current_packet_length > CCX_DTVCC_MAX_PACKET_LENGTH - 1)
				{
					ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_data: "
										  "Warning: Legal packet size exceeded (2), data not added.\n");
				}
				else
				{
					if (dtvcc->is_current_packet_header_parsed)
					{
						ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_data: "
											  "Warning: Incorrect packet length specified. Packet will be skipped.\n");
						dtvcc_clear_packet(dtvcc);
					}
					dtvcc->current_packet[dtvcc->current_packet_length++] = data[2];
					dtvcc->current_packet[dtvcc->current_packet_length++] = data[3];
					dtvcc->is_current_packet_header_parsed = 1;
				}
			}
			break;
		default:
			ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_BUG_BUG, "[CEA-708] dtvcc_process_data: "
									      "shouldn't be here - cc_type: %d\n",
						     cc_type);
	}
}

//--------------------------------------------------------------------------------------

dtvcc_ctx *dtvcc_init(struct ccx_decoder_dtvcc_settings *opts)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] initializing dtvcc decoder\n");
	dtvcc_ctx *ctx = (dtvcc_ctx *)malloc(sizeof(dtvcc_ctx));
	if (!ctx)
	{
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "[CEA-708] dtvcc_init");
		return NULL;
	}

	ctx->report = opts->report;
	ctx->report->reset_count = 0;
	ctx->is_active = 0;
	ctx->report_enabled = 0;
	ctx->no_rollup = opts->no_rollup;
	ctx->active_services_count = opts->active_services_count;

	memcpy(ctx->services_active, opts->services_enabled, CCX_DTVCC_MAX_SERVICES * sizeof(int));

	dtvcc_clear_packet(ctx);

	ctx->last_sequence = CCX_DTVCC_NO_LAST_SEQUENCE;

	ctx->report_enabled = opts->print_file_reports;
	ctx->timing = opts->timing;

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] initializing services\n");

	for (int i = 0; i < CCX_DTVCC_MAX_SERVICES; i++)
	{
		if (!ctx->services_active[i])
			continue;

		dtvcc_service_decoder *decoder = &ctx->decoders[i];
		decoder->cc_count = 0;
		decoder->tv = (dtvcc_tv_screen *)malloc(sizeof(dtvcc_tv_screen));
		decoder->tv->service_number = i + 1;
		decoder->tv->cc_count = 0;
		if (!decoder->tv)
			ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "dtvcc_init");

		for (int j = 0; j < CCX_DTVCC_MAX_WINDOWS; j++)
			decoder->windows[j].memory_reserved = 0;

		dtvcc_windows_reset(decoder);
	}

	return ctx;
}

void dtvcc_free(dtvcc_ctx **ctx_ptr)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_free: cleaning up\n");

	dtvcc_ctx *ctx = *ctx_ptr;

	for (int i = 0; i < CCX_DTVCC_MAX_SERVICES; i++)
	{
		if (!ctx->services_active[i])
			continue;

		dtvcc_service_decoder *decoder = &ctx->decoders[i];

		for (int j = 0; j < CCX_DTVCC_MAX_WINDOWS; j++)
			if (decoder->windows[j].memory_reserved)
			{
				for (int k = 0; k < CCX_DTVCC_MAX_ROWS; k++)
					free(decoder->windows[j].rows[k]);
				decoder->windows[j].memory_reserved = 0;
			}

		free(decoder->tv);
	}
	freep(ctx_ptr);
}
