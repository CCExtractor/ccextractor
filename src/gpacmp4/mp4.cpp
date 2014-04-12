#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <gpac/isomedia.h>
#include "../ccextractor.h"

void do_NAL (unsigned char *NALstart, LLONG NAL_length); // From avc_functions.cpp
void set_fts(void); // From timing.cpp

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

static struct {
	unsigned total;
	unsigned type[256];
}s_sei_stats;

static int process_avc_sample(u32 timescale, GF_AVCConfig* c, GF_ISOSample* s)
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
			do_NAL ((unsigned char *) &(s->data[i]) ,nal_length); 
		i += nal_length;
	} // outer for
	assert(i == s->dataLength);

	return status;
}
static int process_xdvb_track(const char* basename, GF_ISOFile* f, u32 track)
{
	u32 timescale, i, sample_count, last_sdi = 0;
	int status;
	if((sample_count = gf_isom_get_sample_count(f, track)) < 1){
		return 0;
	}

	timescale = gf_isom_get_media_timescale(f, track);

	status = 0;

	for(i = 0; i < sample_count; i++){
		u32 sdi;
		
		GF_ISOSample* s = gf_isom_get_sample(f, track, i + 1, &sdi);
		if (s!=NULL) {
	
		s32 signed_cts=(s32) s->CTS_Offset; // Convert from unsigned to signed. GPAC uses u32 but unsigned values are legal.
		current_pts=(LLONG )(s->DTS + signed_cts)*MPEG_CLOCK_FREQ/timescale ; // Convert frequency to official one
		if (pts_set==0)
			pts_set=1;
		set_fts();

			process_m2v ((unsigned char *) s->data,s->dataLength);			
			gf_isom_sample_del(&s);
		}

        int progress = (int) ((i*100) / sample_count);
        if (last_reported_progress != progress)
        {
            int cur_sec = (int) (get_fts() / 1000);
            activity_progress(progress, cur_sec/60, cur_sec%60);                    
            last_reported_progress = progress;
        }
	}
	int cur_sec = (int) (get_fts() / 1000);
	activity_progress(100, cur_sec/60, cur_sec%60);

	return status;
}

static int process_avc_track(const char* basename, GF_ISOFile* f, u32 track)
{
	u32 timescale, i, sample_count, last_sdi = 0;
	int status;
	GF_AVCConfig* c = NULL;
	
	if((sample_count = gf_isom_get_sample_count(f, track)) < 1){
		return 0;
	}

	timescale = gf_isom_get_media_timescale(f, track);

	status = 0;

	for(i = 0; i < sample_count; i++){
		u32 sdi;
		
		GF_ISOSample* s = gf_isom_get_sample(f, track, i + 1, &sdi);

		if(s != NULL){
			if(sdi != last_sdi){
				if(c != NULL){
					gf_odf_avc_cfg_del(c);
					c = NULL;
				}
				
				if((c = gf_isom_avc_config_get(f, track, sdi)) == NULL){
					gf_isom_sample_del(&s);
					status = -1;
					break;
				}

				last_sdi = sdi;
			}

			status = process_avc_sample(timescale, c, s);

			gf_isom_sample_del(&s);

			if(status != 0){
				break;
			}
		}

        int progress = (int) ((i*100) / sample_count);
        if (last_reported_progress != progress)
        {
            int cur_sec = (int) (get_fts() / 1000);
            activity_progress(progress, cur_sec/60, cur_sec%60);                    
            last_reported_progress = progress;
        }
	}
	int cur_sec = (int) (get_fts() / 1000);
	activity_progress(100, cur_sec/60, cur_sec%60);

	if(c != NULL){
		gf_odf_avc_cfg_del(c);
		c = NULL;
	}

	return status;
}

