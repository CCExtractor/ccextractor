#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "activity.h"
#include "ccx_demuxer.h"
#include "list.h"
#include "dvb_subtitle_decoder.h"
#include "ccx_decoders_isdb.h"
#include "file_buffer.h"
#include <inttypes.h>

#ifdef DEBUG_SAVE_TS_PACKETS
#include <sys/types.h>
#include <unistd.h>
#endif

#define RAI_MASK 0x40 // byte mask to check if RAI bit is set (random access indicator)

unsigned char tspacket[188]; // Current packet

// struct ts_payload payload;

static unsigned char *haup_capbuf = NULL;
static long haup_capbufsize = 0;
static long haup_capbuflen = 0; // Bytes read in haup_capbuf
uint64_t last_pts = 0;		// PTS of last PES packet (debug purposes)

// Descriptions for ts ccx_stream_type
const char *desc[256];

char *get_buffer_type_str(struct cap_info *cinfo)
{
	if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_MPEG2)
	{
		return strdup("MPG");
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_H264)
	{
		return strdup("H.264");
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_HEVC)
	{
		return strdup("HEVC");
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_ISDB_CC)
	{
		return strdup("ISDB CC subtitle");
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_DVB)
	{
		return strdup("DVB subtitle");
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_UNKNOWNSTREAM && ccx_options.hauppauge_mode)
	{
		return strdup("Hauppage");
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_TELETEXT)
	{
		return strdup("Teletext");
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_ATSC_CC)
	{
		return strdup("CC in private MPEG packet");
	}
	else
	{
		return NULL;
	}
}
void pes_header_dump(uint8_t *buffer, long len)
{
	// Write the PES Header to console
	uint64_t pes_prefix;
	uint8_t pes_stream_id;
	uint16_t pes_packet_length;
	uint8_t optional_pes_header_included = NO;
	uint16_t optional_pes_header_length = 0;
	uint64_t pts = 0;

	// check if we have at least some data to work with
	if (len < 6)
		return;

	// Packetized Elementary Stream (PES) 32-bit start code
	pes_prefix = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
	pes_stream_id = buffer[3];

	// check for PES header
	if (pes_prefix != 0x000001)
		return;

	pes_packet_length = 6 + ((buffer[4] << 8) | buffer[5]); // 5th and 6th byte of the header define the length of the rest of the packet (+6 is for the prefix, stream ID and packet length)

	printf("Packet start code prefix: %04lx # ", pes_prefix);
	printf("Stream ID: %04x # ", pes_stream_id);
	printf("Packet length: %d ", pes_packet_length);

	if (pes_packet_length == 6)
	{
		// great, there is only a header and no extension + payload
		printf("\n");
		return;
	}

	// optional PES header marker bits (10.. ....)
	if ((buffer[6] & 0xc0) == 0x80)
	{
		optional_pes_header_included = YES;
		optional_pes_header_length = buffer[8];
	}

	if (optional_pes_header_included == YES && optional_pes_header_length > 0 && (buffer[7] & 0x80) > 0 && len >= 14)
	{
		// get associated PTS as it exists
		pts = (buffer[9] & 0x0e);
		pts <<= 29;
		pts |= (buffer[10] << 22);
		pts |= ((buffer[11] & 0xfe) << 14);
		pts |= (buffer[12] << 7);
		pts |= ((buffer[13] & 0xfe) >> 1);
		// printf("# Associated PTS: %d \n", pts);
		printf("# Associated PTS: %" PRId64 " # ", pts);
		printf("Diff: %" PRIu64 "\n", pts - last_pts);
		// printf("Diff: %d # ", pts - last_pts);
		last_pts = pts;
	}
}
enum ccx_bufferdata_type get_buffer_type(struct cap_info *cinfo)
{
	if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_MPEG2)
	{
		return CCX_PES;
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_H264)
	{
		return CCX_H264;
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_HEVC)
	{
		return CCX_HEVC;
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_DVB)
	{
		return CCX_DVB_SUBTITLE;
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_ISDB_CC)
	{
		return CCX_ISDB_SUBTITLE;
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_UNKNOWNSTREAM && ccx_options.hauppauge_mode)
	{
		return CCX_HAUPPAGE;
	}
	else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_TELETEXT)
	{
		return CCX_TELETEXT;
	}
	else if ((cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 ||
		  cinfo->stream == CCX_STREAM_TYPE_PRIVATE_USER_MPEG2) &&
		 cinfo->codec == CCX_CODEC_ATSC_CC)
	{
		// ATSC CC can be in either private stream type - process both as PES
		return CCX_PES;
	}
	else
	{
		return CCX_EINVAL;
	}
}
void init_ts(struct ccx_demuxer *ctx)
{
	// Constants
	desc[CCX_STREAM_TYPE_UNKNOWNSTREAM] = "Unknown";
	desc[CCX_STREAM_TYPE_VIDEO_MPEG1] = "MPEG-1 video";
	desc[CCX_STREAM_TYPE_VIDEO_MPEG2] = "MPEG-2 video";
	desc[CCX_STREAM_TYPE_AUDIO_MPEG1] = "MPEG-1 audio";
	desc[CCX_STREAM_TYPE_AUDIO_MPEG2] = "MPEG-2 audio";
	desc[CCX_STREAM_TYPE_MHEG_PACKETS] = "MHEG Packets";
	desc[CCX_STREAM_TYPE_PRIVATE_TABLE_MPEG2] = "MPEG-2 private table sections";
	desc[CCX_STREAM_TYPE_PRIVATE_MPEG2] = "MPEG-2 private data";
	desc[CCX_STREAM_TYPE_MPEG2_ANNEX_A_DSM_CC] = "MPEG-2 Annex A DSM CC";
	desc[CCX_STREAM_TYPE_ITU_T_H222_1] = "ITU-T Rec. H.222.1";
	desc[CCX_STREAM_TYPE_AUDIO_AAC] = "AAC audio";
	desc[CCX_STREAM_TYPE_VIDEO_MPEG4] = "MPEG-4 video";
	desc[CCX_STREAM_TYPE_VIDEO_H264] = "H.264 video";
	desc[CCX_STREAM_TYPE_VIDEO_HEVC] = "HEVC video";
	desc[CCX_STREAM_TYPE_PRIVATE_USER_MPEG2] = "MPEG-2 User Private";
	desc[CCX_STREAM_TYPE_AUDIO_AC3] = "AC3 audio";
	desc[CCX_STREAM_TYPE_AUDIO_DTS] = "DTS audio";
	desc[CCX_STREAM_TYPE_AUDIO_HDMV_DTS] = "HDMV audio";
	desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_A] = "ISO/IEC 13818-6 type A";
	desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_B] = "ISO/IEC 13818-6 type B";
	desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_C] = "ISO/IEC 13818-6 type C";
	desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_D] = "ISO/IEC 13818-6 type D";
}

