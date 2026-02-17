#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <gpac/isomedia.h>
#include <gpac/mpeg4_odf.h>
#include "lib_ccx.h"
#include "utility.h"
#include "ccx_encoders_common.h"
#include "ccx_encoders_mcc.h"
#include "ccx_common_option.h"
#include "ccx_mp4.h"
#include "activity.h"
#include "ccx_dtvcc.h"
#include "vobsub_decoder.h"

#define MEDIA_TYPE(type, subtype) (((u64)(type) << 32) + (subtype))

#define GF_ISOM_SUBTYPE_C708 GF_4CC('c', '7', '0', '8')

// HEVC subtypes (hev1, hvc1)
#ifndef GF_ISOM_SUBTYPE_HEV1
#define GF_ISOM_SUBTYPE_HEV1 GF_4CC('h', 'e', 'v', '1')
#endif
#ifndef GF_ISOM_SUBTYPE_HVC1
#define GF_ISOM_SUBTYPE_HVC1 GF_4CC('h', 'v', 'c', '1')
#endif

// VOBSUB subtype (mp4s or MPEG)
#ifndef GF_ISOM_SUBTYPE_MPEG4
#define GF_ISOM_SUBTYPE_MPEG4 GF_4CC('M', 'P', 'E', 'G')
#endif

static int16_t bswap16(int16_t v)
{
	return ((v >> 8) & 0x00FF) | ((v << 8) & 0xFF00);
}

static int32_t bswap32(int32_t v)
{
	// For 0x12345678 returns 78563412
	// Use int32_t instead of long for consistent behavior across platforms
	// (long is 4 bytes on Windows x64 but 8 bytes on Linux x64)
	int32_t swapped = ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24);
	return swapped;
}
static struct
{
	unsigned total;
	unsigned type[32];
} s_nalu_stats;

static int process_avc_sample(struct lib_ccx_ctx *ctx, u32 timescale, GF_AVCConfig *c, GF_ISOSample *s, struct cc_subtitle *sub)
{
	int status = 0;
	u32 i;
	s32 signed_cts = (s32)s->CTS_Offset; // Convert from unsigned to signed. GPAC uses u32 but unsigned values are legal.
	struct lib_cc_decode *dec_ctx = NULL;
	struct encoder_ctx *enc_ctx = NULL;

	dec_ctx = update_decoder_list(ctx);
	enc_ctx = update_encoder_list(ctx);

	set_current_pts(dec_ctx->timing, (s->DTS + signed_cts) * MPEG_CLOCK_FREQ / timescale);
	set_fts(dec_ctx->timing);

	for (i = 0; i < s->dataLength;)
	{
		u32 nal_length;

		if (i + c->nal_unit_size > s->dataLength)
		{
			mprint("Corrupted packet detected in process_avc_sample. dataLength "
			       "%u is less than index %u + nal_unit_size %u. Ignoring.\n",
			       s->dataLength, i, c->nal_unit_size);
			// The packet is likely corrupted, it's unsafe to read this many bytes
			// even to detect the length of the next `nal`. Ignoring this error,
			// hopefully the outer loop in `process_avc_track` can recover.
			return status;
		}
		switch (c->nal_unit_size)
		{
			case 1:
				nal_length = s->data[i];
				break;
			case 2:
				nal_length = bswap16(*(int16_t *)&s->data[i]);
				break;
			case 4:
				nal_length = bswap32(*(int32_t *)&s->data[i]);
				break;
		}
		const u32 previous_index = i;
		i += c->nal_unit_size;
		if (i + nal_length <= previous_index || i + nal_length > s->dataLength)
		{
			mprint("Corrupted sample detected in process_avc_sample. dataLength %u "
			       "is less than index %u + nal_unit_size %u + nal_length %u. Ignoring.\n",
			       s->dataLength, previous_index, c->nal_unit_size, nal_length);
			// The packet is likely corrupted, it's unsafe to procell nal_length bytes
			// because they are past the sample end. Ignoring this error, hopefully
			// the outer loop in `process_avc_track` can recover.
			return status;
		}

		s_nalu_stats.total += 1;
		temp_debug = 0;

		if (nal_length > 0)
		{
			// s->data[i] is only relevant and safe to access here.
			s_nalu_stats.type[s->data[i] & 0x1F] += 1;
			do_NAL(enc_ctx, dec_ctx, (unsigned char *)&(s->data[i]), nal_length, sub);
		}
		i += nal_length;
	} // outer for
	assert(i == s->dataLength);

	return status;
}

