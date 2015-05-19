#include "lib_ccx.h"
#include "ccx_common_option.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "dvb_subtitle_decoder.h"
#include "ccx_encoders_common.h"
// IMPORTED TRASH INFO, REMOVE
extern long num_nal_unit_type_7;
extern long num_vcl_hrd;
extern long num_nal_hrd;
extern long num_jump_in_frames;
extern long num_unexpected_sei_length;

/* General video information */
 unsigned current_hor_size = 0;
unsigned current_vert_size = 0;
unsigned current_aspect_ratio = 0;
unsigned current_frame_rate = 4; // Assume standard fps, 29.97
unsigned rollover_bits = 0; // The PTS rolls over every 26 hours and that can happen in the middle of a stream.
LLONG result; // Number of bytes read/skipped in last read operation
int end_of_file=0; // End of file?


const static unsigned char DO_NOTHING[] = {0x80, 0x80};
LLONG inbuf = 0; // Number of bytes loaded in buffer
int ccx_bufferdatatype = CCX_PES; // Can be RAW, PES, H264 or Hauppage

// Remember if the last header was valid. Used to suppress too much output
// and the expected unrecognized first header for TiVo files.
int strangeheader=0;

unsigned char *filebuffer;
LLONG filebuffer_start; // Position of buffer start relative to file
int filebuffer_pos; // Position of pointer relative to buffer start
int bytesinbuffer; // Number of bytes we actually have on buffer
extern void *ccx_dvb_context;