// Return 1 for successfully read ts packet
int ts_readpacket(struct ccx_demuxer *ctx, struct ts_payload *payload)
{
	unsigned int adaptation_field_length = 0;
	unsigned int adaptation_field_control;
	long long result;
	if (ctx->m2ts)
	{
		/* M2TS just adds 4 bytes to each packet (so size goes from 188 to 192)
		   The 4 bytes are not important to us, so just skip
		// TP_extra_header {
		Copy_permission_indicator 2  unimsbf
		Arrival_time_stamp 30 unimsbf
		} */
		unsigned char tp_extra_header[4];
		result = buffered_read(ctx, tp_extra_header, 4);
		ctx->past += result;
		if (result != 4)
		{
			if (result > 0)
				mprint("Premature end of file (incomplete TS packer header, expected 4 bytes to skip M2TS extra bytes, got %lld).\n", result);
			return CCX_EOF;
		}
	}

	result = buffered_read(ctx, tspacket, 188);
	ctx->past += result;
	if (result != 188)
	{
		if (result > 0)
			mprint("Premature end of file - Transport Stream packet is incomplete (expected 188 bytes, got %lld).\n", result);
		return CCX_EOF;
	}

	int printtsprob = 1;
	while (tspacket[0] != 0x47)
	{
		if (printtsprob)
		{
			dbg_print(CCX_DMT_DUMPDEF, "\nProblem: No TS header mark (filepos=%lld). Received bytes:\n", ctx->past);
			dump(CCX_DMT_DUMPDEF, tspacket, 4, 0, 0);

			dbg_print(CCX_DMT_DUMPDEF, "Skip forward to the next TS header mark.\n");
			printtsprob = 0;
		}

		unsigned char *tstemp;
		// The amount of bytes read into tspacket
		int tslen = 188;

		// Check for 0x47 in the remaining bytes of tspacket
		tstemp = (unsigned char *)memchr(tspacket + 1, 0x47, tslen - 1);
		if (tstemp != NULL)
		{
			// Found it
			int atpos = tstemp - tspacket;

			memmove(tspacket, tstemp, (size_t)(tslen - atpos));
			result = buffered_read(ctx, tspacket + (tslen - atpos), atpos);
			ctx->past += result;
			if (result != atpos)
			{
				mprint("Premature end of file!\n");
				return CCX_EOF;
			}
		}
		else
		{
			// Read the next 188 bytes.
			result = buffered_read(ctx, tspacket, tslen);
			ctx->past += result;
			if (result != tslen)
			{
				mprint("Premature end of file!\n");
				return CCX_EOF;
			}
		}
	}

#ifdef DEBUG_SAVE_TS_PACKETS
	// quick & dirty way to save packets so we reproduce issues that only
	// seem to happen when there's packet loss when processing a network
	// stream.
	FILE *savepacket;
	pid_t mypid = getpid();
	char spfn[1024];
	snprintf(spfn, sizeof(spfn), "/tmp/packets_%u.ts", (unsigned)mypid);
	savepacket = fopen(spfn, "ab");
	if (savepacket)
	{
		fwrite(tspacket, 188, 1, savepacket);
		fclose(savepacket);
	}
#endif

	payload->transport_error = (tspacket[1] & 0x80) >> 7;
	payload->pesstart = (tspacket[1] & 0x40) >> 6;
	// unsigned transport_priority = (tspacket[1]&0x20)>>5;
	payload->pid = (((tspacket[1] & 0x1F) << 8) | tspacket[2]) & 0x1FFF;
	// unsigned transport_scrambling_control = (tspacket[3]&0xC0)>>6;
	adaptation_field_control = (tspacket[3] & 0x30) >> 4;
	payload->counter = tspacket[3] & 0xF;

	if (payload->transport_error)
	{
		dbg_print(CCX_DMT_DUMPDEF, "Warning: Defective (error indicator on) TS packet (filepos=%lld):\n", ctx->past);
		dump(CCX_DMT_DUMPDEF, tspacket, 188, 0, 0);
	}

	payload->start = tspacket + 4;
	payload->length = 188 - 4;
	if (adaptation_field_control & 2)
	{
		// Take the PCR (Program Clock Reference) from here, in case PTS is not available (copied from telxcc).
		adaptation_field_length = tspacket[4];

		payload->have_pcr = (tspacket[5] & 0x10) >> 4;
		if (payload->have_pcr)
		{
			payload->pcr = 0;
			payload->pcr |= (tspacket[6] << 25);
			payload->pcr |= (tspacket[7] << 17);
			payload->pcr |= (tspacket[8] << 9);
			payload->pcr |= (tspacket[9] << 1);
			payload->pcr |= (tspacket[10] >> 7);
			/* Ignore 27 Mhz clock since we dont deal in nanoseconds*/
			// payload->pcr = ((tspacket[10] & 0x01) << 8);
			// payload->pcr |= tspacket[11];
		}

		payload->has_random_access_indicator = (tspacket[5] & RAI_MASK) != 0;

		// Catch bad packages with adaptation_field_length > 184 and
		// the unsigned nature of payload_length leading to huge numbers.
		if (adaptation_field_length < payload->length)
		{
			payload->start += adaptation_field_length + 1;
			payload->length -= adaptation_field_length + 1;
		}
		else
		{
			// This renders the package invalid
			payload->length = 0;
			dbg_print(CCX_DMT_PARSE, "  Reject package - set length to zero.\n");
		}
	}
	else
		payload->has_random_access_indicator = 0;

	dbg_print(CCX_DMT_PARSE, "TS pid: %d  PES start: %d  counter: %u  payload length: %u  adapt length: %d\n",
		  payload->pid, payload->start, payload->counter, payload->length,
		  (int)(adaptation_field_length));

	if (payload->length == 0)
	{
		dbg_print(CCX_DMT_PARSE, "  No payload in package.\n");
	}
	// Store packet information
	return CCX_OK;
}