static int process_hevc_sample(struct lib_ccx_ctx *ctx, u32 timescale, GF_HEVCConfig *c, GF_ISOSample *s, struct cc_subtitle *sub)
{
	int status = 0;
	u32 i;
	s32 signed_cts = (s32)s->CTS_Offset;
	struct lib_cc_decode *dec_ctx = NULL;
	struct encoder_ctx *enc_ctx = NULL;

	dec_ctx = update_decoder_list(ctx);
	enc_ctx = update_encoder_list(ctx);

	// Enable HEVC mode for NAL parsing
	dec_ctx->avc_ctx->is_hevc = 1;

	set_current_pts(dec_ctx->timing, (s->DTS + signed_cts) * MPEG_CLOCK_FREQ / timescale);
	set_fts(dec_ctx->timing);

	for (i = 0; i < s->dataLength;)
	{
		u32 nal_length;

		if (i + c->nal_unit_size > s->dataLength)
		{
			mprint("Corrupted packet detected in process_hevc_sample. dataLength "
			       "%u is less than index %u + nal_unit_size %u. Ignoring.\n",
			       s->dataLength, i, c->nal_unit_size);
			return status;
		}
		switch (c->nal_unit_size)
		{
			case 1:
				nal_length = s->data[i];
				break;
			case 2:
				nal_length = bswap16(*(int16_t *)&s->data[i]);
				break;
			case 4:
				nal_length = bswap32(*(int32_t *)&s->data[i]);
				break;
			default:
				mprint("Unexpected nal_unit_size %u in HEVC config\n", c->nal_unit_size);
				return status;
		}
		const u32 previous_index = i;
		i += c->nal_unit_size;
		if (i + nal_length <= previous_index || i + nal_length > s->dataLength)
		{
			mprint("Corrupted sample detected in process_hevc_sample. dataLength %u "
			       "is less than index %u + nal_unit_size %u + nal_length %u. Ignoring.\n",
			       s->dataLength, previous_index, c->nal_unit_size, nal_length);
			return status;
		}

		s_nalu_stats.total += 1;
		temp_debug = 0;

		if (nal_length > 0)
		{
			// For HEVC, NAL type is in bits [6:1] of byte 0
			u8 nal_type = (s->data[i] >> 1) & 0x3F;
			if (nal_type < 32)
				s_nalu_stats.type[nal_type] += 1;
			do_NAL(enc_ctx, dec_ctx, (unsigned char *)&(s->data[i]), nal_length, sub);
		}
		i += nal_length;
	}
	assert(i == s->dataLength);

	// For HEVC, we need to flush CC data after each sample (unlike H.264 which does this in slice_header)
	// This is because HEVC SEI messages contain the CC data and we don't parse slice headers
	if (dec_ctx->avc_ctx->cc_count > 0)
	{
		// Store the CC data for processing
		store_hdcc(enc_ctx, dec_ctx, dec_ctx->avc_ctx->cc_data, dec_ctx->avc_ctx->cc_count,
			   dec_ctx->timing->current_tref, dec_ctx->timing->fts_now, sub);
		dec_ctx->avc_ctx->cc_buffer_saved = CCX_TRUE;
		dec_ctx->avc_ctx->cc_count = 0;
	}

	return status;
}
static int process_xdvb_track(struct lib_ccx_ctx *ctx, const char *basename, GF_ISOFile *f, u32 track, struct cc_subtitle *sub)
{
	u32 timescale, i, sample_count;
	int status;

	struct lib_cc_decode *dec_ctx = NULL;
	struct encoder_ctx *enc_ctx = NULL;

	dec_ctx = update_decoder_list(ctx);
	enc_ctx = update_encoder_list(ctx);

	// Set buffer data type to CCX_PES for MP4/MOV MPEG-2 tracks.
	// This ensures cb_field counters are not incremented in do_cb(),
	// which is correct because container formats associate captions
	// with the frame's PTS directly.
	dec_ctx->in_bufferdatatype = CCX_PES;

	if ((sample_count = gf_isom_get_sample_count(f, track)) < 1)
	{
		return 0;
	}

	timescale = gf_isom_get_media_timescale(f, track);

	status = 0;

	for (i = 0; i < sample_count; i++)
	{
		u32 sdi;

		GF_ISOSample *s = gf_isom_get_sample(f, track, i + 1, &sdi);
		if (s != NULL)
		{
			s32 signed_cts = (s32)s->CTS_Offset; // Convert from unsigned to signed. GPAC uses u32 but unsigned values are legal.
			set_current_pts(dec_ctx->timing, (s->DTS + signed_cts) * MPEG_CLOCK_FREQ / timescale);
			set_fts(dec_ctx->timing);

			process_m2v(enc_ctx, dec_ctx, (unsigned char *)s->data, s->dataLength, sub);
			gf_isom_sample_del(&s);
		}

		int progress = (int)((i * 100) / sample_count);
		if (ctx->last_reported_progress != progress)
		{
			int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
			activity_progress(progress, cur_sec / 60, cur_sec % 60);
			ctx->last_reported_progress = progress;
		}
	}
	int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
	activity_progress(100, cur_sec / 60, cur_sec % 60);

	return status;
}

static int process_avc_track(struct lib_ccx_ctx *ctx, const char *basename, GF_ISOFile *f, u32 track, struct cc_subtitle *sub)
{
	u32 timescale, i, sample_count, last_sdi = 0;
	int status;
	GF_AVCConfig *c = NULL;
	struct lib_cc_decode *dec_ctx = NULL;

	dec_ctx = update_decoder_list(ctx);

	// Set buffer data type to CCX_H264 for MP4/MOV AVC tracks.
	// This ensures cb_field counters are not incremented in do_cb(),
	// which is correct because container formats associate captions
	// with the frame's PTS directly.
	dec_ctx->in_bufferdatatype = CCX_H264;

	if ((sample_count = gf_isom_get_sample_count(f, track)) < 1)
	{
		return 0;
	}

	timescale = gf_isom_get_media_timescale(f, track);

	status = 0;

	for (i = 0; i < sample_count; i++)
	{
		u32 sdi;

		GF_ISOSample *s = gf_isom_get_sample(f, track, i + 1, &sdi);

		if (s != NULL)
		{
			if (sdi != last_sdi)
			{
				if (c != NULL)
				{
					gf_odf_avc_cfg_del(c);
					c = NULL;
				}

				if ((c = gf_isom_avc_config_get(f, track, sdi)) == NULL)
				{
					gf_isom_sample_del(&s);
					status = -1;
					break;
				}

				last_sdi = sdi;
			}

			status = process_avc_sample(ctx, timescale, c, s, sub);

			gf_isom_sample_del(&s);

			if (status != 0)
			{
				break;
			}
		}

		int progress = (int)((i * 100) / sample_count);
		if (ctx->last_reported_progress != progress)
		{
			int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
			activity_progress(progress, cur_sec / 60, cur_sec % 60);
			ctx->last_reported_progress = progress;
		}
	}
	int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
	activity_progress(100, cur_sec / 60, cur_sec % 60);

	if (c != NULL)
	{
		gf_odf_avc_cfg_del(c);
		c = NULL;
	}

	return status;
}