// Program stream specific data grabber
LLONG ps_getmoredata(struct lib_ccx_ctx *ctx)
{
	int enough = 0;
	int payload_read = 0;

	static unsigned vpesnum=0;

	unsigned char nextheader[512]; // Next header in PS
	int falsepack=0;

	// Read and return the next video PES payload
	do
	{
		if (BUFSIZE-inbuf<500)
		{
			mprint("Less than 500 left\n");
			enough=1; // Stop when less than 500 bytes are left in buffer
		}
		else
		{
			buffered_read(ctx, nextheader, 6);
			ctx->past+=result;
			if (result!=6)
			{
				// Consider this the end of the show.
				end_of_file=1;
				break;
			}

			// Search for a header that is not a picture header (nextheader[3]!=0x00)
			while ( !(nextheader[0]==0x00 && nextheader[1]==0x00
						&& nextheader[2]==0x01 && nextheader[3]!=0x00) )
			{
				if( !strangeheader )
				{
					mprint ("\nNot a recognized header. Searching for next header.\n");
					dump (CCX_DMT_GENERIC_NOTICES, nextheader,6,0,0);
					// Only print the message once per loop / unrecognized header
					strangeheader = 1;
				}

				unsigned char *newheader;
				// The amount of bytes read into nextheader by the buffered_read above
				int hlen = 6;
				// Find first 0x00
				// If there is a 00 in the first element we need to advance
				// one step as clearly bytes 1,2,3 are wrong
				newheader = (unsigned char *) memchr (nextheader+1, 0, hlen-1);
				if (newheader != NULL )
				{
					int atpos = newheader-nextheader;

					memmove (nextheader,newheader,(size_t)(hlen-atpos));
					buffered_read(ctx, nextheader+(hlen-atpos),atpos);
					ctx->past+=result;
					if (result!=atpos)
					{
						end_of_file=1;
						break;
					}
				}
				else
				{
					buffered_read(ctx, nextheader, hlen);
					ctx->past+=result;
					if (result!=hlen)
					{
						end_of_file=1;
						break;
					}
				}
			}
			if (end_of_file)
			{
				// No more headers
				break;
			}
			// Found 00-00-01 in nextheader, assume a regular header
			strangeheader=0;

			// PACK header
			if ( nextheader[3]==0xBA)
			{
				dbg_print(CCX_DMT_VERBOSE, "PACK header\n");
				buffered_read(ctx, nextheader+6,8);
				ctx->past+=result;
				if (result!=8)
				{
					// Consider this the end of the show.
					end_of_file=1;
					break;
				}

				if ( (nextheader[4]&0xC4)!=0x44 || !(nextheader[6]&0x04)
						|| !(nextheader[8]&0x04) || !(nextheader[9]&0x01)
						|| (nextheader[12]&0x03)!=0x03 )
				{
					// broken pack header
					falsepack=1;
				}
				// We don't need SCR/SCR_ext
				int stufflen=nextheader[13]&0x07;

				if (falsepack)
				{
					mprint ("Warning: Defective Pack header\n");
				}

				// If not defect, load stuffing
				buffered_skip (ctx, (int) stufflen);
				ctx->past+=stufflen;
				// fake a result value as something was skipped
				result=1;
				continue;
			}
			// Some PES stream
			else if (nextheader[3]>=0xBB && nextheader[3]<=0xDF)
			{
				// System header
				// nextheader[3]==0xBB
				// 0xBD Private 1
				// 0xBE PAdding
				// 0xBF Private 2
				// 0xC0-0DF audio

				unsigned headerlen=nextheader[4]<<8 | nextheader[5];

				dbg_print(CCX_DMT_VERBOSE, "non Video PES (type 0x%2X) - len %u\n",
						nextheader[3], headerlen);

				// The 15000 here is quite arbitrary, the longest packages I
				// know of are 12302 bytes (Private 1 data in RTL recording).
				if ( headerlen > 15000 )
				{
					mprint("Suspicious non Video PES (type 0x%2X) - len %u\n",
							nextheader[3], headerlen);
					mprint("Do not skip over, search for next.\n");
					headerlen = 2;
				}

				// Skip over it
				buffered_skip (ctx, (int) headerlen);
				ctx->past+=headerlen;
				// fake a result value as something was skipped
				result=1;

				continue;
			}
			// Read the next video PES
			else if ((nextheader[3]&0xf0)==0xe0)
			{
				int hlen; // Dummy variable, unused
				int peslen = read_video_pes_header(ctx, nextheader, &hlen, 0);
				if (peslen < 0)
				{
					end_of_file=1;
					break;
				}

				vpesnum++;
				dbg_print(CCX_DMT_VERBOSE, "PES video packet #%u\n", vpesnum);


				int want = (int) ((BUFSIZE-inbuf)>peslen ? peslen : (BUFSIZE-inbuf));

				if (want != peslen) {
					fatal(EXIT_BUFFER_FULL, "Oh Oh, PES longer than remaining buffer space\n");
				}
				if (want == 0) // Found package with header but without payload
				{
					continue;
				}

				buffered_read (ctx, ctx->buffer+inbuf, want);
				ctx->past=ctx->past+result;
				if (result>0) {
					payload_read+=(int) result;
				}
				inbuf+=result;

				if (result!=want) { // Not complete - EOF
					end_of_file=1;
					break;
				}
				enough = 1; // We got one PES

			} else {
				// If we are here this is an unknown header type
				mprint("Unknown header %02X\n", nextheader[3]);
				strangeheader=1;
			}
		}
	}
	while (result!=0 && !enough && BUFSIZE!=inbuf);

	dbg_print(CCX_DMT_VERBOSE, "PES data read: %d\n", payload_read);

	return payload_read;
}


// Returns number of bytes read, or zero for EOF
LLONG general_getmoredata(struct lib_ccx_ctx *ctx)
{
	int bytesread = 0;
	int want;

	do
	{
		want = (int) (BUFSIZE-inbuf);
		buffered_read (ctx, ctx->buffer+inbuf,want); // This is a macro.
		// 'result' HAS the number of bytes read
		ctx->past=ctx->past+result;
		inbuf+=result;
		bytesread+=(int) result;
	} while (result!=0 && result!=want);
	return bytesread;
}

