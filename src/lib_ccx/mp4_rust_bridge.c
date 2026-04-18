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
#include "ccx_decoders_common.h"
#include "ccx_encoders_mcc.h"
#include "ccx_mp4.h"
#include "mp4_rust_bridge.h"

/* Walk a length-prefixed AVCC/HVCC sample, invoking do_NAL() per NAL unit.
 * AVC and HEVC share the iteration; is_hevc only flips the decoder state and
 * whether we flush accumulated CC data at the end of the sample. */
int ccx_mp4_process_nal_sample(struct lib_ccx_ctx *ctx, unsigned int timescale,
			       unsigned char nal_unit_size, int is_hevc,
			       unsigned char *data, unsigned int data_length,
			       long long dts, int cts_offset,
			       struct cc_subtitle *sub)
{
	struct lib_cc_decode *dec_ctx = update_decoder_list(ctx);
	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);

	dec_ctx->in_bufferdatatype = CCX_H264;
	dec_ctx->avc_ctx->is_hevc = is_hevc ? 1 : 0;

	set_current_pts(dec_ctx->timing, (dts + cts_offset) * MPEG_CLOCK_FREQ / timescale);
	set_fts(dec_ctx->timing);

	for (unsigned int i = 0; i < data_length;)
	{
		unsigned int nal_length;

		if (i + nal_unit_size > data_length)
		{
			mprint("Corrupted packet in %s sample: offset %u + nal_unit_size %u > %u. Ignoring.\n",
			       is_hevc ? "HEVC" : "AVC", i, nal_unit_size, data_length);
			return 0;
		}
		switch (nal_unit_size)
		{
			case 1:
				nal_length = data[i];
				break;
			case 2:
				nal_length = RB16(&data[i]);
				break;
			case 4:
				nal_length = RB32(&data[i]);
				break;
			default:
				mprint("Unexpected nal_unit_size %u (%s)\n",
				       nal_unit_size, is_hevc ? "HEVC" : "AVC");
				return -1;
		}
		unsigned int prev = i;
		i += nal_unit_size;
		if (i + nal_length <= prev || i + nal_length > data_length)
		{
			mprint("Corrupted %s sample. Ignoring.\n", is_hevc ? "HEVC" : "AVC");
			return 0;
		}

		temp_debug = 0;
		if (nal_length > 0)
			do_NAL(enc_ctx, dec_ctx, &data[i], nal_length, sub);
		i += nal_length;
	}

	/* HEVC captions arrive as SEI inside the sample; flush the accumulated
	 * cc_data buffer now since there's no slice-header path to do it. */
	if (is_hevc && dec_ctx->avc_ctx->cc_count > 0)
	{
		store_hdcc(enc_ctx, dec_ctx, dec_ctx->avc_ctx->cc_data,
			   dec_ctx->avc_ctx->cc_count,
			   dec_ctx->timing->current_tref,
			   dec_ctx->timing->fts_now, sub);
		dec_ctx->avc_ctx->cc_buffer_saved = CCX_TRUE;
		dec_ctx->avc_ctx->cc_count = 0;
	}

	return 0;
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

	/* ISO BMFF clcp samples are wrapped: [big-endian u32 length][4cc type][payload].
	 * libavformat keeps this wrapper in the AVPacket, so unwrap to match GPAC's
	 * process_clcp(). Fall through to raw-payload handling if the wrapper is absent. */
	unsigned char *payload = data;
	unsigned int payload_len = data_length;
	if (data_length >= 8)
	{
		unsigned int atom_length = ((unsigned int)data[0] << 24) |
					   ((unsigned int)data[1] << 16) |
					   ((unsigned int)data[2] << 8) |
					   (unsigned int)data[3];
		const char *atom_type = (const char *)(data + 4);
		int is_cdat = !memcmp(atom_type, "cdat", 4) || !memcmp(atom_type, "cdt2", 4);
		int is_ccdp = !memcmp(atom_type, "ccdp", 4);
		if (atom_length >= 8 && atom_length <= data_length && (is_cdat || is_ccdp))
		{
			payload = data + 8;
			payload_len = atom_length - 8;
		}
	}

	/* Two in-the-wild payload shapes for c608/c708 samples:
	 *   1) Raw CEA-608 pairs — the classic GPAC-extracted payload inside a
	 *      cdat/cdt2 atom. Byte values sit in the 0x00-0x7F + parity range.
	 *   2) cc_data triplets — [cc_info][b1][b2] where cc_info has
	 *      reserved=11111 in the top 5 bits (value 0xF8-0xFF). Modern
	 *      libavformat delivers many c608 tracks in this shape.
	 * Distinguish by looking at the first byte and the length. */
	int looks_like_cc_data = (payload_len >= 3 && payload_len % 3 == 0 &&
				  (payload[0] & 0xF8) == 0xF8);

	if (looks_like_cc_data && sub_type_c608)
	{
		/* c608 track with cc_data triplets: skip the cc_info byte and feed each
		 * valid field-1/field-2 pair into process608 directly, matching GPAC's
		 * process_clcp() -> process608 path. do_cb is avoided here because its
		 * CCX_H264 guard (set by interleaved H.264 packets earlier in the stream)
		 * suppresses the cb_field increments process608 relies on for
		 * caption-boundary timing. */
		for (unsigned int i = 0; i + 3 <= payload_len; i += 3)
		{
			unsigned char cc_info = payload[i];
			unsigned char cc_valid = (cc_info & 0x04) >> 2;
			unsigned char cc_type = cc_info & 0x03;
			if (!cc_valid || cc_type > 1)
				continue;
			/* process608 picks the field-1 or field-2 decoder context from
			 * dec_ctx->current_field; set it before each pair so both fields
			 * are routed correctly when they arrive interleaved. */
			dec_ctx->current_field = (cc_type == 0) ? 1 : 2;
			unsigned char pair[2] = {payload[i + 1], payload[i + 2]};
			int ret = process608(pair, 2, dec_ctx, sub);
			if (ret <= 0)
				continue;
			if (cc_type == 0)
				cb_field1++;
			else
				cb_field2++;
			if (sub->got_output)
			{
				encode_sub(enc_ctx, sub);
				sub->got_output = 0;
			}
		}
	}
	else if (looks_like_cc_data)
	{
		ctx->dec_global_setting->settings_dtvcc->enabled = 1;
		unsigned int cc_count = payload_len / 3;
		process_cc_data(enc_ctx, dec_ctx, payload, (int)cc_count, sub);
		if (sub->got_output)
		{
			encode_sub(enc_ctx, sub);
			sub->got_output = 0;
		}
	}
	else if (sub_type_c608)
	{
		/* Classic c608: payload is raw CEA-608 pairs */
		int ret = 0;
		int len = (int)payload_len;
		unsigned char *tdata = payload;
		while (len > 0)
		{
			ret = process608(tdata, len > 2 ? 2 : len, dec_ctx, sub);
			if (ret <= 0)
				break;
			len -= ret;
			tdata += ret;
			cb_field1++;
			if (sub->got_output)
			{
				encode_sub(enc_ctx, sub);
				sub->got_output = 0;
			}
		}
	}
	else
	{
		/* Classic c708: payload is a CDP packet */
		unsigned int cc_count;
		unsigned char *cc_data = ccdp_find_data(payload, payload_len, &cc_count);
		if (!cc_data)
			return 0;

		ctx->dec_global_setting->settings_dtvcc->enabled = 1;
		process_cc_data(enc_ctx, dec_ctx, cc_data, (int)cc_count, sub);
		if (sub->got_output)
		{
			encode_sub(enc_ctx, sub);
			sub->got_output = 0;
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