static int process_hevc_track(struct lib_ccx_ctx *ctx, const char *basename, GF_ISOFile *f, u32 track, struct cc_subtitle *sub)
{
	u32 timescale, i, sample_count, last_sdi = 0;
	int status;
	GF_HEVCConfig *c = NULL;
	struct lib_cc_decode *dec_ctx = NULL;

	dec_ctx = update_decoder_list(ctx);

	// Enable HEVC mode
	dec_ctx->avc_ctx->is_hevc = 1;

	// Set buffer data type to CCX_H264 for MP4/MOV HEVC tracks.
	// This ensures cb_field counters are not incremented in do_cb(),
	// which is correct because container formats associate captions
	// with the frame's PTS directly.
	dec_ctx->in_bufferdatatype = CCX_H264;

	if ((sample_count = gf_isom_get_sample_count(f, track)) < 1)
	{
		return 0;
	}

	timescale = gf_isom_get_media_timescale(f, track);

	status = 0;

	for (i = 0; i < sample_count; i++)
	{
		u32 sdi;

		GF_ISOSample *s = gf_isom_get_sample(f, track, i + 1, &sdi);

		if (s != NULL)
		{
			if (sdi != last_sdi)
			{
				if (c != NULL)
				{
					gf_odf_hevc_cfg_del(c);
					c = NULL;
				}

				if ((c = gf_isom_hevc_config_get(f, track, sdi)) == NULL)
				{
					gf_isom_sample_del(&s);
					status = -1;
					break;
				}

				last_sdi = sdi;
			}

			status = process_hevc_sample(ctx, timescale, c, s, sub);

			gf_isom_sample_del(&s);

			if (status != 0)
			{
				break;
			}
		}

		int progress = (int)((i * 100) / sample_count);
		if (ctx->last_reported_progress != progress)
		{
			int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
			activity_progress(progress, cur_sec / 60, cur_sec % 60);
			ctx->last_reported_progress = progress;
		}
	}
	int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
	activity_progress(100, cur_sec / 60, cur_sec % 60);

	if (c != NULL)
	{
		gf_odf_hevc_cfg_del(c);
		c = NULL;
	}

	return status;
}

static int process_vobsub_track(struct lib_ccx_ctx *ctx, GF_ISOFile *f, u32 track, struct cc_subtitle *sub)
{
	u32 timescale, i, sample_count;
	int status = 0;
	struct lib_cc_decode *dec_ctx = NULL;
	struct encoder_ctx *enc_ctx = NULL;
	struct vobsub_ctx *vob_ctx = NULL;

	dec_ctx = update_decoder_list(ctx);
	enc_ctx = update_encoder_list(ctx);

	if ((sample_count = gf_isom_get_sample_count(f, track)) < 1)
	{
		return 0;
	}

	timescale = gf_isom_get_media_timescale(f, track);

	/* Check if OCR is available */
	if (!vobsub_ocr_available())
	{
		fatal(EXIT_NOT_CLASSIFIED,
		      "VOBSUB to text conversion requires OCR support.\n"
		      "Please rebuild CCExtractor with -DWITH_OCR=ON");
	}

	/* Initialize VOBSUB decoder */
	vob_ctx = init_vobsub_decoder();
	if (!vob_ctx)
	{
		fatal(EXIT_NOT_CLASSIFIED,
		      "VOBSUB decoder initialization failed.\n"
		      "Please ensure Tesseract is installed with language data.");
	}

	/* Try to get decoder config for palette info */
	GF_GenericSampleDescription *gdesc = gf_isom_get_generic_sample_description(f, track, 1);
	if (gdesc && gdesc->extension_buf && gdesc->extension_buf_size > 0)
	{
		/* The extension buffer may contain an idx-like header with palette */
		char *header = malloc(gdesc->extension_buf_size + 1);
		if (header)
		{
			memcpy(header, gdesc->extension_buf, gdesc->extension_buf_size);
			header[gdesc->extension_buf_size] = '\0';
			vobsub_parse_palette(vob_ctx, header);
			free(header);
		}
	}
	if (gdesc)
		free(gdesc);

	mprint("Processing VOBSUB track (%u samples)\n", sample_count);

	for (i = 0; i < sample_count; i++)
	{
		u32 sdi;
		GF_ISOSample *s = gf_isom_get_sample(f, track, i + 1, &sdi);

		if (s != NULL)
		{
			s32 signed_cts = (s32)s->CTS_Offset;
			LLONG start_time_ms = (LLONG)((s->DTS + signed_cts) * 1000) / timescale;

			/* Calculate end time from next sample if available */
			LLONG end_time_ms = 0;
			if (i + 1 < sample_count)
			{
				u32 next_sdi;
				GF_ISOSample *next_s = gf_isom_get_sample(f, track, i + 2, &next_sdi);
				if (next_s)
				{
					s32 next_signed_cts = (s32)next_s->CTS_Offset;
					end_time_ms = (LLONG)((next_s->DTS + next_signed_cts) * 1000) / timescale;
					gf_isom_sample_del(&next_s);
				}
			}
			if (end_time_ms == 0)
				end_time_ms = start_time_ms + 5000; /* Default 5 second duration */

			set_current_pts(dec_ctx->timing, (s->DTS + signed_cts) * MPEG_CLOCK_FREQ / timescale);
			set_fts(dec_ctx->timing);

			/* Decode SPU and run OCR */
			struct cc_subtitle vob_sub;
			memset(&vob_sub, 0, sizeof(vob_sub));

			int ret = vobsub_decode_spu(vob_ctx,
						    (unsigned char *)s->data, s->dataLength,
						    start_time_ms, end_time_ms,
						    &vob_sub);

			if (ret == 0 && vob_sub.got_output)
			{
				/* Encode the subtitle to output format */
				encode_sub(enc_ctx, &vob_sub);
				sub->got_output = 1;

				/* Free subtitle data */
				if (vob_sub.data)
				{
					struct cc_bitmap *rect = (struct cc_bitmap *)vob_sub.data;
					for (int j = 0; j < vob_sub.nb_data; j++)
					{
						if (rect[j].data0)
							free(rect[j].data0);
						if (rect[j].data1)
							free(rect[j].data1);
#ifdef ENABLE_OCR
						if (rect[j].ocr_text)
							free(rect[j].ocr_text);
#endif
					}
					free(vob_sub.data);
				}
			}

			gf_isom_sample_del(&s);
		}

		int progress = (int)((i * 100) / sample_count);
		if (ctx->last_reported_progress != progress)
		{
			int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
			activity_progress(progress, cur_sec / 60, cur_sec % 60);
			ctx->last_reported_progress = progress;
		}
	}

	int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
	activity_progress(100, cur_sec / 60, cur_sec % 60);

	delete_vobsub_decoder(&vob_ctx);
	mprint("VOBSUB processing complete\n");

	return status;
}