/*
	Here is application algorithm described in some C-like pseudo code:
		main(){
			media = open()
			for each track in media
				if track is AVC track
					for each sample in track
						for each NALU in sample
							send to avc.cpp for processing
			close(media)
		}

*/
int processmp4 (char *file)
{	
	GF_ISOFile* f;
	u32 i, j, track_count, avc_track_count, cc_track_count;
	
	mprint("opening \'%s\': ", file);

	if((f = gf_isom_open(file, GF_ISOM_OPEN_READ, NULL)) == NULL){
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
		if (type == GF_ISOM_MEDIA_CAPTIONS && subtype == GF_ISOM_SUBTYPE_C608)
			cc_track_count++;					
		if( type == GF_ISOM_MEDIA_VISUAL && subtype == GF_ISOM_SUBTYPE_AVC_H264)
			avc_track_count++;
	}

	for(i = 0; i < track_count; i++){
		const u32 type = gf_isom_get_media_type(f, i + 1);
		const u32 subtype = gf_isom_get_media_subtype(f, i + 1, 1);
	
		if ( type == GF_ISOM_MEDIA_VISUAL && subtype == GF_ISOM_SUBTYPE_XDVB)
		{
			if (cc_track_count && !ccx_options.mp4vidtrack)
				continue;
			if(process_xdvb_track(file, f, i + 1) != 0){
				mprint("error\n");
				return -3;
			}		}

		if( type == GF_ISOM_MEDIA_VISUAL && subtype == GF_ISOM_SUBTYPE_AVC_H264)
		{			
			if (cc_track_count && !ccx_options.mp4vidtrack)
				continue;
			GF_AVCConfig *cnf = gf_isom_avc_config_get(f,i+1,1);
			if (cnf!=NULL)
			{
				for (j=0; j<gf_list_count(cnf->sequenceParameterSets);j++)
				{
					GF_AVCConfigSlot* seqcnf=(GF_AVCConfigSlot* )gf_list_get(cnf->sequenceParameterSets,j);					
					do_NAL ((unsigned char *) seqcnf->data,seqcnf->size);				
				}
			}

			if(process_avc_track(file, f, i + 1) != 0){
				mprint("error\n");
				return -3;
			}

			
		}
		if (type == GF_ISOM_MEDIA_CAPTIONS && subtype == GF_ISOM_SUBTYPE_C608)
		{			
			if (avc_track_count && ccx_options.mp4vidtrack)
				continue;

			unsigned num_streams = gf_isom_get_sample_description_count (f,i+1);
			unsigned num_samples = gf_isom_get_sample_count (f,i+1);
			
			u32 ProcessingStreamDescriptionIndex = 0; // Current track we are processing, 0 = we don't know yet
			u32 timescale = gf_isom_get_media_timescale(f,i+1);
			// u64 duration = gf_isom_get_media_duration(f,i+1);
			/* mprint ("%u streams\n",num_streams);
			mprint ("%u sample counts\n",num_samples);
			mprint ("%u timescale\n",(unsigned) timescale);
			mprint ("%u duration\n",(unsigned) duration); */
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
				// mprint ("Data length: %lu\n",sample->dataLength);
				const LLONG timestamp = (LLONG )((sample->DTS + sample->CTS_Offset) * 1000) / timescale;
				current_pts=(LLONG )(sample->DTS + sample->CTS_Offset)*MPEG_CLOCK_FREQ/timescale ; // Convert frequency to official one
				if (pts_set==0)
					pts_set=1;
				set_fts();

				// Apparently the first 4 bytes are the sample length, and then comes 'cdat', and then the data itself
				if (sample->dataLength>8 && strncmp (sample->data+4, "cdat", 4)==0)
				{
					// dump (256,( unsigned char *) sample->data+8,sample->dataLength-8,0, 1);				
					process608 ((const unsigned char *) sample->data+8,sample->dataLength-8,&wbout1);
				}
				int progress = (int) ((k*100) / num_samples);
				if (last_reported_progress != progress)
				{
					int cur_sec = (int) (get_fts() / 1000);
					activity_progress(progress, cur_sec/60, cur_sec%60);                    
					last_reported_progress = progress;
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

		if(avc_track_count == 0){
		mprint("Found no AVC track(s). ", file);
	}else{
		mprint("Found %d AVC track(s). ", avc_track_count);
	}
	if (cc_track_count)
		mprint ("Found %d CC track(s).\n", cc_track_count);
	else
		mprint ("found no dedicated CC track(s).\n");

	return 0;
}
