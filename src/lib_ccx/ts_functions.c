#include "lib_ccx.h"
#include "ccx_common_option.h"

unsigned char tspacket[188]; // Current packet

struct ts_payload payload;
unsigned char *last_pat_payload=NULL;
unsigned last_pat_length = 0;


static unsigned char *haup_capbuf = NULL;
static long haup_capbufsize = 0;
static long haup_capbuflen = 0; // Bytes read in haup_capbuf

unsigned TS_program_number = 0; // Identifier for current program
unsigned pmtpid = 0; // PID for Program Map Table
unsigned cap_stream_type=CCX_STREAM_TYPE_UNKNOWNSTREAM; // Stream type for cappid
extern void *ccx_dvb_context;

// Descriptions for ts ccx_stream_type
const char *desc[256];

void dinit_ts (struct lib_ccx_ctx *ctx)
{
	freep(&ctx->capbuf);
}
void init_ts(struct lib_ccx_ctx *ctx)
{
	// Constants
	desc[CCX_STREAM_TYPE_UNKNOWNSTREAM] = "Unknown";
	desc[CCX_STREAM_TYPE_VIDEO_MPEG1] = "MPEG-1 video";
	desc[CCX_STREAM_TYPE_VIDEO_MPEG2] = "MPEG-2 video";
	desc[CCX_STREAM_TYPE_AUDIO_MPEG1] = "MPEG-1 audio";
	desc[CCX_STREAM_TYPE_AUDIO_MPEG2] = "MPEG-2 audio";
	desc[CCX_STREAM_TYPE_MHEG_PACKETS] = "MHEG Packets";
	desc[CCX_STREAM_TYPE_PRIVATE_TABLE_MPEG2] = "MPEG-2 private table sections";
	desc[CCX_STREAM_TYPE_PRIVATE_MPEG2] ="MPEG-2 private data";
	desc[CCX_STREAM_TYPE_MPEG2_ANNEX_A_DSM_CC] = "MPEG-2 Annex A DSM CC";
	desc[CCX_STREAM_TYPE_ITU_T_H222_1] = "ITU-T Rec. H.222.1";
	desc[CCX_STREAM_TYPE_AUDIO_AAC] =   "AAC audio";
	desc[CCX_STREAM_TYPE_VIDEO_MPEG4] = "MPEG-4 video";
	desc[CCX_STREAM_TYPE_VIDEO_H264] =  "H.264 video";
	desc[CCX_STREAM_TYPE_PRIVATE_USER_MPEG2] = "MPEG-2 User Private";
	desc[CCX_STREAM_TYPE_AUDIO_AC3] =   "AC3 audio";
	desc[CCX_STREAM_TYPE_AUDIO_DTS] =   "DTS audio";
	desc[CCX_STREAM_TYPE_AUDIO_HDMV_DTS]="HDMV audio";
	desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_A] = "ISO/IEC 13818-6 type A";
	desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_B] = "ISO/IEC 13818-6 type B";
	desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_C] = "ISO/IEC 13818-6 type C";
	desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_D] = "ISO/IEC 13818-6 type D";

	//Buffer
	ctx->capbuf = (unsigned char*)malloc(ctx->capbufsize);
}