static char *format_duration(u64 dur, u32 timescale, char *szDur, size_t szDur_size)
{
	u32 h, m, s, ms;
	if ((dur == (u64)-1) || (dur == (u32)-1))
	{
		snprintf(szDur, szDur_size, "Unknown");
		return szDur;
	}
	dur = (u64)((((Double)(s64)dur) / timescale) * 1000);
	h = (u32)(dur / 3600000);
	m = (u32)(dur / 60000) - h * 60;
	s = (u32)(dur / 1000) - h * 3600 - m * 60;
	ms = (u32)(dur)-h * 3600000 - m * 60000 - s * 1000;
	if (h <= 24)
	{
		snprintf(szDur, szDur_size, "%02d:%02d:%02d.%03d", h, m, s, ms);
	}
	else
	{
		u32 d = (u32)(dur / 3600000 / 24);
		h = (u32)(dur / 3600000) - 24 * d;
		if (d <= 365)
		{
			snprintf(szDur, szDur_size, "%d Days, %02d:%02d:%02d.%03d", d, h, m, s, ms);
		}
		else
		{
			u32 y = 0;
			while (d > 365)
			{
				y++;
				d -= 365;
				if (y % 4)
					d--;
			}
			snprintf(szDur, szDur_size, "%d Years %d Days, %02d:%02d:%02d.%03d", y, d, h, m, s, ms);
		}
	}
	return szDur;
}

unsigned char *ccdp_find_data(unsigned char *ccdp_atom_content, unsigned int len, unsigned int *cc_count)
{
	unsigned char *data = ccdp_atom_content;

	if (len < 4)
	{
		dbg_print(CCX_DMT_PARSE, "mp4-708-cdp: unexpected size of cdp\n");
		return NULL;
	}

	unsigned int cdp_id = (data[0] << 8) | data[1];
	if (cdp_id != 0x9669)
	{
		dbg_print(CCX_DMT_PARSE, "mp4-708-cdp: unexpected header %hhX %hhX\n", data[0], data[1]);
		return NULL;
	}

	data += 2;
	len -= 2;

	unsigned int cdp_data_count = data[0];
	unsigned int cdp_frame_rate = data[1] >> 4; // frequency could be calculated
	if (cdp_data_count != len + 2)
	{
		dbg_print(CCX_DMT_PARSE, "mp4-708-cdp: unexpected data length %u %u\n", cdp_data_count, len + 2);
		return NULL;
	}

	data += 2;
	len -= 2;

	unsigned int cdp_flags = data[0];
	unsigned int cdp_counter = (data[1] << 8) | data[2];

	data += 3;
	len -= 3;

	unsigned int cdp_timecode_added = (cdp_flags & 0x80) >> 7;
	unsigned int cdp_data_added = (cdp_flags & 0x40) >> 6;

	if (!cdp_data_added)
	{
		dbg_print(CCX_DMT_PARSE, "mp4-708-cdp: packet without data\n");
		return NULL;
	}

	if (cdp_timecode_added)
	{
		data += 4;
		len -= 4;
	}

	if (data[0] != CDP_SECTION_DATA)
	{
		dbg_print(CCX_DMT_PARSE, "mp4-708-cdp: cdp_data_section byte not found\n");
		return NULL;
	}

	*cc_count = (unsigned int)(data[1] & 0x1F);

	if (*cc_count != 10 && *cc_count != 20 && *cc_count != 25 && *cc_count != 30)
	{
		dbg_print(CCX_DMT_PARSE, "mp4-708-cdp: unexpected cc_count %u\n", *cc_count);
		return NULL;
	}

	data += 2;
	len -= 2;

	if ((*cc_count) * 3 > len)
	{
		dbg_print(CCX_DMT_PARSE, "mp4-708-cdp: not enough bytes left (%u) to carry %u*3 bytes\n", len, *cc_count);
		return NULL;
	}

	(void)(cdp_counter);
	(void)(cdp_frame_rate);

	return data;
}