#ifdef WTV_DEBUG
// Hexadecimal dump process
void processhex (struct lib_ccx_ctx *ctx, char *filename)
{
	size_t max=(size_t) ctx->inputsize+1; // Enough for the whole thing. Hex dumps are small so we can be lazy here
	char *line=(char *) malloc (max);
	/* const char *mpeg_header="00 00 01 b2 43 43 01 f8 "; // Always present */
	FILE *fr = fopen (filename, "rt");
	unsigned char *bytes=NULL;
	unsigned byte_count=0;
	int warning_shown=0;
	while(fgets(line, max-1, fr) != NULL)
	{
		char *c1, *c2=NULL; // Positions for first and second colons
		/* int len; */
		long timing;
		if (line[0]==';') // Skip comments
			continue;
		c1=strchr (line,':');
		if (c1) c2=strchr (c1+1,':');
		if (!c2) // Line doesn't contain what we want
			continue;
		*c1=0;
		*c2=0;
		/* len=atoi (line); */
		timing=atol (c1+2)*(MPEG_CLOCK_FREQ/1000);
		current_pts=timing;
		if (pts_set==0)
			pts_set=1;
		set_fts();
		c2++;
/*
		if (strlen (c2)==8)
		{
			unsigned char high1=c2[1];
			unsigned char low1=c2[2];
			int value1=hex2int (high1,low1);
			unsigned char high2=c2[4];
			unsigned char low2=c2[5];
			int value2=hex2int (high2,low2);
			buffer[0]=value1;
			buffer[1]=value2;
			inbuf=2;
			process_raw();
			continue;
		}
		if (strlen (c2)<=(strlen (mpeg_header)+1))
			continue; */
		c2++; // Skip blank
		/*
		if (strncmp (c2,mpeg_header,strlen (mpeg_header))) // No idea how to deal with this.
		{
			if (!warning_shown)
			{
				warning_shown=1;
				mprint ("\nWarning: This file contains data I can't process: Please submit .hex for analysis!\n");
			}
			continue;
		}
		// OK, seems like a decent chunk of CCdata.
		c2+=strlen (mpeg_header);
		*/
		byte_count=strlen (c2)/3;
		/*
		if (atoi (line)!=byte_count+strlen (mpeg_header)/3) // Number of bytes reported don't match actual contents
			continue;
			*/
		if (atoi (line)!=byte_count) // Number of bytes reported don't match actual contents
			continue;
		if (!byte_count) // Nothing to get from this line except timing info, already done.
			continue;
		bytes=(unsigned char *) malloc (byte_count);
		if (!bytes)
			fatal (EXIT_NOT_ENOUGH_MEMORY, "Out of memory.\n");
		unsigned char *bytes=(unsigned char *) malloc (byte_count);
		for (unsigned i=0;i<byte_count;i++)
		{
			unsigned char high=c2[0];
			unsigned char low=c2[1];
			int value=hex2int (high,low);
			if (value==-1)
				fatal (EXIT_FAILURE, "Incorrect format, unexpected non-hex string.");
			bytes[i]=value;
			c2+=3;
		}
		memcpy (ctx->buffer, bytes, byte_count);
		inbuf=byte_count;
		process_raw();
		continue;
		// New wtv format, everything else hopefully obsolete

		int ok=0; // Were we able to process the line?
		// Attempt to detect how the data is encoded.
		// Case 1 (seen in all elderman's samples):
		// 18 : 467 : 00 00 01 b2 43 43 01 f8 03 42 ff fd 54 80 fc 94 2c ff
		// Always 03 after header, then something unknown (seen 42, 43, c2, c3...),
		// then ff, then data with field info, and terminated with ff.
		if (byte_count>3 && bytes[0]==0x03 &&
			bytes[2]==0xff && bytes[byte_count-1]==0xff)
		{
			ok=1;
			for (unsigned i=3; i<byte_count-2; i+=3)
			{
				inbuf=3;
				ctx->buffer[0]=bytes[i];
				ctx->buffer[1]=bytes[i+1];
				ctx->buffer[2]=bytes[i+2];
				process_raw_with_field();
			}
		}
		// Case 2 (seen in P2Pfiend samples):
		// Seems to match McPoodle's descriptions on DVD encoding
		else
		{
			unsigned char magic=bytes[0];
			/* unsigned extra_field_flag=magic&1; */
			unsigned caption_count=((magic>>1)&0x1F);
			unsigned filler=((magic>>6)&1);
			/* unsigned pattern=((magic>>7)&1); */
			int always_ff=1;
			int current_field=0;
			if (filler==0 && caption_count*6==byte_count-1) // Note that we are ignoring the extra field for now...
			{
				ok=1;
				for (unsigned i=1; i<byte_count-2; i+=3)
					if (bytes[i]!=0xff)
					{
						// If we only find FF in the first byte then either there's only field 1 data, OR
						// there's alternating field 1 and field 2 data. Don't know how to tell apart. For now
						// let's assume that always FF means alternating.
						always_ff=0;
						break;
					}

				for (unsigned i=1; i<byte_count-2; i+=3)
				{
					inbuf=3;
					if (always_ff) // Try to tell apart the fields based on the pattern field.
					{
						ctx->buffer[0]=current_field | 4; // | 4 to enable the 'valid' bit
						current_field = !current_field;
					}
					else
						ctx->buffer[0]=bytes[i];

					ctx->buffer[1]=bytes[i+1];
					ctx->buffer[2]=bytes[i+2];
					process_raw_with_field();
				}
			}
		}
		if (!ok && !warning_shown)
		{
			warning_shown=1;
			mprint ("\nWarning: This file contains data I can't process: Please submit .hex for analysis!\n");
		}
		free (bytes);
	}
	fclose(fr);
}
#endif
// Raw file process
void raw_loop (struct lib_ccx_ctx *ctx, void *enc_ctx)
{
	LLONG got;
	LLONG processed;
	struct cc_subtitle dec_sub;

	current_pts = 90; // Pick a valid PTS time
	pts_set = 1;
	set_fts(); // Now set the FTS related variables
	memset(&dec_sub, 0, sizeof(dec_sub));
	dbg_print(CCX_DMT_VIDES, "PTS: %s (%8u)",
			print_mstime(current_pts/(MPEG_CLOCK_FREQ/1000)),
			(unsigned) (current_pts));
	dbg_print(CCX_DMT_VIDES, "  FTS: %s\n", print_mstime(get_fts()));

	do
	{
		inbuf=0;

		got = general_getmoredata(ctx);

		if (got == 0) // Shortcircuit if we got nothing to process
			break;

		processed=process_raw(ctx, &dec_sub);
		if (dec_sub.got_output)
		{
			encode_sub(enc_ctx,&dec_sub);
			dec_sub.got_output = 0;
		}

		int ccblocks = cb_field1;
		current_pts += cb_field1*1001/30*(MPEG_CLOCK_FREQ/1000);
		set_fts(); // Now set the FTS related variables including fts_max

		dbg_print(CCX_DMT_VIDES, "PTS: %s (%8u)",
				print_mstime(current_pts/(MPEG_CLOCK_FREQ/1000)),
				(unsigned) (current_pts));
		dbg_print(CCX_DMT_VIDES, "  FTS: %s incl. %d CB\n",
				print_mstime(get_fts()), ccblocks);

		if (processed<got)
		{
			mprint ("BUG BUG\n");
		}
	} while (inbuf);
}