// Return 1 for sucessfully read ts packet
int ts_readpacket(struct lib_ccx_ctx* ctx)
{
	if (ctx->m2ts)
	{
		/* M2TS just adds 4 bytes to each packet (so size goes from 188 to 192)
		   The 4 bytes are not important to us, so just skip
		// TP_extra_header {   
		Copy_permission_indicator 2  unimsbf
		Arrival_time_stamp 30 unimsbf
		} */
		char tp_extra_header[4];
		buffered_read(ctx, tp_extra_header, 4);
		ctx->past += result;
		if (result != 4)
		{
			if (result>0)
				mprint("Premature end of file!\n");
			end_of_file = 1;
			return 0;
		}
	}
	buffered_read(ctx, tspacket, 188);
	ctx->past+=result;
	if (result!=188)
	{
		if (result>0)
			mprint("Premature end of file!\n");
		end_of_file=1;
		return 0;
	}

	int printtsprob = 1;
	while (tspacket[0]!=0x47)
	{
		if (printtsprob)
		{
			mprint ("\nProblem: No TS header mark (filepos=%lld). Received bytes:\n", ctx->past);
			dump (CCX_DMT_GENERIC_NOTICES, tspacket,4, 0, 0);

			mprint ("Skip forward to the next TS header mark.\n");
			printtsprob = 0;
		}

		unsigned char *tstemp;
		// The amount of bytes read into tspacket
		int tslen = 188;

		// Check for 0x47 in the remaining bytes of tspacket
		tstemp = (unsigned char *) memchr (tspacket+1, 0x47, tslen-1);
		if (tstemp != NULL )
		{
			// Found it
			int atpos = tstemp-tspacket;

			memmove (tspacket,tstemp,(size_t)(tslen-atpos));
			buffered_read(ctx, tspacket+(tslen-atpos), atpos);
			ctx->past+=result;
			if (result!=atpos)
			{
				mprint("Premature end of file!\n");
				end_of_file=1;
				return 0;
			}
		}
		else
		{
			// Read the next 188 bytes.
			buffered_read(ctx, tspacket, tslen);
			ctx->past+=result;
			if (result!=tslen)
			{
				mprint("Premature end of file!\n");
				end_of_file=1;
				return 0;
			}
		}
	}

	unsigned char *payload_start = tspacket + 4;
	unsigned payload_length = 188 - 4;

	unsigned transport_error_indicator = (tspacket[1]&0x80)>>7;
	unsigned payload_start_indicator = (tspacket[1]&0x40)>>6;
	// unsigned transport_priority = (tspacket[1]&0x20)>>5;
	unsigned pid = (((tspacket[1] & 0x1F) << 8) | tspacket[2]) & 0x1FFF;
	// unsigned transport_scrambling_control = (tspacket[3]&0xC0)>>6;
	unsigned adaptation_field_control = (tspacket[3]&0x30)>>4;
	unsigned ccounter = tspacket[3] & 0xF;

	if (transport_error_indicator)
	{
		mprint ("Warning: Defective (error indicator on) TS packet (filepos=%lld):\n", ctx->past);
		dump (CCX_DMT_GENERIC_NOTICES, tspacket, 188, 0, 0);
	}

	unsigned adaptation_field_length = 0;
	if ( adaptation_field_control & 2 )
	{
		// Take the PCR (Program Clock Reference) from here, in case PTS is not available (copied from telxcc).
		adaptation_field_length = tspacket[4];

		uint8_t af_pcr_exists = (tspacket[5] & 0x10) >> 4;
		if (af_pcr_exists > 0)
		{
			uint64_t pts = 0;
			pts |= (tspacket[6] << 25);
			pts |= (tspacket[7] << 17);
			pts |= (tspacket[8] << 9);
			pts |= (tspacket[9] << 1);
			pts |= (tspacket[10] >> 7);
			ctx->global_timestamp = (uint32_t) pts / 90;
			pts = ((tspacket[10] & 0x01) << 8);
			pts |= tspacket[11];
			ctx->global_timestamp += (uint32_t) (pts / 27000);
			if (!ctx->global_timestamp_inited)
			{
				ctx->min_global_timestamp = ctx->global_timestamp;
				ctx->global_timestamp_inited = 1;
			}
		}


		payload_start = payload_start + adaptation_field_length + 1;
		payload_length = tspacket+188-payload_start;
	}

	dbg_print(CCX_DMT_PARSE, "TS pid: %d  PES start: %d  counter: %u  payload length: %u  adapt length: %d\n",
			pid, payload_start_indicator, ccounter, payload_length,
			(int) (adaptation_field_length));

	// Catch bad packages with adaptation_field_length > 184 and
	// the unsigned nature of payload_length leading to huge numbers.
	if (payload_length > 184)
	{
		// This renders the package invalid
		payload_length = 0;
		dbg_print(CCX_DMT_PARSE, "  Reject package - set length to zero.\n");
	}

	// Save data in global struct
	payload.start = payload_start;
	payload.length = payload_length;
	payload.pesstart = payload_start_indicator;
	payload.pid = pid;
	payload.counter = ccounter;
	payload.transport_error = transport_error_indicator;
	if (payload_length == 0)
	{
		dbg_print(CCX_DMT_PARSE, "  No payload in package.\n");
	}

	// Store packet information
	return 1;
}