// Process clcp type atom
// Return the length of the atom
// Return -1 if unrecoverable error happened. In this case, the sample will be skipped.
static int process_clcp(struct lib_ccx_ctx *ctx, struct encoder_ctx *enc_ctx,
			struct lib_cc_decode *dec_ctx, struct cc_subtitle *dec_sub, int *mp4_ret,
			u32 sub_type, char *data, size_t data_length)
{
	unsigned int atom_length = RB32(data);
	if (atom_length < 8 || atom_length > data_length)
	{
		mprint("Invalid atom length. Atom length: %u,  should be: %u\n", atom_length, data_length);
		return -1;
	}
#ifdef MP4_DEBUG
	dump(256, (unsigned char *)data, atom_length - 8, 0, 1);
#endif
	data += 4;
	int is_ccdp = !strncmp(data, "ccdp", 4);

	if (!strncmp(data, "cdat", 4) || !strncmp(data, "cdt2", 4) || is_ccdp)
	{
		if (sub_type == GF_ISOM_SUBTYPE_C708)
		{
			if (!is_ccdp)
			{
				mprint("Your video file seems to be an interesting sample for us (%4.4s)\n", data);
				mprint("We haven't seen a c708 subtitle outside a ccdp atom before\n");
				mprint("Please, report\n");
				return -1;
			}

			unsigned int cc_count;
			data += 4;
			unsigned char *cc_data = ccdp_find_data((unsigned char *)data, data_length - 8, &cc_count);

			if (!cc_data)
			{
				dbg_print(CCX_DMT_PARSE, "mp4-708: no cc data found in ccdp\n");
				return -1;
			}

			ctx->dec_global_setting->settings_dtvcc->enabled = 1;
			unsigned char temp[4];
			for (int cc_i = 0; cc_i < cc_count; cc_i++, cc_data += 3)
			{
				unsigned char cc_info = cc_data[0];
				unsigned char cc_valid = (unsigned char)((cc_info & 4) >> 2);
				unsigned char cc_type = (unsigned char)(cc_info & 3);

				if (cc_info == CDP_SECTION_SVC_INFO || cc_info == CDP_SECTION_FOOTER)
				{
					dbg_print(CCX_DMT_PARSE, "MP4-708: premature end of sample (0x73 or 0x74)\n");
					break;
				}

				if ((cc_info == 0xFA || cc_info == 0xFC || cc_info == 0xFD) && (cc_data[1] & 0x7F) == 0 && (cc_data[2] & 0x7F) == 0)
				{
					dbg_print(CCX_DMT_PARSE, "MP4-708: skipped (zero cc data)\n");
					continue;
				}

				temp[0] = cc_valid;
				temp[1] = cc_type;
				temp[2] = cc_data[1];
				temp[3] = cc_data[2];

				if (cc_type < 2)
				{
					dbg_print(CCX_DMT_PARSE, "MP4-708: atom skipped (cc_type < 2)\n");
					continue;
				}
#ifndef DISABLE_RUST
				ccxr_dtvcc_process_data(dec_ctx->dtvcc_rust, cc_valid, cc_type, temp[2], temp[3]);
#else
				dtvcc_process_data(dec_ctx->dtvcc, (unsigned char *)temp);
#endif
				cb_708++;
			}
			if (ctx->write_format == CCX_OF_MCC)
			{
				mcc_encode_cc_data(enc_ctx, dec_ctx, cc_data, cc_count);
			}
		}
		else // subtype == GF_ISOM_SUBTYPE_C608
		{
			if (is_ccdp)
			{
				mprint("Your video file seems to be an interesting sample for us\n");
				mprint("We haven't seen a c608 subtitle inside a ccdp atom before\n");
				mprint("Please, report\n");
				return -1;
			}

			int ret = 0;
			int len = atom_length - 8;
			data += 4;
			char *tdata = data;
			do
			{
				// Process each pair independently so we can adjust timing
				ret = process608((unsigned char *)tdata, len > 2 ? 2 : len, dec_ctx, dec_sub);
				len -= ret;
				tdata += ret;
				cb_field1++;
				if (dec_sub->got_output)
				{
					*mp4_ret = 1;
					encode_sub(enc_ctx, dec_sub);
					dec_sub->got_output = 0;
				}
			} while (len > 0);
		}
	}
	return atom_length;
}

// Process tx3g type atom
// Return the length of the atom
// Return -1 if unrecoverable error happened or process of the atom is finished.
// In this case, the sample will be skipped.
// Argument `encode_last_only` is used to print the last subtitle
static int process_tx3g(struct lib_ccx_ctx *ctx, struct encoder_ctx *enc_ctx,
			struct lib_cc_decode *dec_ctx, struct cc_subtitle *dec_sub, int *mp4_ret,
			char *data, size_t data_length, int encode_last_only)
{
	// tx3g data format:
	// See https://developer.apple.com/library/content/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html (Section Subtitle Sample Data)
	// See http://www.etsi.org/deliver/etsi_ts/126200_126299/126245/14.00.00_60/ts_126245v140000p.pdf (Section 5.17)
	// Basically it's 16-bit length + UTF-8/UTF-16 text + optional extension (ignored)
	// TODO: support extension & style

	static struct cc_subtitle last_sub;
	static int has_previous_sub = 0;

	// Always encode the previous subtitle, no matter the current one is valid or not
	if (has_previous_sub)
	{
		dec_sub->end_time = dec_ctx->timing->fts_now;
		encode_sub(enc_ctx, dec_sub); // encode the previous subtitle
		has_previous_sub = 0;
	}
	if (encode_last_only)
		return 0; // Caller in this case doesn't care about the value

	unsigned int atom_length = RB16(data);
	data += 2;

	if (atom_length > data_length)
	{
		mprint("Invalid atom length. Atom length: %u, should be: %u\n", atom_length, data_length);
		return -1;
	}
	if (atom_length == 0)
	{
		return -1;
	}
#ifdef MP4_DEBUG
	dump(256, (unsigned char *)data, atom_length, 0, 1);
#endif

	// Start encode the current subtitle
	// But they won't be written to file until we get the next subtitle
	// So that we can know its end time
	dec_sub->type = CC_TEXT;
	dec_sub->start_time = dec_ctx->timing->fts_now;
	if (dec_sub->data != NULL)
		free(dec_sub->data);
	dec_sub->data = malloc(atom_length + 1);
	if (!dec_sub->data)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In process_tx3g_atom: Out of memory allocating subtitle data.");
	}
	dec_sub->datatype = CC_DATATYPE_GENERIC;
	memcpy(dec_sub->data, data, atom_length);
	*((char *)dec_sub->data + atom_length) = '\0';

	*mp4_ret = 1;
	has_previous_sub = 1;

	return -1; // Assume there's only one subtitle in one atom.
}

