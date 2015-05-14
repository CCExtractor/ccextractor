#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <gpac/isomedia.h>
#include "lib_ccx.h"
#include "utility.h"
#include "ccx_encoders_common.h"
#include "ccx_common_option.h"
#include "ccx_mp4.h"

void do_NAL (struct lib_ccx_ctx *ctx, unsigned char *NALstart, LLONG NAL_length, struct cc_subtitle *sub);
void set_fts(void); // From timing.c

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
	current_pts=(LLONG )(s->DTS + signed_cts)*MPEG_CLOCK_FREQ/timescale ; // Convert frequency to official one

	if (pts_set==0)
		pts_set=1;
	set_fts();

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
			do_NAL (ctx, (unsigned char *) &(s->data[i]) ,nal_length, sub);
		i += nal_length;
	} // outer for
	assert(i == s->dataLength);

	return status;
}
static int process_xdvb_track(struct lib_ccx_ctx *ctx, const char* basename, GF_ISOFile* f, u32 track, struct cc_subtitle *sub)
{
	u32 timescale, i, sample_count;

	int status;
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
			current_pts=(LLONG )(s->DTS + signed_cts)*MPEG_CLOCK_FREQ/timescale ; // Convert frequency to official one
			if (pts_set==0)
				pts_set=1;
			set_fts();

			process_m2v (ctx, (unsigned char *) s->data,s->dataLength, sub);
			gf_isom_sample_del(&s);
		}

		int progress = (int) ((i*100) / sample_count);
		if (ctx->last_reported_progress != progress)
		{
			int cur_sec = (int) (get_fts() / 1000);
			activity_progress(progress, cur_sec/60, cur_sec%60);
			ctx->last_reported_progress = progress;
		}
	}
	int cur_sec = (int) (get_fts() / 1000);
	activity_progress(100, cur_sec/60, cur_sec%60);

	return status;
}

static int process_avc_track(struct lib_ccx_ctx *ctx, const char* basename, GF_ISOFile* f, u32 track, struct cc_subtitle *sub)
{
	u32 timescale, i, sample_count, last_sdi = 0;
	int status;
	GF_AVCConfig* c = NULL;
	
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
			int cur_sec = (int) (get_fts() / 1000);
			activity_progress(progress, cur_sec/60, cur_sec%60);
			ctx->last_reported_progress = progress;
		}
	}
	int cur_sec = (int) (get_fts() / 1000);
	activity_progress(100, cur_sec/60, cur_sec%60);

	if(c != NULL)
	{
		gf_odf_avc_cfg_del(c);
		c = NULL;
	}

	return status;
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