void look_for_caption_data (struct lib_ccx_ctx *ctx)
{
	// See if we find the usual CC data marker (GA94) in this packet.
	if (payload.length<4 || ctx->PIDs_seen[payload.pid]==3) // Second thing means we already inspected this PID
		return;
	for (unsigned i=0;i<payload.length-3;i++)
	{
		if (payload.start[i]=='G' && payload.start[i+1]=='A' &&
				payload.start[i+2]=='9' && payload.start[i+3]=='4')
		{
			mprint ("PID %u seems to contain captions.\n", payload.pid);
			ctx->PIDs_seen[payload.pid]=3;
			return;
		}
	}

}

// Read ts packets until a complete video PES element can be returned.
// The data is read into capbuf and the function returns the number of
// bytes read.
long ts_readstream(struct lib_ccx_ctx *ctx)
{
	static int prev_ccounter = 0;
	static int prev_packet = 0;
	int gotpes = 0;
	long pespcount = 0; // count packets in PES with captions
	long pcount=0; // count all packets until PES is complete
	int saw_pesstart = 0;
	int packet_analysis_mode = 0; // If we can't find any packet with CC based from PMT, look for captions in all packets
	ctx->capbuflen = 0;

	do
	{
		pcount++;

		if( !prev_packet )
		{
			// Exit the loop at EOF
			if ( !ts_readpacket(ctx) )
				break;
		}
		else
			prev_packet = 0;

		// Skip damaged packets, they could do more harm than good
		if (payload.transport_error)
		{
			dbg_print(CCX_DMT_VERBOSE, "Packet (pid %u) skipped - transport error.\n",
				payload.pid);
			continue;
		}
		// Skip packets with no payload.  This also fixes the problems
		// with the continuity counter not being incremented in empty
		// packets.
		if ( !payload.length )
		{
			dbg_print(CCX_DMT_VERBOSE, "Packet (pid %u) skipped - no payload.\n",
				payload.pid);
			continue;
		}

		// Check for PAT
		if( payload.pid == 0) // This is a PAT
		{
			if (parse_PAT(ctx)) // Returns 1 if there was some data in the buffer already
				ctx->capbuflen = 0;
			continue;
		}
		
		if( ccx_options.xmltv >= 1 && payload.pid == 0x12) // This is DVB EIT
			parse_EPG_packet(ctx);
		if( ccx_options.xmltv >= 1 && payload.pid >= 0x1000) // This may be ATSC EPG packet
			parse_EPG_packet(ctx);

		// PID != 0 but no PMT selected yet, ignore the rest of the current
		// package and continue searching, UNLESS we are in -autoprogram, which requires us
		// to analyze all PMTs to look for a stream with data.
		if ( !pmtpid && ccx_options.teletext_mode!=CCX_TXT_IN_USE && !ccx_options.ts_autoprogram)
		{
			dbg_print(CCX_DMT_PARSE, "Packet (pid %u) skipped - no PMT pid identified yet.\n",
					   payload.pid);
			continue;
		}

		int is_pmt=0, j;
		for (j=0;j<pmt_array_length;j++)
		{
			if (pmt_array[j].PMT_PID==payload.pid)
			{
				if (!ctx->PIDs_seen[payload.pid])
					dbg_print(CCX_DMT_PAT, "This PID (%u) is a PMT for program %u.\n",payload.pid, pmt_array[j].program_number);
				is_pmt=1;
				break;
			}
		}

		if (is_pmt)
		{
			ctx->PIDs_seen[payload.pid]=2;
			if(payload.pesstart)
			{
				int len = *payload.start++;
				payload.start += len;
				if(write_section(ctx, &payload,payload.start,(tspacket + 188 ) - payload.start,j))
					gotpes=1; // Signals that something changed and that we must flush the buffer
			}
			else
			{
				if(write_section(ctx, &payload,payload.start,(tspacket + 188 ) - payload.start,j))
					gotpes=1; // Signals that something changed and that we must flush the buffer
			}
			if (payload.pid==pmtpid && ccx_options.ts_cappid==0 && ccx_options.investigate_packets) // It was our PMT yet we don't have a PID to get data from
				packet_analysis_mode=1;

			continue;
		}

		switch (ctx->PIDs_seen[payload.pid])
		{
			case 0: // First time we see this PID
				if (ctx->PIDs_programs[payload.pid])
				{
					dbg_print(CCX_DMT_PARSE, "\nNew PID found: %u (%s), belongs to program: %u\n", payload.pid,
						desc[ctx->PIDs_programs[payload.pid]->printable_stream_type],
						ctx->PIDs_programs[payload.pid]->program_number);
					ctx->PIDs_seen[payload.pid]=2;
				}
				else
				{
					dbg_print(CCX_DMT_PARSE, "\nNew PID found: %u, program number still unknown\n", payload.pid);
					ctx->PIDs_seen[payload.pid]=1;
				}
				break;
			case 1: // Saw it before but we didn't know what program it belonged to. Luckier now?
				if (ctx->PIDs_programs[payload.pid])
				{
					dbg_print(CCX_DMT_PARSE, "\nProgram for PID: %u (previously unknown) is: %u (%s)\n", payload.pid,
						ctx->PIDs_programs[payload.pid]->program_number,
						desc[ctx->PIDs_programs[payload.pid]->printable_stream_type]
						);
					ctx->PIDs_seen[payload.pid]=2;
				}
				break;
			case 2: // Already seen and reported with correct program
				break;
			case 3: // Already seen, reported, and inspected for CC data (and found some)
				break;
		}


		if (payload.pid==1003 && !ctx->hauppauge_warning_shown && !ccx_options.hauppauge_mode)
		{
			// TODO: Change this very weak test for something more decent such as size.
			mprint ("\n\nNote: This TS could be a recording from a Hauppage card. If no captions are detected, try --hauppauge\n\n");
			ctx->hauppauge_warning_shown=1;
		}

		// No caption stream PID defined yet, continue searching.
		if ( !ccx_options.ts_cappid )
		{
			if (!packet_analysis_mode)
				dbg_print(CCX_DMT_PARSE, "Packet (pid %u) skipped - no stream with captions identified yet.\n",
					   payload.pid);
			else
				look_for_caption_data (ctx);
			continue;
		}

		if (ccx_options.hauppauge_mode && payload.pid==HAUPPAGE_CCPID)
		{
			// Haup packets processed separately, because we can't mix payloads. So they go in their own buffer
			// copy payload to capbuf
			int haup_newcapbuflen = haup_capbuflen + payload.length;
			if ( haup_newcapbuflen > haup_capbufsize) {
				haup_capbuf = (unsigned char*)realloc(haup_capbuf, haup_newcapbuflen);
				if (!haup_capbuf)
					fatal(EXIT_NOT_ENOUGH_MEMORY, "Out of memory");
				haup_capbufsize = haup_newcapbuflen;
			}
			memcpy(haup_capbuf+haup_capbuflen, payload.start, payload.length);
			haup_capbuflen = haup_newcapbuflen;

		}
		/*
		 * if dvb subtitle is selected then start time taken from first PTS
		 * of any stream
		 */
		if ( cap_stream_type == CCX_STREAM_TYPE_PRIVATE_MPEG2 && ccx_dvb_context && !pts_set)
		{
			if(read_pts_pes(payload.start,payload.length) == 0)
				set_fts();
		}

		// Check for PID with captions. Note that in Hauppauge mode we also process the video stream because
		// we need the timing from its PES header, which isn't included in Hauppauge's packets
		if( payload.pid == ccx_options.ts_cappid)
		{   // Now we got a payload

			// Video PES start
			if (payload.pesstart)
			{
				// Pretend the previous was smaller
				prev_ccounter=payload.counter-1;

				saw_pesstart = 1;
			}

			// Discard packets when no pesstart was found.
			if ( !saw_pesstart )
			{
				dbg_print(CCX_DMT_PARSE, "Packet (pid %u) skipped - Did not see pesstart.\n",
						   payload.pid);
				continue;
			}

			// If the buffer is empty we just started this function
			if (payload.pesstart && ctx->capbuflen > 0)
			{
				dbg_print(CCX_DMT_PARSE, "\nPES finished (%ld bytes/%ld PES packets/%ld total packets)\n",
						   ctx->capbuflen, pespcount, pcount);

				// Keep the data in capbuf to be worked on

				prev_packet = 1;
				gotpes = 1;
				break;
			}

			if ( (prev_ccounter==15 ? 0 : prev_ccounter+1) != payload.counter )
			{
				mprint("TS continuity counter not incremented prev/curr %u/%u\n",
					   prev_ccounter, payload.counter);
			}
			prev_ccounter = payload.counter;


			pespcount++;
			// copy payload to capbuf
			int newcapbuflen = ctx->capbuflen + payload.length;
			if ( newcapbuflen > ctx->capbufsize) {
				ctx->capbuf = (unsigned char*)realloc(ctx->capbuf, newcapbuflen);
				if (!ctx->capbuf)
					fatal(EXIT_NOT_ENOUGH_MEMORY, "Out of memory");
				ctx->capbufsize = newcapbuflen;
			}
			memcpy(ctx->capbuf + ctx->capbuflen, payload.start, payload.length);
			ctx->capbuflen = newcapbuflen;
		}
		//else
		//	if(debug_verbose)
		//		printf("Packet (pid %u) skipped - unused.\n",
		//			   payload.pid);

		// Nothing suitable found, start over
	}
	while( !gotpes ); // gotpes==1 never arrives here because of the breaks

	return ctx->capbuflen;
}


