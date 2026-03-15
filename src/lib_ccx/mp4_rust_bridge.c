#ifdef ENABLE_FFMPEG_MP4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib_ccx.h"
#include "utility.h"
#include "ccx_encoders_common.h"
#include "ccx_common_option.h"
#include "ccx_dtvcc.h"
#include "activity.h"
#include "avc_functions.h"
#include "ccx_decoders_608.h"
#include "ccx_encoders_mcc.h"
#include "ccx_mp4.h"
#include "mp4_rust_bridge.h"

/* Byte-swap helpers (same as mp4.c) */
static int16_t bridge_bswap16(int16_t v)
{
	return ((v >> 8) & 0x00FF) | ((v << 8) & 0xFF00);
}

static int32_t bridge_bswap32(int32_t v)
{
	return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) |
	       ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24);
}

int ccx_mp4_process_avc_sample(struct lib_ccx_ctx *ctx, unsigned int timescale,
                               unsigned char nal_unit_size,
                               unsigned char *data, unsigned int data_length,
                               long long dts, int cts_offset,
                               struct cc_subtitle *sub)
{
	int status = 0;
	unsigned int i;
	struct lib_cc_decode *dec_ctx = update_decoder_list(ctx);
	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);

	dec_ctx->in_bufferdatatype = CCX_H264;

	set_current_pts(dec_ctx->timing, (dts + cts_offset) * MPEG_CLOCK_FREQ / timescale);
	set_fts(dec_ctx->timing);

	for (i = 0; i < data_length;)
	{
		unsigned int nal_length;

		if (i + nal_unit_size > data_length)
		{
			mprint("Corrupted packet in ccx_mp4_process_avc_sample. dataLength "
			       "%u < index %u + nal_unit_size %u. Ignoring.\n",
			       data_length, i, nal_unit_size);
			return status;
		}
		switch (nal_unit_size)
		{
			case 1:
				nal_length = data[i];
				break;
			case 2:
				nal_length = bridge_bswap16(*(int16_t *)&data[i]);
				break;
			case 4:
				nal_length = bridge_bswap32(*(int32_t *)&data[i]);
				break;
			default:
				mprint("Unexpected nal_unit_size %u\n", nal_unit_size);
				return -1;
		}
		unsigned int previous_index = i;
		i += nal_unit_size;
		if (i + nal_length <= previous_index || i + nal_length > data_length)
		{
			mprint("Corrupted sample in ccx_mp4_process_avc_sample. Ignoring.\n");
			return status;
		}

		temp_debug = 0;
		if (nal_length > 0)
		{
			do_NAL(enc_ctx, dec_ctx, &data[i], nal_length, sub);
		}
		i += nal_length;
	}

	return status;
}

int ccx_mp4_process_hevc_sample(struct lib_ccx_ctx *ctx, unsigned int timescale,
                                unsigned char nal_unit_size,
                                unsigned char *data, unsigned int data_length,
                                long long dts, int cts_offset,
                                struct cc_subtitle *sub)
{
	int status = 0;
	unsigned int i;
	struct lib_cc_decode *dec_ctx = update_decoder_list(ctx);
	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);

	dec_ctx->in_bufferdatatype = CCX_H264;
	dec_ctx->avc_ctx->is_hevc = 1;

	set_current_pts(dec_ctx->timing, (dts + cts_offset) * MPEG_CLOCK_FREQ / timescale);
	set_fts(dec_ctx->timing);

	for (i = 0; i < data_length;)
	{
		unsigned int nal_length;

		if (i + nal_unit_size > data_length)
		{
			mprint("Corrupted packet in ccx_mp4_process_hevc_sample. Ignoring.\n");
			return status;
		}
		switch (nal_unit_size)
		{
			case 1:
				nal_length = data[i];
				break;
			case 2:
				nal_length = bridge_bswap16(*(int16_t *)&data[i]);
				break;
			case 4:
				nal_length = bridge_bswap32(*(int32_t *)&data[i]);
				break;
			default:
				mprint("Unexpected nal_unit_size %u in HEVC\n", nal_unit_size);
				return -1;
		}
		unsigned int previous_index = i;
		i += nal_unit_size;
		if (i + nal_length <= previous_index || i + nal_length > data_length)
		{
			mprint("Corrupted sample in ccx_mp4_process_hevc_sample. Ignoring.\n");
			return status;
		}

		temp_debug = 0;
		if (nal_length > 0)
		{
			do_NAL(enc_ctx, dec_ctx, &data[i], nal_length, sub);
		}
		i += nal_length;
	}

	/* Flush CC data after each HEVC sample */
	if (dec_ctx->avc_ctx->cc_count > 0)
	{
		store_hdcc(enc_ctx, dec_ctx, dec_ctx->avc_ctx->cc_data,
		           dec_ctx->avc_ctx->cc_count,
		           dec_ctx->timing->current_tref,
		           dec_ctx->timing->fts_now, sub);
		dec_ctx->avc_ctx->cc_buffer_saved = CCX_TRUE;
		dec_ctx->avc_ctx->cc_count = 0;
	}

	return status;
}