/* Process inbuf bytes in buffer holding raw caption data (three byte packets, the first being the field).
 * The number of processed bytes is returned. */
LLONG process_raw_with_field (struct lib_ccx_ctx *ctx, struct cc_subtitle *sub)
{
	unsigned char data[3];
	struct lib_cc_decode *dec_ctx = NULL;
	dec_ctx = ctx->dec_ctx;
	data[0]=0x04; // Field 1
	current_field=1;

	for (unsigned long i=0; i<inbuf; i=i+3)
	{
		if ( !dec_ctx->saw_caption_block && *(ctx->buffer+i)==0xff && *(ctx->buffer+i+1)==0xff)
		{
			// Skip broadcast header
		}
		else
		{
			data[0]=ctx->buffer[i];
			data[1]=ctx->buffer[i+1];
			data[2]=ctx->buffer[i+2];

			// do_cb increases the cb_field1 counter so that get_fts()
			// is correct.
			do_cb(dec_ctx, data, sub);
		}
	}
	return inbuf;
}


/* Process inbuf bytes in buffer holding raw caption data (two byte packets).
 * The number of processed bytes is returned. */
LLONG process_raw (struct lib_ccx_ctx *ctx, struct cc_subtitle *sub)
{
	unsigned char data[3];
	struct lib_cc_decode *dec_ctx = NULL;
	dec_ctx = ctx->dec_ctx;
	data[0]=0x04; // Field 1
	current_field=1;

	for (unsigned long i=0; i<inbuf; i=i+2)
	{
		if ( !dec_ctx->saw_caption_block && *(ctx->buffer+i)==0xff && *(ctx->buffer+i+1)==0xff)
		{
			// Skip broadcast header
		}
		else
		{
			data[1]=ctx->buffer[i];
			data[2]=ctx->buffer[i+1];

			// do_cb increases the cb_field1 counter so that get_fts()
			// is correct.
			do_cb(dec_ctx, data, sub);
		}
	}
	return inbuf;
}