/*
	Here is application algorithm described in some C-like pseudo code:
		main(){
			media = open()
			for each track in media
				switch track
				AVC track:
					for each sample in track
						for each NALU in sample
							send to avc.c for processing
				CC track:
					for each sample in track
						deliver to corresponding process_xxx functions
			close(media)
		}

*/
int processmp4(struct lib_ccx_ctx *ctx, struct ccx_s_mp4Cfg *cfg, char *file)
{
	int mp4_ret = 0;
	GF_ISOFile *f;
	u32 i, j, track_count, avc_track_count, hevc_track_count, cc_track_count;
	struct cc_subtitle dec_sub;
	struct lib_cc_decode *dec_ctx = NULL;
	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);

	dec_ctx = update_decoder_list(ctx);

	if (enc_ctx)
		enc_ctx->timing = dec_ctx->timing;

		// WARN: otherwise cea-708 will not work
#ifndef DISABLE_RUST
	ccxr_dtvcc_set_encoder(dec_ctx->dtvcc_rust, enc_ctx);
#else
	dec_ctx->dtvcc->encoder = (void *)enc_ctx;
#endif

	memset(&dec_sub, 0, sizeof(dec_sub));
	if (file == NULL)
	{
		mprint("Error: NULL file path provided to processmp4\n");
		return -1;
	}
	mprint("Opening \'%s\': ", file);
#ifdef MP4_DEBUG
	gf_log_set_tool_level(GF_LOG_CONTAINER, GF_LOG_DEBUG);