void look_for_caption_data(struct ccx_demuxer *ctx, struct ts_payload *payload)
{
	unsigned int i = 0;
	// See if we find the usual CC data marker (GA94) in this packet.
	if (payload->length < 4 || ctx->PIDs_seen[payload->pid] == 3) // Second thing means we already inspected this PID
		return;

	// Check for PES header to detect video streams (no PAT/PMT mode)
	if (payload->pesstart && payload->length >= 9)
	{
		// Check for PES start code (00 00 01)
		if (payload->start[0] == 0x00 && payload->start[1] == 0x00 && payload->start[2] == 0x01)
		{
			unsigned char stream_id = payload->start[3];
			// Video stream IDs are 0xE0-0xEF
			if (stream_id >= 0xE0 && stream_id <= 0xEF)
			{
				// This is a video stream - check if we need to register it
				struct cap_info *cinfo = get_cinfo(ctx, payload->pid);
				if (cinfo == NULL)
				{
					// Not registered yet - determine video type from elementary stream
					// Look for MPEG-2 sequence header (00 00 01 B3) or H.264 NAL unit
					unsigned char pes_header_len = 0;
					if (payload->length > 8)
						pes_header_len = payload->start[8];

					unsigned int es_start = 9 + pes_header_len;
					if (es_start + 4 < payload->length)
					{
						unsigned char *es_data = payload->start + es_start;
						enum ccx_stream_type stream_type = CCX_STREAM_TYPE_VIDEO_MPEG2; // Default to MPEG-2

						// Check for H.264/H.265 NAL start codes
						if (es_data[0] == 0x00 && es_data[1] == 0x00 && es_data[2] == 0x00 && es_data[3] == 0x01)
						{
							// Check for H.264 NAL types first (1-byte header, type in bits 4:0)
							unsigned char h264_nal_type = es_data[4] & 0x1F;
							if (h264_nal_type == 7 || h264_nal_type == 8) // H.264 SPS or PPS
								stream_type = CCX_STREAM_TYPE_VIDEO_H264;
							else
							{
								// Check for HEVC NAL types (2-byte header, type in bits 6:1 of first byte)
								unsigned char hevc_nal_type = (es_data[4] >> 1) & 0x3F;
								// HEVC VPS=32, SPS=33, PPS=34, PREFIX_SEI=39, SUFFIX_SEI=40
								// Also check for IDR (19, 20) and CRA (21) which are common first NALs
								if (hevc_nal_type == 32 || hevc_nal_type == 33 || hevc_nal_type == 34 ||
								    hevc_nal_type == 39 || hevc_nal_type == 40 ||
								    hevc_nal_type == 19 || hevc_nal_type == 20 || hevc_nal_type == 21)
									stream_type = CCX_STREAM_TYPE_VIDEO_HEVC;
							}
						}

						mprint("PID %u detected as video stream (no PAT/PMT) - assuming %s.\n",
						       payload->pid,
						       stream_type == CCX_STREAM_TYPE_VIDEO_H264 ? "H.264" : (stream_type == CCX_STREAM_TYPE_VIDEO_HEVC ? "HEVC" : "MPEG-2"));

						// Register this PID as a video stream that may contain captions
						update_capinfo(ctx, payload->pid, stream_type, CCX_CODEC_ATSC_CC, 0, NULL);
						ctx->PIDs_seen[payload->pid] = 3;
						return;
					}
				}
			}
		}
	}

	// Look for GA94 caption marker
	for (i = 0; i < (payload->length - 3); i++)
	{
		if (payload->start[i] == 'G' && payload->start[i + 1] == 'A' &&
		    payload->start[i + 2] == '9' && payload->start[i + 3] == '4')
		{
			mprint("PID %u seems to contain CEA-608 captions.\n", payload->pid);

			// Register this PID if not already registered (no PAT/PMT mode)
			struct cap_info *cinfo = get_cinfo(ctx, payload->pid);
			if (cinfo == NULL)
			{
				mprint("Registering PID %u as MPEG-2 video with captions (no PAT/PMT detected).\n", payload->pid);
				update_capinfo(ctx, payload->pid, CCX_STREAM_TYPE_VIDEO_MPEG2, CCX_CODEC_ATSC_CC, 0, NULL);
			}

			ctx->PIDs_seen[payload->pid] = 3;
			return;
		}
	}
}