/*
	Here is application algorithm described in some C-like pseudo code:
		main(){
			media = open()
			for each track in media
				if track is AVC track
					for each sample in track
						for each NALU in sample
							send to avc.c for processing
			close(media)
		}

*/
int processmp4 (struct lib_ccx_ctx *ctx,struct ccx_s_mp4Cfg *cfg, char *file,void *enc_ctx)
{	
	GF_ISOFile* f;
	u32 i, j, track_count, avc_track_count, cc_track_count;
	struct cc_subtitle dec_sub;

	memset(&dec_sub,0,sizeof(dec_sub));
	mprint("opening \'%s\': ", file);
#ifdef MP4_DEBUG
	gf_log_set_tool_level(GF_LOG_CONTAINER,GF_LOG_DEBUG);
#endif

	if((f = gf_isom_open(file, GF_ISOM_OPEN_READ, NULL)) == NULL)
	{
		mprint("failed to open\n");
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
		if ((type == GF_ISOM_MEDIA_CAPTIONS && subtype == GF_ISOM_SUBTYPE_C608) ||
			(type == GF_ISOM_MEDIA_CAPTIONS && subtype == GF_ISOM_SUBTYPE_C708))
			cc_track_count++;
		if (type == GF_ISOM_MEDIA_VISUAL && subtype == GF_ISOM_SUBTYPE_AVC_H264)
			avc_track_count++;
	}

	mprint("mp4: found %u tracks: %u avc and %u cc\n", track_count, avc_track_count, cc_track_count);

	for(i = 0; i < track_count; i++)
	{
		const u32 type = gf_isom_get_media_type(f, i + 1);
		const u32 subtype = gf_isom_get_media_subtype(f, i + 1, 1);

		if ( type == GF_ISOM_MEDIA_VISUAL && subtype == GF_ISOM_SUBTYPE_XDVB)
		{
			if (cc_track_count && !cfg->mp4vidtrack)
				continue;
			if(process_xdvb_track(ctx, file, f, i + 1, &dec_sub) != 0)
			{
				mprint("error\n");
				return -3;
			}
			if(dec_sub.got_output)
			{
				encode_sub(enc_ctx, &dec_sub);
				dec_sub.got_output = 0;
			}
		}

		if( type == GF_ISOM_MEDIA_VISUAL && subtype == GF_ISOM_SUBTYPE_AVC_H264)
		{			
			if (cc_track_count && !cfg->mp4vidtrack)
				continue;
			GF_AVCConfig *cnf = gf_isom_avc_config_get(f,i+1,1);
			if (cnf!=NULL)
			{
				for (j=0; j<gf_list_count(cnf->sequenceParameterSets);j++)
				{
					GF_AVCConfigSlot* seqcnf=(GF_AVCConfigSlot* )gf_list_get(cnf->sequenceParameterSets,j);
					do_NAL (ctx, (unsigned char *) seqcnf->data, seqcnf->size, &dec_sub);
				}
			}

			if(process_avc_track(ctx, file, f, i + 1, &dec_sub) != 0)
			{
				mprint("error\n");
				return -3;
			}
			if(dec_sub.got_output)
			{
				encode_sub(enc_ctx, &dec_sub);
				dec_sub.got_output = 0;
			}

			
		}
		if (type == GF_ISOM_MEDIA_CAPTIONS &&
				(subtype == GF_ISOM_SUBTYPE_C608 || subtype == GF_ISOM_SUBTYPE_C708))
		{
			if (avc_track_count && cfg->mp4vidtrack)
				continue;

#ifdef MP4_DEBUG
			unsigned num_streams = gf_isom_get_sample_description_count (f,i+1);
#endif
			unsigned num_samples = gf_isom_get_sample_count (f,i+1);

			u32 ProcessingStreamDescriptionIndex = 0; // Current track we are processing, 0 = we don't know yet
			u32 timescale = gf_isom_get_media_timescale(f,i+1);
#ifdef MP4_DEBUG
			u64 duration = gf_isom_get_media_duration(f,i+1);
			mprint ("%u streams\n",num_streams);
			mprint ("%u sample counts\n",num_samples);
			mprint ("%u timescale\n",(unsigned) timescale);
			mprint ("%u duration\n",(unsigned) duration);
#endif
			for (unsigned k = 0; k <num_samples; k++)
			{
				u32 StreamDescriptionIndex;
				GF_ISOSample *sample= gf_isom_get_sample(f, i+1, k+1, &StreamDescriptionIndex);
				if (ProcessingStreamDescriptionIndex && ProcessingStreamDescriptionIndex!=StreamDescriptionIndex)
				{
					mprint ("This sample seems to have more than one track. This isn't supported yet.\n");
					mprint ("Submitting this video file will help add support to this case.\n");
					break;
				}
				if (!ProcessingStreamDescriptionIndex)
						ProcessingStreamDescriptionIndex=StreamDescriptionIndex;
				if (sample==NULL)
					continue;
#ifdef DEBUG
				 mprint ("Data length: %lu\n",sample->dataLength);
				const LLONG timestamp = (LLONG )((sample->DTS + sample->CTS_Offset) * 1000) / timescale;
#endif
				current_pts=(LLONG )(sample->DTS + sample->CTS_Offset)*MPEG_CLOCK_FREQ/timescale ; // Convert frequency to official one
				if (pts_set==0)
					pts_set=1;
				set_fts();

				int atomStart = 0;
				// process Atom by Atom
				while (atomStart < sample->dataLength)
				{
					char *data = sample->data + atomStart;
					unsigned int atomLength = RB32(data);
					if (atomLength < 8 || atomLength > sample->dataLength)
					{
						mprint ("Invalid atom length. Atom length: %u,  should be: %u\n", atomLength, sample->dataLength);
						break;
					}
#ifdef MP4_DEBUG
					dump(256, (unsigned char *)data, atomLength - 8, 0, 1);
#endif
					data += 4;
					int is_ccdp = !strncmp(data, "ccdp", 4);

					if (!strncmp(data, "cdat", 4) || !strncmp(data, "cdt2", 4) || is_ccdp)
					{
						if (subtype == GF_ISOM_SUBTYPE_C708)
						{
							if (!is_ccdp)
							{
								mprint("Your video file seems to be an interesting sample for us\n");
								mprint("We haven't met c708 subtitle not in a \'ccdp\' atom before\n");
								mprint("Please, report\n");
								break;
							}

							unsigned int cc_count;
							data += 4;
							unsigned char *cc_data = ccdp_find_data((unsigned char *) data, sample->dataLength - 8, &cc_count);

							if (!cc_data)
							{
								dbg_print(CCX_DMT_PARSE, "mp4-708: no cc data found in ccdp\n");
								break;
							}

							do_cea708 = 1;
							unsigned char temp[4];
							for (int cc_i = 0; cc_i < cc_count; cc_i++, cc_data += 3)
							{
								unsigned char cc_info = cc_data[0];
								unsigned char cc_valid = (unsigned char) ((cc_info & 4) >> 2);
								unsigned char cc_type = (unsigned char) (cc_info & 3);

								if (cc_info == CDP_SECTION_SVC_INFO || cc_info == CDP_SECTION_FOOTER)
								{
									dbg_print(CCX_DMT_PARSE, "mp4-708: premature end of sample (0x73 or 0x74)\n");
									break;
								}

								if ((cc_info == 0xFA || cc_info == 0xFC || cc_info == 0xFD)
									&& (cc_data[1] & 0x7F) == 0 && (cc_data[2] & 0x7F) == 0)
								{
									dbg_print(CCX_DMT_PARSE, "mp4-708: skipped (zero cc data)\n");
									continue;
								}

								temp[0] = cc_valid;
								temp[1] = cc_type;
								temp[2] = cc_data[1];
								temp[3] = cc_data[2];

								if (cc_type < 2)
								{
									dbg_print(CCX_DMT_PARSE, "mp4-708: atom skipped (cc_type < 2)\n");
									continue;
								}
								do_708(ctx->dec_ctx, (unsigned char *) temp, 4);
								cb_708++;
							}
							atomStart = sample->dataLength;
						}
						else //subtype == GF_ISOM_SUBTYPE_C608
						{
							if (is_ccdp)
							{
								mprint("Your video file seems to be an interesting sample for us\n");
								mprint("We haven't met c608 subtitle in a \'ccdp\' atom before\n");
								mprint("Please, report\n");
								break;
							}

							int ret = 0;
							int len = atomLength - 8;
							data += 4;

							do {
								ret = process608((unsigned char *) data, len, ctx->dec_ctx->context_cc608_field_1,
												 &dec_sub);
								len -= ret;
								data += ret;
								if (dec_sub.got_output) {
									encode_sub(enc_ctx, &dec_sub);
									dec_sub.got_output = 0;
								}
							} while (len > 0);
						}
					}
					atomStart += atomLength;
				}

				// End of change
				int progress = (int) ((k*100) / num_samples);
				if (ctx->last_reported_progress != progress)
				{
					int cur_sec = (int) (get_fts() / 1000);
					activity_progress(progress, cur_sec/60, cur_sec%60);
					ctx->last_reported_progress = progress;
				}
			}
			int cur_sec = (int) (get_fts() / 1000);
			activity_progress(100, cur_sec/60, cur_sec%60);
		}
	}

	mprint("\nclosing media: ");

	gf_isom_close(f);
	f = NULL;
	mprint ("ok\n");

	if(avc_track_count == 0)
	{
		mprint("Found no AVC track(s). ", file);
	}
	else
	{
		mprint("Found %d AVC track(s). ", avc_track_count);
	}
	if (cc_track_count)
		mprint ("Found %d CC track(s).\n", cc_track_count);
	else
		mprint ("found no dedicated CC track(s).\n");

	ctx->freport.mp4_cc_track_cnt = cc_track_count;

	return 0;
}