void general_loop(struct lib_ccx_ctx *ctx, void *enc_ctx)
{
	LLONG overlap=0;
	LLONG pos = 0; /* Current position in buffer */
	struct cc_subtitle dec_sub;
	struct lib_cc_decode *dec_ctx = NULL;
	dec_ctx = ctx->dec_ctx;
	dec_ctx->wbout1 = (struct ccx_s_write*)&ctx->wbout1 ; 
	dec_ctx->wbout2 = (struct ccx_s_write*)&ctx->wbout2 ;
	inbuf = 0; // No data yet

	end_of_file = 0;
	current_picture_coding_type = CCX_FRAME_TYPE_RESET_OR_UNKNOWN;
	memset(&dec_sub, 0,sizeof(dec_sub));
	while (!end_of_file && !dec_ctx->processed_enough)
	{
		/* Get rid of the bytes we already processed */
		overlap=inbuf-pos;
		if ( pos != 0 ) {
			// Only when needed as memmove has been seen crashing
			// for dest==source and n >0
			memmove (ctx->buffer,ctx->buffer+pos,(size_t) (inbuf-pos));
			inbuf-=pos;
		}
		pos = 0;

		// GET MORE DATA IN BUFFER
		LLONG i;
		position_sanity_check();
		switch (ctx->stream_mode)
		{
			case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
				i = general_getmoredata(ctx);
				break;
			case CCX_SM_TRANSPORT:
				i = ts_getmoredata(ctx);
				break;
			case CCX_SM_PROGRAM:
				i = ps_getmoredata(ctx);
				break;
			case CCX_SM_ASF:
				i = asf_getmoredata(ctx);
				break;
			case CCX_SM_WTV:
				i = wtv_getmoredata(ctx);
				break;
			default:
				fatal(CCX_COMMON_EXIT_BUG_BUG, "Impossible stream_mode");
		}

		position_sanity_check();
		if (ctx->fh_out_elementarystream!=NULL)
			fwrite (ctx->buffer+overlap,1,(size_t) (inbuf-overlap),ctx->fh_out_elementarystream);

		if (i==0)
		{
			end_of_file = 1;
			memset (ctx->buffer+inbuf, 0, (size_t) (BUFSIZE-inbuf)); /* Clear buffer at the end */
		}

		if (inbuf == 0)
		{
			/* Done: Get outta here */
			break;
		}

		LLONG got; // Means 'consumed' from buffer actually

		static LLONG last_pts = 0x01FFFFFFFFLL;

		if (ctx->hauppauge_mode)
		{
			got = process_raw_with_field(ctx, &dec_sub);
			if (pts_set)
				set_fts(); // Try to fix timing from TS data
		}
		else if(ccx_bufferdatatype == CCX_DVB_SUBTITLE)
		{
			dvbsub_decode(ccx_dvb_context, ctx->buffer + 2, inbuf, &dec_sub);
			set_fts();
			got = inbuf;
		}
		else if (ccx_bufferdatatype == CCX_PES)
		{
			got = process_m2v (ctx, ctx->buffer, inbuf,&dec_sub);
		}
		else if (ccx_bufferdatatype == CCX_TELETEXT)
		{
			// Dispatch to Petr Kutalek 's telxcc.
			tlt_process_pes_packet (ctx, ctx->buffer, (uint16_t) inbuf);
			got = inbuf;
		}
		else if (ccx_bufferdatatype == CCX_PRIVATE_MPEG2_CC)
		{
			got = inbuf; // Do nothing. Still don't know how to process it
		}
		else if (ccx_bufferdatatype == CCX_RAW) // Raw two byte 608 data from DVR-MS/ASF
		{
			// The asf_getmoredata() loop sets current_pts when possible
			if (pts_set == 0)
			{
				mprint("DVR-MS/ASF file without useful time stamps - count blocks.\n");
				// Otherwise rely on counting blocks
				current_pts = 12345; // Pick a valid PTS time
				pts_set = 1;
			}

			if (current_pts != last_pts)
			{
				// Only initialize the FTS values and reset the cb
				// counters when the PTS is different. This happens frequently
				// with ASF files.

				if (min_pts==0x01FFFFFFFFLL)
				{
					// First call
					fts_at_gop_start = 0;
				}
				else
					fts_at_gop_start = get_fts();

				frames_since_ref_time = 0;
				set_fts();

				last_pts = current_pts;
			}

			dbg_print(CCX_DMT_VIDES, "PTS: %s (%8u)",
					print_mstime(current_pts/(MPEG_CLOCK_FREQ/1000)),
					(unsigned) (current_pts));
			dbg_print(CCX_DMT_VIDES, "  FTS: %s\n", print_mstime(get_fts()));

			got = process_raw(ctx, &dec_sub);
		}
		else if (ccx_bufferdatatype == CCX_H264) // H.264 data from TS file
		{
			got = process_avc(ctx, ctx->buffer, inbuf,&dec_sub);
		}
		else
			fatal(CCX_COMMON_EXIT_BUG_BUG, "Unknown data type!");

		if (got>inbuf)
		{
			mprint ("BUG BUG\n");
		}
		pos+=got;

		if (ctx->live_stream)
		{
			int cur_sec = (int) (get_fts() / 1000);
			int th=cur_sec/10;
			if (ctx->last_reported_progress!=th)
			{
				activity_progress (-1,cur_sec/60, cur_sec%60);
				ctx->last_reported_progress = th;
			}
		}
		else
		{
			if (ctx->total_inputsize>255) // Less than 255 leads to division by zero below.
			{
				int progress = (int) ((((ctx->total_past+ctx->past)>>8)*100)/(ctx->total_inputsize>>8));
				if (ctx->last_reported_progress != progress)
				{
					LLONG t=get_fts();
					if (!t && ctx->global_timestamp_inited)
						t=ctx->global_timestamp-ctx->min_global_timestamp;
					int cur_sec = (int) (t / 1000);
					activity_progress(progress, cur_sec/60, cur_sec%60);
					ctx->last_reported_progress = progress;
				}
			}
		}
		if (dec_sub.got_output)
		{
			encode_sub(enc_ctx,&dec_sub);
			dec_sub.got_output = 0;
		}
		position_sanity_check();
	}
	// Flush remaining HD captions
	if (has_ccdata_buffered)
		process_hdcc(ctx, &dec_sub);

	if (ctx->total_past!=ctx->total_inputsize && ctx->binary_concat && !dec_ctx->processed_enough)
	{
		mprint("\n\n\n\nATTENTION!!!!!!\n");
		mprint("Processing of %s %d ended prematurely %lld < %lld, please send bug report.\n\n",
				ctx->inputfile[ctx->current_file], ctx->current_file, ctx->past, ctx->inputsize);
	}
	mprint ("\nNumber of NAL_type_7: %ld\n",num_nal_unit_type_7);
	mprint ("Number of VCL_HRD: %ld\n",num_vcl_hrd);
	mprint ("Number of NAL HRD: %ld\n",num_nal_hrd);
	mprint ("Number of jump-in-frames: %ld\n",num_jump_in_frames);
	mprint ("Number of num_unexpected_sei_length: %ld", num_unexpected_sei_length);
}