// TS specific data grabber
LLONG ts_getmoredata(struct lib_ccx_ctx *ctx)
{
	long payload_read = 0;
	const char *tstr; // Temporary string to describe the stream type

	do
	{
		if( !ts_readstream(ctx) )
		{   // If we didn't get data, try again
			mprint("(no CC data extracted)\n");
			continue;
		}
		// Handle obscure case where we didn't find a PMT (so
		// cap_stream_type wasn't set) but the user told us what kind
		// of stream to look for, so we move forward anyway. This
		// happens with MPEG-2 sources from ZeeVee HDbridge.
		if (cap_stream_type == CCX_STREAM_TYPE_UNKNOWNSTREAM && ccx_options.ts_forced_streamtype != CCX_STREAM_TYPE_UNKNOWNSTREAM)
		{
			cap_stream_type = ccx_options.ts_forced_streamtype;
		}

		// Separate MPEG-2 and H.264 video streams
		if( cap_stream_type == CCX_STREAM_TYPE_VIDEO_MPEG2)
		{
			ccx_bufferdatatype = CCX_PES;
			tstr = "MPG";
		}
		else if( cap_stream_type == CCX_STREAM_TYPE_VIDEO_H264 )
		{
			ccx_bufferdatatype = CCX_H264;
			tstr = "H.264";
		}
		else if ( cap_stream_type == CCX_STREAM_TYPE_PRIVATE_MPEG2 && ccx_dvb_context )
		{
			ccx_bufferdatatype = CCX_DVB_SUBTITLE;
			tstr = "DVB subtitle";
		}
		else if ( cap_stream_type == CCX_STREAM_TYPE_UNKNOWNSTREAM && ccx_options.hauppauge_mode)
		{
			ccx_bufferdatatype = CCX_HAUPPAGE;
			tstr = "Hauppage";
		}
		else if ( cap_stream_type == CCX_STREAM_TYPE_PRIVATE_MPEG2 && ccx_options.teletext_mode==CCX_TXT_IN_USE)
		{
			ccx_bufferdatatype = CCX_TELETEXT;
			tstr = "Teletext";
		}
		else if ( cap_stream_type == CCX_STREAM_TYPE_PRIVATE_MPEG2 && ccx_options.teletext_mode==CCX_TXT_FORBIDDEN)
		{
			ccx_bufferdatatype = CCX_PRIVATE_MPEG2_CC;
			tstr = "CC in private MPEG packet";
		}
		else
		{
			if (ccx_options.ts_forced_cappid)
				fatal(CCX_COMMON_EXIT_UNSUPPORTED, "Unable to determine stream type of selected PID.");
			else
				fatal(CCX_COMMON_EXIT_BUG_BUG, "Not reachable!");
		}
		// We read a video PES

		if (ctx->capbuf[0] != 0x00 || ctx->capbuf[1] != 0x00 ||
			ctx->capbuf[2] != 0x01)
		{
			// ??? Shouldn't happen. Complain and try again.
			mprint("Missing PES header!\n");
			dump(CCX_DMT_GENERIC_NOTICES, ctx->capbuf,256, 0, 0);
			continue;
		}
		unsigned stream_id = ctx->capbuf[3];

		if (ccx_options.teletext_mode == CCX_TXT_IN_USE)
		{
			if (ccx_options.ts_cappid==0)
			{ // If here, the user forced teletext mode but didn't supply a PID, and we haven't found it yet.
				continue;
			}
			memcpy(ctx->buffer+inbuf, ctx->capbuf, ctx->capbuflen);
			payload_read = ctx->capbuflen;
			inbuf += ctx->capbuflen;
			break;
		}
		if (ccx_bufferdatatype == CCX_PRIVATE_MPEG2_CC)
		{
			dump (CCX_DMT_GENERIC_NOTICES, ctx->capbuf, ctx->capbuflen,0, 1);
			// Bogus data, so we return something
			ctx->buffer[inbuf++] = 0xFA;
			ctx->buffer[inbuf++] = 0x80;
			ctx->buffer[inbuf++] = 0x80;
			payload_read += 3;
			break;
		}
		if (ccx_options.hauppauge_mode)
		{
			if (haup_capbuflen%12 != 0)
				mprint ("Warning: Inconsistent Hauppage's buffer length\n");
			if (!haup_capbuflen)
			{
				// Do this so that we always return something until EOF. This will be skipped.
				ctx->buffer[inbuf++] = 0xFA;
				ctx->buffer[inbuf++] = 0x80;
				ctx->buffer[inbuf++] = 0x80;
				payload_read += 3;
			}

			for (int i = 0; i<haup_capbuflen; i += 12)
			{
				unsigned haup_stream_id = haup_capbuf[i+3];
				if (haup_stream_id == 0xbd && haup_capbuf[i+4] == 0 && haup_capbuf[i+5] == 6 )
				{
					// Because I (CFS) don't have a lot of samples for this, for now I make sure everything is like the one I have:
					// 12 bytes total length, stream id = 0xbd (Private non-video and non-audio), etc
					if (2 > BUFSIZE - inbuf)
					{
						fatal(CCX_COMMON_EXIT_BUG_BUG,
								"Remaining buffer (%lld) not enough to hold the 3 Hauppage bytes.\n"
								"Please send bug report!",
								BUFSIZE - inbuf);
					}
					if (haup_capbuf[i+9]==1 || haup_capbuf[i+9]==2) // Field match. // TODO: If extract==12 this won't work!
					{
						if (haup_capbuf[i+9]==1)
							ctx->buffer[inbuf++]=4; // Field 1 + cc_valid=1
						else
							ctx->buffer[inbuf++]=5; // Field 2 + cc_valid=1
						ctx->buffer[inbuf++]=haup_capbuf[i+10];
						ctx->buffer[inbuf++]=haup_capbuf[i+11];
						payload_read+=3;
					}
					/*
					   if (inbuf>1024) // Just a way to send the bytes to the decoder from time to time, otherwise the buffer will fill up.
					   break;
					   else
					   continue; */
				}
			}
			haup_capbuflen=0;
		}

		dbg_print(CCX_DMT_VERBOSE, "TS payload start video PES id: %d  len: %ld\n",
			   stream_id, ctx->capbuflen);

		int pesheaderlen;
		int vpesdatalen = read_video_pes_header(ctx, ctx->capbuf, &pesheaderlen, ctx->capbuflen);

		if (ccx_bufferdatatype == CCX_DVB_SUBTITLE && !vpesdatalen)
		{
			dbg_print(CCX_DMT_VERBOSE, "TS payload is a DVB Subtitle\n");
			payload_read = ctx->capbuflen;
			inbuf += payload_read;
			break;
		}

		if (vpesdatalen < 0)
		{
			dbg_print(CCX_DMT_VERBOSE, "Seems to be a broken PES. Terminating file handling.\n");
			end_of_file = 1;
			break;
		}

		unsigned char *databuf = ctx->capbuf + pesheaderlen;
		long databuflen = ctx->capbuflen - pesheaderlen;

		// If the package length is unknown vpesdatalen is zero.
		// If we know he package length, use it to quit
		dbg_print(CCX_DMT_VERBOSE, "Read PES-%s (databuffer %ld/PES data %d) ",
			   tstr, databuflen, vpesdatalen);
		// We got the whole PES in buffer
		if( vpesdatalen && (databuflen >= vpesdatalen) )
			dbg_print(CCX_DMT_VERBOSE, " - complete");
		dbg_print(CCX_DMT_VERBOSE, "\n");


		if (databuflen > BUFSIZE - inbuf)
		{
			fatal(CCX_COMMON_EXIT_BUG_BUG,
				  "PES data packet (%ld) larger than remaining buffer (%lld).\n"
				  "Please send bug report!",
				   databuflen, BUFSIZE - inbuf);
		}

		if (!ccx_options.hauppauge_mode) // in Haup mode the buffer is filled somewhere else
		{
			memcpy(ctx->buffer+inbuf, databuf, databuflen);
			payload_read = databuflen;
			inbuf += databuflen;
		}
		break;
	} while ( !end_of_file );

	return payload_read;
}