int ccx_mp4_process_cc_packet(struct lib_ccx_ctx *ctx, int sub_type_c608,
                              unsigned char *data, unsigned int data_length,
                              long long dts, int cts_offset,
                              unsigned int timescale,
                              struct cc_subtitle *sub)
{
	struct lib_cc_decode *dec_ctx = update_decoder_list(ctx);
	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);

	set_current_pts(dec_ctx->timing, (dts + cts_offset) * MPEG_CLOCK_FREQ / timescale);
	dec_ctx->timing->current_picture_coding_type = CCX_FRAME_TYPE_I_FRAME;
	set_fts(dec_ctx->timing);

	if (sub_type_c608)
	{
		/* CEA-608: data is raw cc pairs */
		int ret = 0;
		int len = data_length;
		unsigned char *tdata = data;
		do
		{
			ret = process608(tdata, len > 2 ? 2 : len, dec_ctx, sub);
			len -= ret;
			tdata += ret;
			cb_field1++;
			if (sub->got_output)
			{
				encode_sub(enc_ctx, sub);
				sub->got_output = 0;
			}
		} while (len > 0);
	}
	else
	{
		/* CEA-708: data is CDP packet content */
		unsigned int cc_count;
		unsigned char *cc_data = ccdp_find_data(data, data_length, &cc_count);
		if (!cc_data)
			return 0;

		ctx->dec_global_setting->settings_dtvcc->enabled = 1;

		for (unsigned int cc_i = 0; cc_i < cc_count; cc_i++, cc_data += 3)
		{
			unsigned char cc_info = cc_data[0];
			unsigned char cc_valid = (unsigned char)((cc_info & 4) >> 2);
			unsigned char cc_type = (unsigned char)(cc_info & 3);

			if (cc_info == CDP_SECTION_SVC_INFO || cc_info == CDP_SECTION_FOOTER)
				break;

			if ((cc_info == 0xFA || cc_info == 0xFC || cc_info == 0xFD) &&
			    (cc_data[1] & 0x7F) == 0 && (cc_data[2] & 0x7F) == 0)
				continue;

			if (cc_type < 2)
				continue;

#ifndef DISABLE_RUST
			ccxr_dtvcc_process_data(dec_ctx->dtvcc_rust, cc_valid, cc_type,
			                        cc_data[1], cc_data[2]);
#else
			unsigned char temp[4];
			temp[0] = cc_valid;
			temp[1] = cc_type;
			temp[2] = cc_data[1];
			temp[3] = cc_data[2];
			dtvcc_process_data(dec_ctx->dtvcc, temp);
#endif
			cb_708++;
		}
	}

	return 0;
}

int ccx_mp4_process_tx3g_packet(struct lib_ccx_ctx *ctx,
                                unsigned char *data, unsigned int data_length,
                                long long dts, int cts_offset,
                                unsigned int timescale,
                                struct cc_subtitle *sub)
{
	struct lib_cc_decode *dec_ctx = update_decoder_list(ctx);
	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);

	set_current_pts(dec_ctx->timing, (dts + cts_offset) * MPEG_CLOCK_FREQ / timescale);
	set_fts(dec_ctx->timing);

	/* tx3g format: 16-bit text length + UTF-8 text */
	if (data_length < 2)
		return -1;

	unsigned int text_length = (data[0] << 8) | data[1];
	if (text_length == 0 || text_length > data_length - 2)
		return -1;

	/* Set up subtitle for deferred encoding */
	sub->type = CC_TEXT;
	sub->start_time = dec_ctx->timing->fts_now;
	if (sub->data != NULL)
		free(sub->data);
	sub->data = malloc(text_length + 1);
	if (!sub->data)
		return -1;
	sub->datatype = CC_DATATYPE_GENERIC;
	memcpy(sub->data, data + 2, text_length);
	*((char *)sub->data + text_length) = '\0';

	return 0;
}

void ccx_mp4_flush_tx3g(struct lib_ccx_ctx *ctx, struct cc_subtitle *sub)
{
	struct lib_cc_decode *dec_ctx = update_decoder_list(ctx);
	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);

	if (sub->data != NULL)
	{
		sub->end_time = dec_ctx->timing->fts_now;
		encode_sub(enc_ctx, sub);
	}
}

void ccx_mp4_report_progress(struct lib_ccx_ctx *ctx, unsigned int cur, unsigned int total)
{
	if (total == 0)
		return;
	struct lib_cc_decode *dec_ctx = update_decoder_list(ctx);
	int progress = (int)((cur * 100) / total);
	if (ctx->last_reported_progress != progress)
	{
		int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
		activity_progress(progress, cur_sec / 60, cur_sec % 60);
		ctx->last_reported_progress = progress;
	}
}

#endif /* ENABLE_FFMPEG_MP4 */