void delete_demuxer_data_node_by_pid(struct demuxer_data **data, int pid)
{
	struct demuxer_data *ptr;
	struct demuxer_data *sptr = NULL;

	ptr = *data;
	while (ptr)
	{
		if (ptr->stream_pid == pid)
		{
			if (sptr == NULL)
				*data = ptr->next_stream;
			else
				sptr->next_stream = ptr->next_stream;

			delete_demuxer_data(ptr);
			ptr = NULL;
		}
		else
		{
			sptr = ptr;
			ptr = ptr->next_stream;
		}
	}
}

struct demuxer_data *search_or_alloc_demuxer_data_node_by_pid(struct demuxer_data **data, int pid)
{
	struct demuxer_data *ptr;
	struct demuxer_data *sptr;
	if (!*data)
	{
		*data = alloc_demuxer_data();
		(*data)->program_number = -1;
		(*data)->stream_pid = pid;
		(*data)->bufferdatatype = CCX_UNKNOWN;
		(*data)->len = 0;
		(*data)->next_program = NULL;
		(*data)->next_stream = NULL;
		return *data;
	}
	ptr = *data;

	do
	{
		if (ptr->stream_pid == pid)
		{
			return ptr;
		}
		sptr = ptr;
		ptr = ptr->next_stream;
	} while (ptr);

	sptr->next_stream = alloc_demuxer_data();
	ptr = sptr->next_stream;
	ptr->program_number = -1;
	ptr->stream_pid = pid;
	ptr->bufferdatatype = CCX_UNKNOWN;
	ptr->len = 0;
	ptr->next_program = NULL;
	ptr->next_stream = NULL;

	return ptr;
}

struct demuxer_data *get_best_data(struct demuxer_data *data)
{
	struct demuxer_data *ret = NULL;
	struct demuxer_data *ptr = data;
	for (ptr = data; ptr; ptr = ptr->next_stream)
	{
		if (ptr->codec == CCX_CODEC_TELETEXT)
		{
			ret = data;
			goto end;
		}
	}

	for (ptr = data; ptr; ptr = ptr->next_stream)
	{
		if (ptr->codec == CCX_CODEC_DVB)
		{
			ret = ptr;
			goto end;
		}
	}

	for (ptr = data; ptr; ptr = ptr->next_stream)
	{
		if (ptr->codec == CCX_CODEC_ISDB_CC)
		{
			ret = ptr;
			goto end;
		}
	}
	for (ptr = data; ptr; ptr = ptr->next_stream)
	{
		if (ptr->codec == CCX_CODEC_ATSC_CC)
		{
			ret = ptr;
			goto end;
		}
	}
end:

	return ret;
}