// Raw caption with FTS file process
void rcwt_loop(struct lib_ccx_ctx *ctx, void *enc_ctx)
{
	static unsigned char *parsebuf;
	static long parsebufsize = 1024;
	struct cc_subtitle dec_sub;
	struct lib_cc_decode *dec_ctx = NULL;
	dec_ctx = ctx->dec_ctx;

	memset(&dec_sub, 0,sizeof(dec_sub));
	// As BUFSIZE is a macro this is just a reminder
	if (BUFSIZE < (3*0xFFFF + 10))
		fatal (CCX_COMMON_EXIT_BUG_BUG, "BUFSIZE too small for RCWT caption block.\n");

	// Generic buffer to hold some data
	parsebuf = (unsigned char*)malloc(1024);


	LLONG currfts;
	uint16_t cbcount = 0;

	int bread = 0; // Bytes read

	buffered_read(ctx, parsebuf, 11);
	ctx->past+=result;
	bread+=(int) result;
	if (result!=11)
	{
		mprint("Premature end of file!\n");
		end_of_file=1;
		return;
	}

	// Expecting RCWT header
	if( !memcmp(parsebuf, "\xCC\xCC\xED", 3 ) )
	{
		dbg_print(CCX_DMT_PARSE, "\nRCWT header\n");
		dbg_print(CCX_DMT_PARSE, "File created by %02X version %02X%02X\nFile format revision: %02X%02X\n",
				parsebuf[3], parsebuf[4], parsebuf[5],
				parsebuf[6], parsebuf[7]);

	}
	else
	{
		fatal(EXIT_MISSING_RCWT_HEADER, "Missing RCWT header. Abort.\n");
	}

	if (parsebuf[6] == 0 && parsebuf[7] == 2)
	{
		tlt_read_rcwt(ctx);
		return;
	}

	// Initialize first time. As RCWT files come with the correct FTS the
	// initial (minimal) time needs to be set to 0.
	current_pts = 0;
	pts_set=1;
	set_fts(); // Now set the FTS related variables

	// Loop until no more data is found
	while(1)
	{
		// Read the data header
		buffered_read(ctx, parsebuf, 10);
		ctx->past+=result;
		bread+=(int) result;

		if (result!=10)
		{
			if (result!=0)
				mprint("Premature end of file!\n");

			// We are done
			end_of_file=1;
			break;
		}
		currfts = *((LLONG*)(parsebuf));
		cbcount = *((uint16_t*)(parsebuf+8));

		dbg_print(CCX_DMT_PARSE, "RCWT data header FTS: %s  blocks: %u\n",
				print_mstime(currfts), cbcount);

		if ( cbcount > 0 )
		{
			if ( cbcount*3 > parsebufsize) {
				parsebuf = (unsigned char*)realloc(parsebuf, cbcount*3);
				if (!parsebuf)
					fatal(EXIT_NOT_ENOUGH_MEMORY, "Out of memory");
				parsebufsize = cbcount*3;
			}
			buffered_read(ctx, parsebuf, cbcount*3);
			ctx->past+=result;
			bread+=(int) result;
			if (result!=cbcount*3)
			{
				mprint("Premature end of file!\n");
				end_of_file=1;
				break;
			}

			// Process the data
			current_pts = currfts*(MPEG_CLOCK_FREQ/1000);
			if (pts_set==0)
				pts_set=1;
			set_fts(); // Now set the FTS related variables

			dbg_print(CCX_DMT_VIDES, "PTS: %s (%8u)",
					print_mstime(current_pts/(MPEG_CLOCK_FREQ/1000)),
					(unsigned) (current_pts));
			dbg_print(CCX_DMT_VIDES, "  FTS: %s\n", print_mstime(get_fts()));

			for (int j=0; j<cbcount*3; j=j+3)
			{
				do_cb(dec_ctx, parsebuf+j, &dec_sub);
			}
		}
		if (dec_sub.got_output)
		{
			encode_sub(enc_ctx,&dec_sub);
			dec_sub.got_output = 0;
		}
	} // end while(1)

	dbg_print(CCX_DMT_PARSE, "Processed %d bytes\n", bread);
}