#endif

	if ((f = gf_isom_open(file, GF_ISOM_OPEN_READ, NULL)) == NULL)
	{
		mprint("Failed to open input file (gf_isom_open() returned error)\n");
		free(dec_ctx->xds_ctx);
		return -2;
	}

	mprint("ok\n");

	track_count = gf_isom_get_track_count(f);

	avc_track_count = 0;
	hevc_track_count = 0;
	cc_track_count = 0;
	u32 vobsub_track_count = 0;

	for (i = 0; i < track_count; i++)
	{
		const u32 type = gf_isom_get_media_type(f, i + 1);
		const u32 subtype = gf_isom_get_media_subtype(f, i + 1, 1);
		mprint("Track %d, type=%c%c%c%c subtype=%c%c%c%c\n", i + 1, (unsigned char)(type >> 24 % 0x100),
		       (unsigned char)((type >> 16) % 0x100), (unsigned char)((type >> 8) % 0x100), (unsigned char)(type % 0x100),
		       (unsigned char)(subtype >> 24 % 0x100),
		       (unsigned char)((subtype >> 16) % 0x100), (unsigned char)((subtype >> 8) % 0x100), (unsigned char)(subtype % 0x100));
		if (type == GF_ISOM_MEDIA_CLOSED_CAPTION || type == GF_ISOM_MEDIA_SUBT || type == GF_ISOM_MEDIA_TEXT)
			cc_track_count++;
		if (type == GF_ISOM_MEDIA_VISUAL && subtype == GF_ISOM_SUBTYPE_AVC_H264)
			avc_track_count++;
		if (type == GF_ISOM_MEDIA_VISUAL && (subtype == GF_ISOM_SUBTYPE_HEV1 || subtype == GF_ISOM_SUBTYPE_HVC1))
			hevc_track_count++;
		if (type == GF_ISOM_MEDIA_SUBPIC && subtype == GF_ISOM_SUBTYPE_MPEG4)
			vobsub_track_count++;
	}

	mprint("MP4: found %u tracks: %u avc, %u hevc, %u cc, %u vobsub\n", track_count, avc_track_count, hevc_track_count, cc_track_count, vobsub_track_count);

	for (i = 0; i < track_count; i++)
	{
		const u32 type = gf_isom_get_media_type(f, i + 1);
		const u32 subtype = gf_isom_get_media_subtype(f, i + 1, 1);
		mprint("Processing track %d, type=%c%c%c%c subtype=%c%c%c%c\n", i + 1,
		       (unsigned char)(type >> 24 % 0x100), (unsigned char)((type >> 16) % 0x100),
		       (unsigned char)((type >> 8) % 0x100), (unsigned char)(type % 0x100),
		       (unsigned char)(subtype >> 24 % 0x100), (unsigned char)((subtype >> 16) % 0x100),
		       (unsigned char)((subtype >> 8) % 0x100), (unsigned char)(subtype % 0x100));

		const u64 track_type = MEDIA_TYPE(type, subtype);

		switch (track_type)
		{
			case MEDIA_TYPE(GF_ISOM_MEDIA_VISUAL, GF_ISOM_SUBTYPE_XDVB): // vide:xdvb
				if (cc_track_count && !cfg->mp4vidtrack)
					continue;
				// If there are multiple tracks, change fd for different tracks
				if (avc_track_count > 1)
				{
					switch_output_file(ctx, enc_ctx, i);
				}
				if (process_xdvb_track(ctx, file, f, i + 1, &dec_sub) != 0)
				{
					mprint("Error on process_xdvb_track()\n");
					free(dec_ctx->xds_ctx);
					return -3;
				}
				if (dec_sub.got_output)
				{
					mp4_ret = 1;
					encode_sub(enc_ctx, &dec_sub);
					dec_sub.got_output = 0;
				}
				break;

			case MEDIA_TYPE(GF_ISOM_MEDIA_VISUAL, GF_ISOM_SUBTYPE_AVC_H264): // vide:avc1
				if (cc_track_count && !cfg->mp4vidtrack)
					continue;
				// If there are multiple tracks, change fd for different tracks
				if (avc_track_count > 1)
				{
					switch_output_file(ctx, enc_ctx, i);
				}
				GF_AVCConfig *cnf = gf_isom_avc_config_get(f, i + 1, 1);
				if (cnf != NULL)
				{
					for (j = 0; j < gf_list_count(cnf->sequenceParameterSets); j++)
					{
						GF_AVCConfigSlot *seqcnf = (GF_AVCConfigSlot *)gf_list_get(cnf->sequenceParameterSets, j);
						do_NAL(enc_ctx, dec_ctx, (unsigned char *)seqcnf->data, seqcnf->size, &dec_sub);
					}
					gf_odf_avc_cfg_del(cnf);
				}
				if (process_avc_track(ctx, file, f, i + 1, &dec_sub) != 0)
				{
					mprint("Error on process_avc_track()\n");
					free(dec_ctx->xds_ctx);
					return -3;
				}
				if (dec_sub.got_output)
				{
					mp4_ret = 1;
					encode_sub(enc_ctx, &dec_sub);
					dec_sub.got_output = 0;
				}
				break;

			case MEDIA_TYPE(GF_ISOM_MEDIA_VISUAL, GF_ISOM_SUBTYPE_HEV1): // vide:hev1 (HEVC)
			case MEDIA_TYPE(GF_ISOM_MEDIA_VISUAL, GF_ISOM_SUBTYPE_HVC1): // vide:hvc1 (HEVC)
				if (cc_track_count && !cfg->mp4vidtrack)
					continue;
				// If there are multiple tracks, change fd for different tracks
				if (hevc_track_count > 1)
				{
					switch_output_file(ctx, enc_ctx, i);
				}
				// Enable HEVC mode for caption extraction
				dec_ctx->avc_ctx->is_hevc = 1;

				// Process VPS/SPS/PPS from HEVC config to enable SEI parsing
				GF_HEVCConfig *hevc_cnf = gf_isom_hevc_config_get(f, i + 1, 1);
				if (hevc_cnf != NULL)
				{
					// Process parameter sets from config
					for (j = 0; j < gf_list_count(hevc_cnf->param_array); j++)
					{
						GF_NALUFFParamArray *ar = (GF_NALUFFParamArray *)gf_list_get(hevc_cnf->param_array, j);
						if (ar)
						{
							for (u32 k = 0; k < gf_list_count(ar->nalus); k++)
							{
								GF_NALUFFParam *sl = (GF_NALUFFParam *)gf_list_get(ar->nalus, k);
								if (sl && sl->data && sl->size > 0)
								{
									do_NAL(enc_ctx, dec_ctx, (unsigned char *)sl->data, sl->size, &dec_sub);
								}
							}
						}
					}
					gf_odf_hevc_cfg_del(hevc_cnf);
				}
				if (process_hevc_track(ctx, file, f, i + 1, &dec_sub) != 0)
				{
					mprint("Error on process_hevc_track()\n");
					free(dec_ctx->xds_ctx);
					return -3;
				}
				if (dec_sub.got_output)
				{
					mp4_ret = 1;
					encode_sub(enc_ctx, &dec_sub);
					dec_sub.got_output = 0;
				}
				break;

			case MEDIA_TYPE(GF_ISOM_MEDIA_SUBPIC, GF_ISOM_SUBTYPE_MPEG4): // subp:MPEG (VOBSUB)
				// If there are multiple VOBSUB tracks, change fd for different tracks
				if (vobsub_track_count > 1)
				{
					switch_output_file(ctx, enc_ctx, i);
				}
				if (process_vobsub_track(ctx, f, i + 1, &dec_sub) != 0)
				{
					mprint("Error on process_vobsub_track()\n");
					free(dec_ctx->xds_ctx);
					return -3;
				}
				if (dec_sub.got_output)
				{
					mp4_ret = 1;
				}
				break;

			default:
				if (type != GF_ISOM_MEDIA_CLOSED_CAPTION && type != GF_ISOM_MEDIA_SUBT && type != GF_ISOM_MEDIA_TEXT)
					break; // ignore non cc track

				if (avc_track_count && cfg->mp4vidtrack)
					continue;
				// If there are multiple tracks, change fd for different tracks
				if (cc_track_count > 1)
				{
					switch_output_file(ctx, enc_ctx, i);
				}
				unsigned num_samples = gf_isom_get_sample_count(f, i + 1);

				u32 ProcessingStreamDescriptionIndex = 0; // Current track we are processing, 0 = we don't know yet
				u32 timescale = gf_isom_get_media_timescale(f, i + 1);
#ifdef MP4_DEBUG
				unsigned num_streams = gf_isom_get_sample_description_count(f, i + 1);
				u64 duration = gf_isom_get_media_duration(f, i + 1);
				mprint("%u streams\n", num_streams);
				mprint("%u sample counts\n", num_samples);
				mprint("%u timescale\n", (unsigned)timescale);
				mprint("%u duration\n", (unsigned)duration);
#endif
				for (unsigned k = 0; k < num_samples; k++)
				{
					u32 StreamDescriptionIndex;
					GF_ISOSample *sample = gf_isom_get_sample(f, i + 1, k + 1, &StreamDescriptionIndex);
					if (ProcessingStreamDescriptionIndex && ProcessingStreamDescriptionIndex != StreamDescriptionIndex)
					{
						mprint("This sample seems to have more than one description. This isn't supported yet.\n");
						mprint("Submitting this video file will help add support to this case.\n");
						break;
					}
					if (!ProcessingStreamDescriptionIndex)
						ProcessingStreamDescriptionIndex = StreamDescriptionIndex;
					if (sample == NULL)
						continue;
#ifdef MP4_DEBUG
					mprint("Data length: %lu\n", sample->dataLength);
					const LLONG timestamp = (LLONG)((sample->DTS + sample->CTS_Offset) * 1000) / timescale;
#endif
					set_current_pts(dec_ctx->timing, (sample->DTS + sample->CTS_Offset) * MPEG_CLOCK_FREQ / timescale);
					// For caption-only tracks (c608/c708), set frame type to I-frame
					// so that set_fts() will set min_pts from the first sample.
					// Without video frames, frame type would stay Unknown and
					// min_pts would never be set, causing broken timestamps.
					if (type == GF_ISOM_MEDIA_CLOSED_CAPTION)
						dec_ctx->timing->current_picture_coding_type = CCX_FRAME_TYPE_I_FRAME;
					set_fts(dec_ctx->timing);

					int atomStart = 0;
					// Process Atom by Atom
					while (atomStart < sample->dataLength)
					{
						char *data = sample->data + atomStart;
						size_t atom_length = -1;
						switch (track_type)
						{
							case MEDIA_TYPE(GF_ISOM_MEDIA_TEXT, GF_ISOM_SUBTYPE_TX3G): // text:tx3g
							case MEDIA_TYPE(GF_ISOM_MEDIA_SUBT, GF_ISOM_SUBTYPE_TX3G): // sbtl:tx3g (they're the same)
								atom_length = process_tx3g(ctx, enc_ctx, dec_ctx,
											   &dec_sub, &mp4_ret,
											   data, sample->dataLength, 0);
								break;
							case MEDIA_TYPE(GF_ISOM_MEDIA_CLOSED_CAPTION, GF_QT_SUBTYPE_C608):   // clcp:c608
							case MEDIA_TYPE(GF_ISOM_MEDIA_CLOSED_CAPTION, GF_ISOM_SUBTYPE_C708): // clcp:c708
								atom_length = process_clcp(ctx, enc_ctx, dec_ctx,
											   &dec_sub, &mp4_ret, subtype,
											   data, sample->dataLength);
								break;
							case MEDIA_TYPE(GF_ISOM_MEDIA_TEXT, GF_ISOM_SUBTYPE_TEXT): // text:text
							{
								static int unsupported_reported = 0;
								if (!unsupported_reported)
								{
									mprint("\ntext:text not yet supported, see\n");
									mprint("https://developer.apple.com/library/mac/documentation/quicktime/qtff/QTFFChap3/qtff3.html\n");
									mprint("If you want to work on a PR.\n");
									unsupported_reported = 1;
								}
								break;
							}

							default:
								// See https://gpac.wp.imt.fr/2014/09/04/subtitling-with-gpac/ for more possible types
								mprint("\nUnsupported track type \"%c%c%c%c:%c%c%c%c\". Please report.\n",
								       (unsigned char)(type >> 24 % 0x100), (unsigned char)((type >> 16) % 0x100),
								       (unsigned char)((type >> 8) % 0x100), (unsigned char)(type % 0x100),
								       (unsigned char)(subtype >> 24 % 0x100), (unsigned char)((subtype >> 16) % 0x100),
								       (unsigned char)((subtype >> 8) % 0x100), (unsigned char)(subtype % 0x100));
						}
						if (atom_length == -1)
							break; // error happened or process of the sample is finished
						atomStart += atom_length;
					}
					free(sample->data);
					free(sample);

					// End of change
					int progress = (int)((k * 100) / num_samples);
					if (ctx->last_reported_progress != progress)
					{
						int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
						activity_progress(progress, cur_sec / 60, cur_sec % 60);
						ctx->last_reported_progress = progress;
					}
				}

				// Encode the last subtitle
				if (subtype == GF_ISOM_SUBTYPE_TX3G)
				{
					process_tx3g(ctx, enc_ctx, dec_ctx,
						     &dec_sub, &mp4_ret,
						     NULL, 0, 1);
				}

				int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
				activity_progress(100, cur_sec / 60, cur_sec % 60);
		}
	}

	freep(&dec_ctx->xds_ctx);

	mprint("\nClosing media: ");
	gf_isom_close(f);
	f = NULL;
	mprint("ok\n");

	if (avc_track_count)
		mprint("Found %d AVC track(s). ", avc_track_count);
	else
		mprint("Found no AVC track(s). ");

	if (hevc_track_count)
		mprint("Found %d HEVC track(s). ", hevc_track_count);
	else
		mprint("Found no HEVC track(s). ");

	if (cc_track_count)
		mprint("Found %d CC track(s). ", cc_track_count);
	else
		mprint("Found no dedicated CC track(s). ");

	if (vobsub_track_count)
		mprint("Found %d VOBSUB track(s).\n", vobsub_track_count);
	else
		mprint("\n");

	ctx->freport.mp4_cc_track_cnt = cc_track_count;

	if ((dec_ctx->write_format == CCX_OF_MCC) && (dec_ctx->saw_caption_block == CCX_TRUE))
	{
		mp4_ret = 1;
	}

	return mp4_ret;
}