int copy_capbuf_demux_data(struct ccx_demuxer *ctx, struct demuxer_data **data, struct cap_info *cinfo)
{
	int vpesdatalen;
	int pesheaderlen;
	unsigned char *databuf;
	long databuflen;
	struct demuxer_data *ptr;

	ptr = search_or_alloc_demuxer_data_node_by_pid(data, cinfo->pid);
	ptr->program_number = cinfo->program_number;
	ptr->codec = cinfo->codec;
	ptr->bufferdatatype = get_buffer_type(cinfo);

	if (!cinfo->capbuf || !cinfo->capbuflen)
		return -1;

	if (cinfo->codec == CCX_CODEC_TELETEXT)
	{
		if (cinfo->capbuflen > BUFSIZE - ptr->len)
		{
			fatal(CCX_COMMON_EXIT_BUG_BUG,
			      "Teletext packet (%" PRId64 ") larger than remaining buffer (%" PRId64 ").\n",
			      cinfo->capbuflen, (int64_t)(BUFSIZE - ptr->len));
		}

		memcpy(ptr->buffer + ptr->len, cinfo->capbuf, cinfo->capbuflen);
		ptr->len += cinfo->capbuflen;
		return CCX_OK;
	}
	vpesdatalen = read_video_pes_header(ctx, ptr, cinfo->capbuf, &pesheaderlen, cinfo->capbuflen);
	if (vpesdatalen < 0 && ccx_options.split_dvb_subs && cinfo->codec == CCX_CODEC_DVB)
	{
		// Fallback: Treat as raw payload if PES header is missing/invalid in split mode
		vpesdatalen = cinfo->capbuflen;
		pesheaderlen = 0;
		dbg_print(CCX_DMT_VERBOSE, "Fallback: Treating broken PES packet as raw DVB payload.\n");
	}

	if (ccx_options.pes_header_to_stdout && cinfo->codec == CCX_CODEC_DVB) // for teletext we have its own header dump
	{
		pes_header_dump(cinfo->capbuf, pesheaderlen);
	}
	if (vpesdatalen < 0)
	{
		// Don't terminate file processing for a single broken PES packet.
		// Just skip this packet and continue with the next one.
		// This commonly occurs in UK Freeview DVB recordings.
		dbg_print(CCX_DMT_VERBOSE, "Skipping broken PES packet (buffer too small or malformed header).\n");
		return CCX_OK;
	}

	if (ccx_options.hauppauge_mode)
	{
		// if (cinfo->pid == 0x104) mprint("DEBUG-HAUP: 0x104 detected\n");
		if (haup_capbuflen % 12 != 0)
			mprint("Warning: Inconsistent Hauppage's buffer length\n");
		if (!haup_capbuflen)
		{
			// Do this so that we always return something until EOF. This will be skipped.
			ptr->buffer[ptr->len++] = 0xFA;
			ptr->buffer[ptr->len++] = 0x80;
			ptr->buffer[ptr->len++] = 0x80;
		}

		for (int i = 0; i < haup_capbuflen; i += 12)
		{
			unsigned haup_stream_id = haup_capbuf[i + 3];
			if (haup_stream_id == 0xbd && haup_capbuf[i + 4] == 0 && haup_capbuf[i + 5] == 6)
			{
				// Because I (CFS) don't have a lot of samples for this, for now I make sure everything is like the one I have:
				// 12 bytes total length, stream id = 0xbd (Private non-video and non-audio), etc
				if (2 > BUFSIZE - ptr->len)
				{
					fatal(CCX_COMMON_EXIT_BUG_BUG,
					      "Remaining buffer (%lld) not enough to hold the 3 Hauppage bytes.\n"
					      "Please send bug report!",
					      BUFSIZE - ptr->len);
				}
				if (haup_capbuf[i + 9] == 1 || haup_capbuf[i + 9] == 2) // Field match. // TODO: If extract==12 this won't work!
				{
					if (haup_capbuf[i + 9] == 1)
						ptr->buffer[ptr->len++] = 4; // Field 1 + cc_valid=1
					else
						ptr->buffer[ptr->len++] = 5; // Field 2 + cc_valid=1
					ptr->buffer[ptr->len++] = haup_capbuf[i + 10];
					ptr->buffer[ptr->len++] = haup_capbuf[i + 11];
				}
				/*
				   if (inbuf>1024) // Just a way to send the bytes to the decoder from time to time, otherwise the buffer will fill up.
				   break;
				   else
				   continue; */
			}
		}
		haup_capbuflen = 0;
	}
	databuf = cinfo->capbuf + pesheaderlen;
	databuflen = cinfo->capbuflen - pesheaderlen;

	if (!ccx_options.hauppauge_mode) // in Haup mode the buffer is filled somewhere else
	{
		if (ptr->len + databuflen >= BUFSIZE)
		{
			mprint("Warning: PES data packet (%ld) larger than remaining buffer (%lld), skipping packet.\n",
			       databuflen, BUFSIZE - ptr->len);
			return CCX_OK;
		}
		memcpy(ptr->buffer + ptr->len, databuf, databuflen);
		ptr->len += databuflen;
	}

	return CCX_OK;
}

void cinfo_cremation(struct ccx_demuxer *ctx, struct demuxer_data **data)
{
	struct cap_info *iter;
	list_for_each_entry(iter, &ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
	{
		copy_capbuf_demux_data(ctx, data, iter);
		freep(&iter->capbuf);
	}
}

int copy_payload_to_capbuf(struct cap_info *cinfo, struct ts_payload *payload)
{

	if (cinfo->ignore == CCX_TRUE &&
	    ((cinfo->stream != CCX_STREAM_TYPE_VIDEO_MPEG2 &&
	      cinfo->stream != CCX_STREAM_TYPE_VIDEO_H264 &&
	      cinfo->stream != CCX_STREAM_TYPE_VIDEO_HEVC) ||
	     !ccx_options.analyze_video_stream))
	{
		// In split DVB mode, allow DVB subtitle packets even if ignored
		if (cinfo->codec != CCX_CODEC_DVB)
		{
			return CCX_OK;
		}
	}

	// Verify PES before copy to capbuf
	if (cinfo->capbuflen == 0)
	{
		if (payload->start[0] != 0x00 || payload->start[1] != 0x00 ||
		    payload->start[2] != 0x01)
		{
			if (ccx_options.split_dvb_subs && cinfo->codec == CCX_CODEC_DVB)
			{
				dbg_print(CCX_DMT_VERBOSE, "Notice: Missing PES header in DVB packet (allowing due to split mode)\n");
			}
			else
			{
				mprint("Notice: Missing PES header\n");
				dump(CCX_DMT_DUMPDEF, payload->start, payload->length, 0, 0);
				cinfo->saw_pesstart = 0;
				errno = EINVAL;
				return -1;
			}
		}
	}

	// copy payload to capbuf
	if (payload->length > INT64_MAX - cinfo->capbuflen)
	{
		mprint("Error: capbuf size overflow\n");
		return -1;
	}
	int64_t newcapbuflen = (int64_t)cinfo->capbuflen + payload->length;
	if (newcapbuflen > (int64_t)cinfo->capbufsize)
	{
		unsigned char *new_capbuf = (unsigned char *)realloc(cinfo->capbuf, (size_t)newcapbuflen);
		if (!new_capbuf)
			return -1;
		cinfo->capbuf = new_capbuf;
		cinfo->capbufsize = newcapbuflen; // Note: capbufsize is int in struct cap_info
	}
	memcpy(cinfo->capbuf + cinfo->capbuflen, payload->start, payload->length);
	cinfo->capbuflen = newcapbuflen; // Note: capbuflen is int in struct cap_info

	return CCX_OK;
}
// Read ts packets until a complete video PES element can be returned.
// The data is read into capbuf and the function returns the number of
// bytes read.

uint64_t get_pts(uint8_t *buffer)
{
	uint64_t pes_prefix;
	uint8_t optional_pes_header_included = NO;
	uint16_t optional_pes_header_length = 0;
	uint64_t pts = 0;

	// Packetized Elementary Stream (PES) 32-bit start code
	pes_prefix = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];

	// check for PES header
	if (pes_prefix == 0x000001)
	{
		// optional PES header marker bits (10.. ....)
		if ((buffer[6] & 0xc0) == 0x80)
		{
			optional_pes_header_included = YES;
			optional_pes_header_length = buffer[8];
		}

		if (optional_pes_header_included == YES && optional_pes_header_length > 0 && (buffer[7] & 0x80) > 0)
		{
			// get associated PTS as it exists
			pts = (buffer[9] & 0x0e);
			pts <<= 29;
			pts |= (buffer[10] << 22);
			pts |= ((buffer[11] & 0xfe) << 14);
			pts |= (buffer[12] << 7);
			pts |= ((buffer[13] & 0xfe) >> 1);
			return pts;
		}
	}
	return UINT64_MAX;
}

