#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <gpac/isomedia.h>
#include "lib_ccx.h"
#include "utility.h"
#include "ccx_encoders_common.h"
#include "ccx_encoders_mcc.h"
#include "ccx_common_option.h"
#include "ccx_mp4.h"
#include "activity.h"
#include "ccx_dtvcc.h"

#define MEDIA_TYPE(type, subtype) (((u64)(type)<<32)+(subtype))

static short bswap16(short v)
{
	return ((v >> 8) & 0x00FF) | ((v << 8) & 0xFF00);
}

static long bswap32(long v)
{
	// For 0x12345678 returns 78563412	
	long swapped=((v&0xFF)<<24) | ((v&0xFF00)<<8) | ((v&0xFF0000) >>8) | ((v&0xFF000000) >>24);	
	return swapped;
}
static struct {
	unsigned total;
	unsigned type[32];
}s_nalu_stats;

static int process_avc_sample(struct lib_ccx_ctx *ctx, u32 timescale, GF_AVCConfig* c, GF_ISOSample* s, struct cc_subtitle *sub)
{
	int status = 0;
	u32 i;
	s32 signed_cts=(s32) s->CTS_Offset; // Convert from unsigned to signed. GPAC uses u32 but unsigned values are legal.
	struct lib_cc_decode *dec_ctx = NULL;
    struct encoder_ctx *enc_ctx = NULL;

	dec_ctx = update_decoder_list(ctx);
	enc_ctx = update_encoder_list(ctx);

	set_current_pts(dec_ctx->timing, (s->DTS + signed_cts)*MPEG_CLOCK_FREQ/timescale);
	set_fts(dec_ctx->timing);

	for(i = 0; i < s->dataLength; )
	{
		u32 nal_length;

		switch(c->nal_unit_size)
		{
			case 1:
				nal_length = s->data[i];
				break;
			case 2:
				nal_length = bswap16(*(short* )&s->data[i]);
				break;
			case 4:
				nal_length = bswap32(*(long* )&s->data[i]);
				break;
		}
		i += c->nal_unit_size;

		s_nalu_stats.total += 1;
		s_nalu_stats.type[s->data[i] & 0x1F] += 1;

		temp_debug=0;

		if (nal_length>0)
			do_NAL (enc_ctx, dec_ctx, (unsigned char *) &(s->data[i]) ,nal_length, sub);
		i += nal_length;
	} // outer for
	assert(i == s->dataLength);

	return status;
}
static int process_xdvb_track(struct lib_ccx_ctx *ctx, const char* basename, GF_ISOFile* f, u32 track, struct cc_subtitle *sub)
{
	u32 timescale, i, sample_count;
	int status;

	struct lib_cc_decode *dec_ctx = NULL;
    struct encoder_ctx *enc_ctx = NULL;

	dec_ctx = update_decoder_list(ctx);
    enc_ctx = update_encoder_list(ctx);
	if((sample_count = gf_isom_get_sample_count(f, track)) < 1)
	{
		return 0;
	}

	timescale = gf_isom_get_media_timescale(f, track);

	status = 0;

	for(i = 0; i < sample_count; i++)
	{
		u32 sdi;
		
		GF_ISOSample* s = gf_isom_get_sample(f, track, i + 1, &sdi);
		if (s!=NULL)
		{
			s32 signed_cts=(s32) s->CTS_Offset; // Convert from unsigned to signed. GPAC uses u32 but unsigned values are legal.
			set_current_pts(dec_ctx->timing, (s->DTS + signed_cts)*MPEG_CLOCK_FREQ/timescale);
			set_fts(dec_ctx->timing);

			process_m2v (enc_ctx, dec_ctx, (unsigned char *) s->data,s->dataLength, sub);
			gf_isom_sample_del(&s);
		}

		int progress = (int) ((i*100) / sample_count);
		if (ctx->last_reported_progress != progress)
		{
			int cur_sec = (int) (get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
			activity_progress(progress, cur_sec/60, cur_sec%60);
			ctx->last_reported_progress = progress;
		}
	}
	int cur_sec = (int) (get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
	activity_progress(100, cur_sec/60, cur_sec%60);

	return status;
}

static int process_avc_track(struct lib_ccx_ctx *ctx, const char* basename, GF_ISOFile* f, u32 track, struct cc_subtitle *sub)
{
	u32 timescale, i, sample_count, last_sdi = 0;
	int status;
	GF_AVCConfig* c = NULL;
	struct lib_cc_decode *dec_ctx = NULL;

	dec_ctx = update_decoder_list(ctx);
	
	if((sample_count = gf_isom_get_sample_count(f, track)) < 1)
	{
		return 0;
	}

	timescale = gf_isom_get_media_timescale(f, track);

	status = 0;

	for(i = 0; i < sample_count; i++)
	{
		u32 sdi;
		
		GF_ISOSample* s = gf_isom_get_sample(f, track, i + 1, &sdi);

		if(s != NULL)
		{
			if(sdi != last_sdi)
			{
				if(c != NULL)
				{
					gf_odf_avc_cfg_del(c);
					c = NULL;
				}
				
				if((c = gf_isom_avc_config_get(f, track, sdi)) == NULL)
				{
					gf_isom_sample_del(&s);
					status = -1;
					break;
				}

				last_sdi = sdi;
			}

			status = process_avc_sample(ctx, timescale, c, s, sub);

			gf_isom_sample_del(&s);

			if(status != 0)
			{
				break;
			}
		}

		int progress = (int) ((i*100) / sample_count);
		if (ctx->last_reported_progress != progress)
		{
			int cur_sec = (int) (get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
			activity_progress(progress, cur_sec/60, cur_sec%60);
			ctx->last_reported_progress = progress;
		}
	}
	int cur_sec = (int) (get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
	activity_progress(100, cur_sec/60, cur_sec%60);

	if(c != NULL)
	{
		gf_odf_avc_cfg_del(c);
		c = NULL;
	}

	return status;
}

static char *format_duration(u64 dur, u32 timescale, char *szDur)
{
	u32 h, m, s, ms;
	if ((dur==(u64) -1) || (dur==(u32) -1))  {
		strcpy(szDur, "Unknown");
		return szDur;
	}
	dur = (u64) (( ((Double) (s64) dur)/timescale)*1000);
	h = (u32) (dur / 3600000);
	m = (u32) (dur/ 60000) - h*60;
	s = (u32) (dur/1000) - h*3600 - m*60;
	ms = (u32) (dur) - h*3600000 - m*60000 - s*1000;
	if (h<=24) {
		sprintf(szDur, "%02d:%02d:%02d.%03d", h, m, s, ms);
	} else {
		u32 d = (u32) (dur / 3600000 / 24);
		h = (u32) (dur/3600000)-24*d;
		if (d<=365) {
			sprintf(szDur, "%d Days, %02d:%02d:%02d.%03d", d, h, m, s, ms);
		} else {
			u32 y=0;
			while (d>365) {
				y++;
				d-=365;
				if (y%4) d--;
			}
			sprintf(szDur, "%d Years %d Days, %02d:%02d:%02d.%03d", y, d, h, m, s, ms);
		}

	}
	return szDur;
}

unsigned char * ccdp_find_data(unsigned char * ccdp_atom_content, unsigned int len, unsigned int *cc_count)
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
	unsigned int cdp_frame_rate = data[1] >> 4; //frequency could be calculated
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

	*cc_count = (unsigned int) (data[1] & 0x1F);

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
	if (atom_length < 8 || atom_length > data_length) {
		mprint("Invalid atom length. Atom length: %u,  should be: %u\n", atom_length, data_length);
		return -1;
	}
#ifdef MP4_DEBUG
	dump(256, (unsigned char *)data, atom_length - 8, 0, 1);
#endif
	data += 4;
	int is_ccdp = !strncmp(data, "ccdp", 4);

	if (!strncmp(data, "cdat", 4) || !strncmp(data, "cdt2", 4) || is_ccdp) {
		if (sub_type == GF_ISOM_SUBTYPE_C708) {
			if (!is_ccdp) {
				mprint("Your video file seems to be an interesting sample for us (%4.4s)\n", data);
				mprint("We haven't seen a c708 subtitle outside a ccdp atom before\n");
				mprint("Please, report\n");
				return -1;
			}

			unsigned int cc_count;
			data += 4;
			unsigned char *cc_data = ccdp_find_data((unsigned char *)data, data_length - 8, &cc_count);

			if (!cc_data) {
				dbg_print(CCX_DMT_PARSE, "mp4-708: no cc data found in ccdp\n");
				return -1;
			}

			ctx->dec_global_setting->settings_dtvcc->enabled = 1;
			unsigned char temp[4];
			for (int cc_i = 0; cc_i < cc_count; cc_i++, cc_data += 3) {
				unsigned char cc_info = cc_data[0];
				unsigned char cc_valid = (unsigned char)((cc_info & 4) >> 2);
				unsigned char cc_type = (unsigned char)(cc_info & 3);

				if (cc_info == CDP_SECTION_SVC_INFO || cc_info == CDP_SECTION_FOOTER) {
					dbg_print(CCX_DMT_PARSE, "MP4-708: premature end of sample (0x73 or 0x74)\n");
					break;
				}

				if ((cc_info == 0xFA || cc_info == 0xFC || cc_info == 0xFD)
					&& (cc_data[1] & 0x7F) == 0 && (cc_data[2] & 0x7F) == 0) {
					dbg_print(CCX_DMT_PARSE, "MP4-708: skipped (zero cc data)\n");
					continue;
				}

				temp[0] = cc_valid;
				temp[1] = cc_type;
				temp[2] = cc_data[1];
				temp[3] = cc_data[2];

				if (cc_type < 2) {
					dbg_print(CCX_DMT_PARSE, "MP4-708: atom skipped (cc_type < 2)\n");
					continue;
				}
				dec_ctx->dtvcc->encoder = (void *)enc_ctx; //WARN: otherwise cea-708 will not work
														   //TODO is it really always 4-bytes long?
				ccx_dtvcc_process_data(dec_ctx, (unsigned char *)temp, 4);
				cb_708++;
			}
			if( ctx->write_format == CCX_OF_MCC ) {
                mcc_encode_cc_data(enc_ctx, dec_ctx, cc_data, cc_count);
            }
		}
		else //subtype == GF_ISOM_SUBTYPE_C608
		{
			if (is_ccdp) {
				mprint("Your video file seems to be an interesting sample for us\n");
				mprint("We haven't seen a c608 subtitle inside a ccdp atom before\n");
				mprint("Please, report\n");
				return -1;
			}

			int ret = 0;
			int len = atom_length - 8;
			data += 4;
			char *tdata = data;
			do {
				// Process each pair independently so we can adjust timing
				ret = process608((unsigned char *)tdata, len > 2 ? 2 : len, dec_ctx, dec_sub);
				len -= ret;
				tdata += ret;
				cb_field1++;
				if (dec_sub->got_output) {
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
	char *data, size_t data_length, int encode_last_only) {
	// tx3g data format:
	// See https://developer.apple.com/library/content/documentation/QuickTime/QTFF/QTFFChap3/qtff3.html (Section Subtitle Sample Data)
	// See http://www.etsi.org/deliver/etsi_ts/126200_126299/126245/14.00.00_60/ts_126245v140000p.pdf (Section 5.17)
	// Basically it's 16-bit length + UTF-8/UTF-16 text + optional extension (ignored)
	// TODO: support extension & style

	static struct cc_subtitle last_sub;
	static int has_previous_sub = 0;

	// Always encode the previous subtitle, no matter the current one is valid or not
	if (has_previous_sub) {
		dec_sub->end_time = dec_ctx->timing->fts_now;
		encode_sub(enc_ctx, dec_sub); // encode the previous subtitle
		has_previous_sub = 0;
	}
	if (encode_last_only) return 0; // Caller in this case doesn't care about the value

	unsigned int atom_length = RB16(data);
	data += 2;
	
	if (atom_length > data_length) {
		mprint("Invalid atom length. Atom length: %u, should be: %u\n", atom_length, data_length);
		return -1;
	}
	if (atom_length == 0) {
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
	if (dec_sub->data != NULL) free(dec_sub->data);
	dec_sub->data = malloc(atom_length + 1);
	dec_sub->datatype = CC_DATATYPE_GENERIC;
	memcpy(dec_sub->data, data, atom_length);
	*((char*)dec_sub->data + atom_length) = '\0';

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
int processmp4 (struct lib_ccx_ctx *ctx, struct ccx_s_mp4Cfg *cfg, char *file)
{
	int mp4_ret = 0;
	GF_ISOFile* f;
	u32 i, j, track_count, avc_track_count, cc_track_count;
	struct cc_subtitle dec_sub;
	struct lib_cc_decode *dec_ctx = NULL;
	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);

	dec_ctx = update_decoder_list(ctx);

	if (enc_ctx)
		enc_ctx->timing = dec_ctx->timing;

	memset(&dec_sub,0,sizeof(dec_sub));
	mprint("Opening \'%s\': ", file);
#ifdef MP4_DEBUG
	gf_log_set_tool_level(GF_LOG_CONTAINER,GF_LOG_DEBUG);
#endif

	if((f = gf_isom_open(file, GF_ISOM_OPEN_READ, NULL)) == NULL)
	{
		mprint("Failed to open input file (gf_isom_open() returned error)\n");
		free(dec_ctx->xds_ctx);
		return -2;
	}

	mprint("ok\n");

	track_count = gf_isom_get_track_count(f);

	avc_track_count = 0;
	cc_track_count = 0;

	for(i = 0; i < track_count; i++)
	{
		const u32 type = gf_isom_get_media_type(f, i + 1);
		const u32 subtype = gf_isom_get_media_subtype(f, i + 1, 1);
		mprint ("Track %d, type=%c%c%c%c subtype=%c%c%c%c\n", i+1, (unsigned char) (type>>24%0x100),
			(unsigned char) ((type>>16)%0x100),(unsigned char) ((type>>8)%0x100),(unsigned char) (type%0x100),
			(unsigned char) (subtype>>24%0x100),
			(unsigned char) ((subtype>>16)%0x100),(unsigned char) ((subtype>>8)%0x100),(unsigned char) (subtype%0x100));
		if (type == GF_ISOM_MEDIA_CAPTIONS || type == GF_ISOM_MEDIA_SUBT || type == GF_ISOM_MEDIA_TEXT)
			cc_track_count++;
		if (type == GF_ISOM_MEDIA_VISUAL && subtype == GF_ISOM_SUBTYPE_AVC_H264)
			avc_track_count++;
	}

	mprint("MP4: found %u tracks: %u avc and %u cc\n", track_count, avc_track_count, cc_track_count);

	for (i = 0; i < track_count; i++) {
		const u32 type = gf_isom_get_media_type(f, i + 1);
		const u32 subtype = gf_isom_get_media_subtype(f, i + 1, 1);
		mprint("Processing track %d, type=%c%c%c%c subtype=%c%c%c%c\n", i + 1,
			(unsigned char)(type >> 24 % 0x100), (unsigned char)((type >> 16) % 0x100),
			(unsigned char)((type >> 8) % 0x100), (unsigned char)(type % 0x100),
			(unsigned char)(subtype >> 24 % 0x100), (unsigned char)((subtype >> 16) % 0x100),
			(unsigned char)((subtype >> 8) % 0x100), (unsigned char)(subtype % 0x100));

		const u64 track_type = MEDIA_TYPE(type, subtype);

		switch (track_type) {
		case MEDIA_TYPE(GF_ISOM_MEDIA_VISUAL, GF_ISOM_SUBTYPE_XDVB): //vide:xdvb
			if (cc_track_count && !cfg->mp4vidtrack)
				continue;
			// If there are multiple tracks, change fd for different tracks
			if (avc_track_count > 1) {
				switch_output_file(ctx, enc_ctx, i);
			}
			if (process_xdvb_track(ctx, file, f, i + 1, &dec_sub) != 0) {
				mprint("Error on process_xdvb_track()\n");
				free(dec_ctx->xds_ctx);
				return -3;
			}
			if (dec_sub.got_output) {
				mp4_ret = 1;
				encode_sub(enc_ctx, &dec_sub);
				dec_sub.got_output = 0;
			}
			break;

		case MEDIA_TYPE(GF_ISOM_MEDIA_VISUAL, GF_ISOM_SUBTYPE_AVC_H264): // vide:avc1
			if (cc_track_count && !cfg->mp4vidtrack)
				continue;
			// If there are multiple tracks, change fd for different tracks
			if (avc_track_count > 1) {
				switch_output_file(ctx, enc_ctx, i);
			}
			GF_AVCConfig *cnf = gf_isom_avc_config_get(f, i + 1, 1);
			if (cnf != NULL) {
				for (j = 0; j < gf_list_count(cnf->sequenceParameterSets); j++) {
					GF_AVCConfigSlot* seqcnf = (GF_AVCConfigSlot*)gf_list_get(cnf->sequenceParameterSets, j);
					do_NAL(enc_ctx, dec_ctx, (unsigned char *)seqcnf->data, seqcnf->size, &dec_sub);
				}
			}
			if (process_avc_track(ctx, file, f, i + 1, &dec_sub) != 0) {
				mprint("Error on process_avc_track()\n");
				free(dec_ctx->xds_ctx);
				return -3;
			}
			if (dec_sub.got_output) {
				mp4_ret = 1;
				encode_sub(enc_ctx, &dec_sub);
				dec_sub.got_output = 0;
			}
			break;

		default:
			if (type != GF_ISOM_MEDIA_CAPTIONS && type != GF_ISOM_MEDIA_SUBT && type != GF_ISOM_MEDIA_TEXT)
				break; // ignore non cc track

			if (avc_track_count && cfg->mp4vidtrack)
				continue;
			// If there are multiple tracks, change fd for different tracks
			if (cc_track_count > 1) {
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
			for (unsigned k = 0; k < num_samples; k++) {
				u32 StreamDescriptionIndex;
				GF_ISOSample *sample = gf_isom_get_sample(f, i + 1, k + 1, &StreamDescriptionIndex);
				if (ProcessingStreamDescriptionIndex && ProcessingStreamDescriptionIndex != StreamDescriptionIndex) {
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
				set_current_pts(dec_ctx->timing, (sample->DTS + sample->CTS_Offset)*MPEG_CLOCK_FREQ / timescale);
				set_fts(dec_ctx->timing);

				int atomStart = 0;
				// Process Atom by Atom
				while (atomStart < sample->dataLength) {
					char *data = sample->data + atomStart;
					size_t atom_length = -1;
					switch (track_type) {
					case MEDIA_TYPE(GF_ISOM_MEDIA_TEXT, GF_ISOM_SUBTYPE_TX3G): // text:tx3g
					case MEDIA_TYPE(GF_ISOM_MEDIA_SUBT, GF_ISOM_SUBTYPE_TX3G): // sbtl:tx3g (they're the same)
						atom_length = process_tx3g(ctx, enc_ctx, dec_ctx,
							&dec_sub, &mp4_ret,
							data, sample->dataLength, 0);
						break;
					case MEDIA_TYPE(GF_ISOM_MEDIA_CAPTIONS, GF_ISOM_SUBTYPE_C608): // clcp:c608
					case MEDIA_TYPE(GF_ISOM_MEDIA_CAPTIONS, GF_ISOM_SUBTYPE_C708): // clcp:c708
						atom_length = process_clcp(ctx, enc_ctx, dec_ctx,
							&dec_sub, &mp4_ret, subtype,
							data, sample->dataLength);
						break;
					default:
						// See https://gpac.wp.imt.fr/2014/09/04/subtitling-with-gpac/ for more possible types
						mprint("\nUnsupported track type %c%c%c%c:%c%c%c%c! Please report.\n",
							(unsigned char)(type >> 24 % 0x100), (unsigned char)((type >> 16) % 0x100),
							(unsigned char)((type >> 8) % 0x100), (unsigned char)(type % 0x100),
							(unsigned char)(subtype >> 24 % 0x100), (unsigned char)((subtype >> 16) % 0x100),
							(unsigned char)((subtype >> 8) % 0x100), (unsigned char)(subtype % 0x100));
					}
					if (atom_length == -1) break; // error happened or process of the sample is finished
					atomStart += atom_length;
				}
				free(sample->data);
				free(sample);

				// End of change
				int progress = (int)((k * 100) / num_samples);
				if (ctx->last_reported_progress != progress) {
					int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
					activity_progress(progress, cur_sec / 60, cur_sec % 60);
					ctx->last_reported_progress = progress;
				}
			}
			
			// Encode the last subtitle
			if (subtype == GF_ISOM_SUBTYPE_TX3G) {
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
	mprint ("ok\n");

	if(avc_track_count)
		mprint("Found %d AVC track(s). ", avc_track_count);
	else
		mprint("Found no AVC track(s). ");

	if (cc_track_count)
		mprint ("Found %d CC track(s).\n", cc_track_count);
	else
		mprint ("Found no dedicated CC track(s).\n");

	ctx->freport.mp4_cc_track_cnt = cc_track_count;

	if( (dec_ctx->write_format == CCX_OF_MCC) && (dec_ctx->saw_caption_block == CCX_TRUE) ) {
	    mp4_ret = 1;
	}

	return mp4_ret;
}

int dumpchapters (struct lib_ccx_ctx *ctx,struct ccx_s_mp4Cfg *cfg, char *file)
{
	int mp4_ret = 0;
	GF_ISOFile* f;
	mprint("Opening \'%s\': ", file);
#ifdef MP4_DEBUG
	gf_log_set_tool_level(GF_LOG_CONTAINER,GF_LOG_DEBUG);
#endif

	if((f = gf_isom_open(file, GF_ISOM_OPEN_READ, NULL)) == NULL)
	{
		mprint("failed to open\n");
		return 5;
	}

	mprint("ok\n");

	char szName[1024];
	FILE *t;
	u32 i, count;
	count = gf_isom_get_chapter_count(f, 0);
	if(count>0)
	{
		if (file) 
		{
			strcpy(szName, get_basename(file));
			strcat(szName, ".txt");

			t = gf_fopen(szName, "wt");
			if (!t) return 5;	
		} 
		else {
			t = stdout;
		}
		mp4_ret=1;
		printf("Writing chapters into %s\n",szName);
	}
	else
	{
		mprint("No chapters information found!\n");
	}
		
	

	for (i=0; i<count; i++) {
		u64 chapter_time;
		const char *name;
		char szDur[20];
		gf_isom_get_chapter(f, 0, i+1, &chapter_time, &name);		
		fprintf(t, "CHAPTER%02d=%s\n", i+1, format_duration(chapter_time, 1000, szDur));
		fprintf(t, "CHAPTER%02dNAME=%s\n", i+1, name);
		
	}
	if (file) gf_fclose(t);
	return mp4_ret;
}