int dumpchapters(struct lib_ccx_ctx *ctx, struct ccx_s_mp4Cfg *cfg, char *file)
{
	int mp4_ret = 0;
	GF_ISOFile *f;
	mprint("Opening \'%s\': ", file);
#ifdef MP4_DEBUG
	gf_log_set_tool_level(GF_LOG_CONTAINER, GF_LOG_DEBUG);
#endif

	if ((f = gf_isom_open(file, GF_ISOM_OPEN_READ, NULL)) == NULL)
	{
		mprint("failed to open\n");
		return 5;
	}

	mprint("ok\n");

	char szName[1024];
	FILE *t;
	u32 i, count;
	count = gf_isom_get_chapter_count(f, 0);
	if (count > 0)
	{
		if (file)
		{
			snprintf(szName, sizeof(szName), "%s.txt", get_basename(file));

			t = gf_fopen(szName, "wt");
			if (!t)
				return 5;
		}
		else
		{
			t = stdout;
		}
		mp4_ret = 1;
		printf("Writing chapters into %s\n", szName);
	}
	else
	{
		mprint("No chapters information found!\n");
	}

	for (i = 0; i < count; i++)
	{
		u64 chapter_time;
		const char *name;
		char szDur[64];
		gf_isom_get_chapter(f, 0, i + 1, &chapter_time, &name);
		fprintf(t, "CHAPTER%02d=%s\n", i + 1, format_duration(chapter_time, 1000, szDur, sizeof(szDur)));
		fprintf(t, "CHAPTER%02dNAME=%s\n", i + 1, name);
	}
	if (file)
		gf_fclose(t);
	return mp4_ret;
}