// Threshold for enabling packet analysis mode when no PAT is found (in bytes)
#define NO_PAT_THRESHOLD (188 * 1000) // After ~1000 packets

int64_t ts_readstream(struct ccx_demuxer *ctx, struct demuxer_data **data)
{
	int gotpes = 0;
	long pespcount = 0;	      // count packets in PES with captions
	long pcount = 0;	      // count all packets until PES is complete
	int packet_analysis_mode = 0; // If we can't find any packet with CC based from PMT, look for captions in all packets
	int ret = CCX_EAGAIN;
	struct program_info *pinfo = NULL;
	struct cap_info *cinfo;
	struct ts_payload payload;
	int j;
	static int no_pat_warning_shown = 0;

	memset(&payload, 0, sizeof(payload));

	do
	{
		pcount++;

		// Exit the loop at EOF
		ret = ts_readpacket(ctx, &payload);
		if (ret != CCX_OK)
			break;

		// Skip damaged packets, they could do more harm than good
		if (payload.transport_error)
		{
			dbg_print(CCX_DMT_VERBOSE, "Packet (pid %u) skipped - transport error.\n",
				  payload.pid);
			continue;
		}

		// Check for PAT
		if (payload.pid == 0) // This is a PAT
		{
			ts_buffer_psi_packet(ctx);
			if (ctx->PID_buffers[payload.pid] != NULL && ctx->PID_buffers[payload.pid]->buffer_length > 0)
				parse_PAT(ctx); // Returns 1 if there was some data in the buffer already
			continue;
		}

		if (ccx_options.xmltv >= 1 && payload.pid == 0x11)
		{ // This is SDT (or BAT)
			ts_buffer_psi_packet(ctx);
			if (ctx->PID_buffers[payload.pid] != NULL && ctx->PID_buffers[payload.pid]->buffer_length > 0)
				parse_SDT(ctx);
		}

		if (ccx_options.xmltv >= 1 && payload.pid == 0x12) // This is DVB EIT
			parse_EPG_packet(ctx->parent);
		if (ccx_options.xmltv >= 1 && payload.pid >= 0x1000) // This may be ATSC EPG packet
			parse_EPG_packet(ctx->parent);

		for (j = 0; j < ctx->nb_program; j++)
		{
			if (ctx->pinfo[j].analysed_PMT_once == CCX_TRUE &&
			    ctx->pinfo[j].pcr_pid == payload.pid &&
			    payload.have_pcr)
			{
				ctx->last_global_timestamp = ctx->global_timestamp;
				ctx->global_timestamp = (uint32_t)payload.pcr / 90;
				if (!ctx->global_timestamp_inited)
				{
					ctx->min_global_timestamp = ctx->global_timestamp;
					ctx->global_timestamp_inited = 1;
				}
				if (ctx->min_global_timestamp > ctx->global_timestamp)
				{
					ctx->offset_global_timestamp = ctx->last_global_timestamp - ctx->min_global_timestamp;
					ctx->min_global_timestamp = ctx->global_timestamp;
				}
			}
			if (ctx->pinfo[j].pid == payload.pid)
			{
				if (!ctx->PIDs_seen[payload.pid])
					dbg_print(CCX_DMT_PAT, "This PID (%u) is a PMT for program %u.\n", payload.pid, ctx->pinfo[j].program_number);
				pinfo = ctx->pinfo + j;
				break;
			}
		}
		if (j != ctx->nb_program)
		{
			ctx->PIDs_seen[payload.pid] = 2;
			ts_buffer_psi_packet(ctx);
			if (ctx->PID_buffers[payload.pid] != NULL && ctx->PID_buffers[payload.pid]->buffer_length > 0)
				if (parse_PMT(ctx, ctx->PID_buffers[payload.pid]->buffer + 1, ctx->PID_buffers[payload.pid]->buffer_length - 1, pinfo))
					gotpes = 1; // Signals that something changed and that we must flush the buffer
			continue;
		}

		// Enable packet analysis mode if no PAT has been found after reading enough data
		// This handles TS files that don't have PAT/PMT tables (e.g., some DVR recordings)
		if (ctx->nb_program == 0 && ctx->past > NO_PAT_THRESHOLD && !packet_analysis_mode)
		{
			packet_analysis_mode = 1;
			if (!no_pat_warning_shown)
			{
				mprint("\nNo PAT/PMT found after %lld bytes. Enabling packet analysis mode to detect video streams.\n",
				       ctx->past);
				no_pat_warning_shown = 1;
			}
		}

		switch (ctx->PIDs_seen[payload.pid])
		{
			case 0: // First time we see this PID
				if (ctx->PIDs_programs[payload.pid])
				{
					unsigned int st = ctx->PIDs_programs[payload.pid]->printable_stream_type;
					dbg_print(CCX_DMT_PARSE, "\nNew PID found: %u (%s), belongs to program: %u\n", payload.pid,
						  (st < 256) ? desc[st] : "Unknown",
						  ctx->PIDs_programs[payload.pid]->program_number);
					ctx->PIDs_seen[payload.pid] = 2;
				}
				else
				{
					dbg_print(CCX_DMT_PARSE, "\nNew PID found: %u, program number still unknown\n", payload.pid);
					ctx->PIDs_seen[payload.pid] = 1;
				}
				ctx->have_PIDs[ctx->num_of_PIDs] = payload.pid;
				ctx->num_of_PIDs++;
				break;
			case 1: // Saw it before but we didn't know what program it belonged to. Luckier now?
				if (ctx->PIDs_programs[payload.pid])
				{
					unsigned int st = ctx->PIDs_programs[payload.pid]->printable_stream_type;
					dbg_print(CCX_DMT_PARSE, "\nProgram for PID: %u (previously unknown) is: %u (%s)\n", payload.pid,
						  ctx->PIDs_programs[payload.pid]->program_number,
						  (st < 256) ? desc[st] : "Unknown");
					ctx->PIDs_seen[payload.pid] = 2;
				}
				break;
			case 2: // Already seen and reported with correct program
				break;
			case 3: // Already seen, reported, and inspected for CC data (and found some)
				break;
		}

		// PTS calculation
		if (payload.pesstart && payload.length >= 14) // if there is PES Header data in the payload and we didn't get the first pts of that stream
		{
			// Packetized Elementary Stream (PES) 32-bit start code
			uint64_t pes_prefix = (payload.start[0] << 16) | (payload.start[1] << 8) | payload.start[2];
			uint8_t pes_stream_id = payload.start[3];

			uint64_t pts = 0;

			// check for PES header
			if (pes_prefix == 0x000001)
			{
				pts = get_pts(payload.start);
				// keep in mind we already checked if we have this stream id
				// we find the index of the packet PID in the have_PIDs array
				int pid_index = -1;
				for (int i = 0; i < ctx->num_of_PIDs; i++)
					if (payload.pid == ctx->have_PIDs[i])
						pid_index = i;
				if (pid_index != -1)
				{
					ctx->stream_id_of_each_pid[pid_index] = pes_stream_id;
					if (pts < ctx->min_pts[pid_index])
						ctx->min_pts[pid_index] = pts; // and add its packet pts
				}
			}
		}

		if (payload.pid == 8191) // Null packet
			continue;
		if (payload.pid == 1003 && !ctx->hauppauge_warning_shown && !ccx_options.hauppauge_mode)
		{
			// TODO: Change this very weak test for something more decent such as size.
			mprint("\n\nNote: This TS could be a recording from a Hauppage card. If no captions are detected, try --hauppauge\n\n");
			ctx->hauppauge_warning_shown = 1;
		}

		if (ccx_options.hauppauge_mode && payload.pid == HAUPPAGE_CCPID)
		{
			// Haup packets processed separately, because we can't mix payloads. So they go in their own buffer
			// copy payload to capbuf
			int haup_newcapbuflen = haup_capbuflen + payload.length;
			if (haup_newcapbuflen > haup_capbufsize)
			{
				unsigned char *new_haup_capbuf = (unsigned char *)realloc(haup_capbuf, haup_newcapbuflen);
				if (!new_haup_capbuf)
				{
					free(haup_capbuf);
					fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to store hauppauge packets");
				}
				haup_capbuf = new_haup_capbuf;
				haup_capbufsize = haup_newcapbuflen;
			}
			memcpy(haup_capbuf + haup_capbuflen, payload.start, payload.length);
			haup_capbuflen = haup_newcapbuflen;
		}

		// Skip packets with no payload.  This also fixes the problems
		// with the continuity counter not being incremented in empty
		// packets.
		if (!payload.length)
		{
			dbg_print(CCX_DMT_VERBOSE, "Packet (pid %u) skipped - no payload.\n",
				  payload.pid);
			continue;
		}

		cinfo = get_cinfo(ctx, payload.pid);
		cinfo = get_cinfo(ctx, payload.pid);
		if (cinfo == NULL)
		{
			if (!packet_analysis_mode)
				dbg_print(CCX_DMT_PARSE, "Packet (pid %u) skipped - no stream with captions identified yet.\n",
					  payload.pid);
			else
				look_for_caption_data(ctx, &payload);
			continue;
		}
		else if (cinfo->ignore == CCX_TRUE &&
			 ((cinfo->stream != CCX_STREAM_TYPE_VIDEO_MPEG2 &&
			   cinfo->stream != CCX_STREAM_TYPE_VIDEO_H264 &&
			   cinfo->stream != CCX_STREAM_TYPE_VIDEO_HEVC) ||
			  !ccx_options.analyze_video_stream))
		{
			// In split DVB mode, do NOT skip/cleanup DVB streams
			if (ccx_options.split_dvb_subs && cinfo->codec == CCX_CODEC_DVB)
			{
				// Fall through - process this DVB packet
			}
			else
			{
				if (cinfo->codec_private_data)
				{
					switch (cinfo->codec)
					{
						case CCX_CODEC_TELETEXT:
							telxcc_close(&cinfo->codec_private_data, NULL);
							break;
						case CCX_CODEC_DVB:
							dvbsub_close_decoder(&cinfo->codec_private_data);
							break;
						case CCX_CODEC_ISDB_CC:
							delete_isdb_decoder(&cinfo->codec_private_data);
						default:
							break;
					}
					cinfo->codec_private_data = NULL;
				}

				if (cinfo->capbuflen > 0)
				{
					freep(&cinfo->capbuf);
					cinfo->capbufsize = 0;
					cinfo->capbuflen = 0;
					delete_demuxer_data_node_by_pid(data, cinfo->pid);
				}
				continue;
			}
		}

		// Video PES start
		if (payload.pesstart && payload.length >= 6)
		{
			cinfo->saw_pesstart = 1;
			cinfo->prev_counter = payload.counter - 1;
		}

		// Discard packets when no pesstart was found.
		// Exception: DVB in split mode - allow packets to accumulate
		if (!cinfo->saw_pesstart)
		{
			if (cinfo->codec != CCX_CODEC_DVB)
			{
				continue;
			}
		}

		if ((cinfo->prev_counter == 15 ? 0 : cinfo->prev_counter + 1) != payload.counter)
		{
			mprint("TS continuity counter not incremented prev/curr %u/%u\n",
			       cinfo->prev_counter, payload.counter);
		}
		cinfo->prev_counter = payload.counter;

		// If the buffer is empty we just started this function
		if (payload.pesstart && cinfo->capbuflen > 0)
		{
			dbg_print(CCX_DMT_PARSE, "\nPES finished (%ld bytes/%ld PES packets/%ld total packets)\n",
				  cinfo->capbuflen, pespcount, pcount);

			// Keep the data from capbuf to be worked on
			ret = copy_capbuf_demux_data(ctx, data, cinfo);
			cinfo->capbuflen = 0;
			gotpes = 1;
		}

		copy_payload_to_capbuf(cinfo, &payload);
		if (ret < 0)
		{
			if (errno == EINVAL)
				continue;
			else
				break;
		}

		pespcount++;
	} while (!gotpes); // gotpes==1 never arrives here because of the breaks

	for (int i = 0; i < ctx->nb_program; i++)
	{
		pinfo = &ctx->pinfo[i];
		if (!pinfo->has_all_min_pts)
		{
			for (int j = 0; j < ctx->num_of_PIDs; j++)
			{
				if (ctx->PIDs_programs[ctx->have_PIDs[j]])
				{
					if (ctx->PIDs_programs[ctx->have_PIDs[j]]->program_number == pinfo->program_number)
					{
						if (ctx->min_pts[j] != UINT64_MAX)
						{
							if (ctx->stream_id_of_each_pid[j] == 0xbd)
								if (ctx->min_pts[j] < pinfo->got_important_streams_min_pts[PRIVATE_STREAM_1])
									pinfo->got_important_streams_min_pts[PRIVATE_STREAM_1] = ctx->min_pts[j];
							if (ctx->stream_id_of_each_pid[j] >= 0xc0 && ctx->stream_id_of_each_pid[j] <= 0xdf)
								if (ctx->min_pts[j] < pinfo->got_important_streams_min_pts[AUDIO])
									pinfo->got_important_streams_min_pts[AUDIO] = ctx->min_pts[j];
							if (ctx->stream_id_of_each_pid[j] >= 0xe0 && ctx->stream_id_of_each_pid[j] <= 0xef)
								if (ctx->min_pts[j] < pinfo->got_important_streams_min_pts[VIDEO])
									pinfo->got_important_streams_min_pts[VIDEO] = ctx->min_pts[j];
							if (pinfo->got_important_streams_min_pts[PRIVATE_STREAM_1] != UINT64_MAX &&
							    pinfo->got_important_streams_min_pts[AUDIO] != UINT64_MAX &&
							    pinfo->got_important_streams_min_pts[VIDEO] != UINT64_MAX)
								pinfo->has_all_min_pts = 1;
						}
					}
				}
			}
		}
	}

	if (ret == CCX_EOF)
	{
		cinfo_cremation(ctx, data);
	}
	return ret;
}

// TS specific data grabber
int ts_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **data)
{
	int ret = CCX_OK;

	do
	{
		ret = ts_readstream(ctx->demux_ctx, data);
	} while (ret == CCX_EAGAIN);

	return ret;
}
