#include "lib_ccx.h"
#include "ccx_common_option.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "dvb_subtitle_decoder.h"
#include "ccx_encoders_common.h"
#include "activity.h"
#include "utility.h"
#include "ccx_demuxer.h"
#include "file_buffer.h"
#include "ccx_decoders_isdb.h"
#include "ffmpeg_intgr.h"
#include "ccx_gxf.h"
#include "dvd_subtitle_decoder.h"
#include "ccx_demuxer_mxf.h"

int end_of_file = 0; // End of file?

// Program stream specific data grabber
int ps_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata)
{
	int enough = 0;
	int payload_read = 0;
	size_t result;

	static unsigned vpesnum = 0;

	unsigned char nextheader[512]; // Next header in PS
	int falsepack = 0;
	struct demuxer_data *data;

	if (!*ppdata)
	{
		*ppdata = alloc_demuxer_data();
		if (!*ppdata)
			return -1;
		data = *ppdata;
		// TODO Set to dummy, find and set actual value
		data->program_number = 1;
		data->stream_pid = 1;
		data->codec = CCX_CODEC_ATSC_CC;
	}
	else
	{
		data = *ppdata;
	}

	// Read and return the next video PES payload
	do
	{
		if (BUFSIZE - data->len < 500)
		{
			mprint("Less than 500 left\n");
			enough = 1; // Stop when less than 500 bytes are left in buffer
		}
		else
		{
			result = buffered_read(ctx->demux_ctx, nextheader, 6);
			ctx->demux_ctx->past += result;
			if (result != 6)
			{
				// Consider this the end of the show.
				end_of_file = 1;
				break;
			}

			// Search for a header that is not a picture header (nextheader[3]!=0x00)
			while (!(nextheader[0] == 0x00 && nextheader[1] == 0x00 && nextheader[2] == 0x01 && nextheader[3] != 0x00))
			{
				if (!ctx->demux_ctx->strangeheader)
				{
					mprint("\nNot a recognized header. Searching for next header.\n");
					dump(CCX_DMT_GENERIC_NOTICES, nextheader, 6, 0, 0);
					// Only print the message once per loop / unrecognized header
					ctx->demux_ctx->strangeheader = 1;
				}

				unsigned char *newheader;
				// The amount of bytes read into nextheader by the buffered_read above
				int hlen = 6;
				// Find first 0x00
				// If there is a 00 in the first element we need to advance
				// one step as clearly bytes 1,2,3 are wrong
				newheader = (unsigned char *)memchr(nextheader + 1, 0, hlen - 1);
				if (newheader != NULL)
				{
					int atpos = newheader - nextheader;

					memmove(nextheader, newheader, (size_t)(hlen - atpos));
					result = buffered_read(ctx->demux_ctx, nextheader + (hlen - atpos), atpos);
					ctx->demux_ctx->past += result;
					if (result != atpos)
					{
						end_of_file = 1;
						break;
					}
				}
				else
				{
					result = buffered_read(ctx->demux_ctx, nextheader, hlen);
					ctx->demux_ctx->past += result;
					if (result != hlen)
					{
						end_of_file = 1;
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
			ctx->demux_ctx->strangeheader = 0;

			// PACK header
			if (nextheader[3] == 0xBA)
			{
				dbg_print(CCX_DMT_VERBOSE, "PACK header\n");
				result = buffered_read(ctx->demux_ctx, nextheader + 6, 8);
				ctx->demux_ctx->past += result;
				if (result != 8)
				{
					// Consider this the end of the show.
					end_of_file = 1;
					break;
				}

				if ((nextheader[4] & 0xC4) != 0x44 || !(nextheader[6] & 0x04) || !(nextheader[8] & 0x04) || !(nextheader[9] & 0x01) || (nextheader[12] & 0x03) != 0x03)
				{
					// broken pack header
					falsepack = 1;
				}
				// We don't need SCR/SCR_ext
				int stufflen = nextheader[13] & 0x07;

				if (falsepack)
				{
					mprint("Warning: Defective Pack header\n");
				}

				// If not defect, load stuffing
				buffered_skip(ctx->demux_ctx, (int)stufflen);
				ctx->demux_ctx->past += stufflen;
				// fake a result value as something was skipped
				result = 1;
				continue;
			}
			// PES Header
			// Private Stream 1 (non MPEG audio , subpictures)
			else if (nextheader[3] == 0xBD)
			{
				uint16_t packetlength = (nextheader[4] << 8) | nextheader[5];
				int ret, datalen;
				// TODO: read PES Header extension , skipped for now
				//  read_video_pes_header() might do
				buffered_skip(ctx->demux_ctx, 2);
				ctx->demux_ctx->past += 2;
				ret = buffered_read(ctx->demux_ctx, nextheader + 6, 1); // PES header data length (extension)
				ctx->demux_ctx->past += 1;
				if (ret != 1)
				{
					end_of_file = 1;
					break;
				}
				buffered_skip(ctx->demux_ctx, (int)nextheader[6]);
				ctx->demux_ctx->past += (int)nextheader[6];

				// Substream ID
				ret = buffered_read(ctx->demux_ctx, nextheader + 7, 1);
				ctx->demux_ctx->past += 1;
				if (ret != 1)
				{
					end_of_file = 1;
					break;
				}

				datalen = packetlength - 4 - nextheader[6];
				// dbg_print(CCX_DMT_VERBOSE, "datalen :%d packetlen :%" PRIu16 " pes header ext :%d\n", datalen, packetlength, nextheader[6]);

				// Subtitle substream ID 0x20 - 0x39 (32 possible)
				if (nextheader[7] >= 0x20 && nextheader[7] < 0x40)
				{
					dbg_print(CCX_DMT_VERBOSE, "Subtitle found Stream id:%02x\n", nextheader[7]);
					result = buffered_read(ctx->demux_ctx, data->buffer, datalen);
					ctx->demux_ctx->past += datalen;
					if (result != datalen)
					{
						end_of_file = 1;
						break;
					}
					if (result > 0)
					{
						payload_read += (int)result;
					}
					// FIXME: Temporary bypass
					data->bufferdatatype = CCX_DVD_SUBTITLE;

					data->len = result;
					enough = 1;

					continue;
				}
				else
				{
					data->bufferdatatype = CCX_PES;
					// Non Subtitle packet
					buffered_skip(ctx->demux_ctx, datalen);
					ctx->demux_ctx->past += datalen;
					// fake a result value as something was skipped
					result = 1;

					continue;
				}
			}
			// Some PES stream
			else if (nextheader[3] >= 0xBB && nextheader[3] <= 0xDF)
			{
				// System header
				// nextheader[3]==0xBB
				// 0xBD Private 1
				// 0xBE PAdding
				// 0xBF Private 2
				// 0xC0-0DF audio

				unsigned headerlen = nextheader[4] << 8 | nextheader[5];

				dbg_print(CCX_DMT_VERBOSE, "non Video PES (type 0x%2X) - len %u\n",
					  nextheader[3], headerlen);

				// The 15000 here is quite arbitrary, the longest packages I
				// know of are 12302 bytes (Private 1 data in RTL recording).
				if (headerlen > 15000)
				{
					mprint("Suspicious non Video PES (type 0x%2X) - len %u\n",
					       nextheader[3], headerlen);
					mprint("Do not skip over, search for next.\n");
					headerlen = 2;
				}
				data->bufferdatatype = CCX_PES;

				// Skip over it
				buffered_skip(ctx->demux_ctx, (int)headerlen);
				ctx->demux_ctx->past += headerlen;
				// fake a result value as something was skipped
				result = 1;

				continue;
			}
			// Read the next video PES
			else if ((nextheader[3] & 0xf0) == 0xe0)
			{
				int hlen; // Dummy variable, unused
				int ret;
				size_t peslen;
				ret = read_video_pes_header(ctx->demux_ctx, data, nextheader, &hlen, 0);
				if (ret < 0)
				{
					end_of_file = 1;
					break;
				}
				else
				{
					peslen = ret;
				}

				vpesnum++;
				dbg_print(CCX_DMT_VERBOSE, "PES video packet #%u\n", vpesnum);

				// peslen is unsigned since loop would have already broken if it was negative
				int want = (int)((BUFSIZE - data->len) > peslen ? peslen : (BUFSIZE - data->len));

				if (want != peslen)
				{
					mprint("General LOOP: want(%d) != peslen(%d) \n", want, peslen);
					continue;
				}
				if (want == 0) // Found package with header but without payload
				{
					continue;
				}

				data->bufferdatatype = CCX_PES;

				result = buffered_read(ctx->demux_ctx, data->buffer + data->len, want);
				ctx->demux_ctx->past = ctx->demux_ctx->past + result;
				if (result > 0)
				{
					payload_read += (int)result;
				}
				data->len += result;

				if (result != want)
				{ // Not complete - EOF
					end_of_file = 1;
					break;
				}
				enough = 1; // We got one PES
			}
			else
			{
				// If we are here this is an unknown header type
				mprint("Unknown header %02X\n", nextheader[3]);
				ctx->demux_ctx->strangeheader = 1;
			}
		}
	} while (result != 0 && !enough && BUFSIZE != data->len);

	dbg_print(CCX_DMT_VERBOSE, "PES data read: %d\n", payload_read);
	// printf("payload\n");

	if (!payload_read)
		return CCX_EOF;
	return payload_read;
}

// Returns number of bytes read, or CCX_OF for EOF
int general_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **data)
{
	int bytesread = 0;
	int want;
	int result;
	struct demuxer_data *ptr;
	if (!*data)
	{
		*data = alloc_demuxer_data();
		if (!*data)
			return -1;
		ptr = *data;
		// Dummy program numbers
		ptr->program_number = 1;
		ptr->stream_pid = 0x100;
		ptr->codec = CCX_CODEC_ATSC_CC;
	}
	ptr = *data;

	do
	{
		want = (int)(BUFSIZE - ptr->len);
		result = buffered_read(ctx->demux_ctx, ptr->buffer + ptr->len, want); // This is a macro.
		// 'result' HAS the number of bytes read
		ctx->demux_ctx->past = ctx->demux_ctx->past + result;
		ptr->len += result;
		bytesread += (int)result;
	} while (result != 0 && result != want);

	if (!bytesread)
		return CCX_EOF;

	return bytesread;
}
#ifdef WTV_DEBUG
// Hexadecimal dump process
void process_hex(struct lib_ccx_ctx *ctx, char *filename)
{
	size_t max = (size_t)ctx->inputsize + 1; // Enough for the whole thing. Hex dumps are small so we can be lazy here
	char *line = (char *)malloc(max);
	if (!line)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In process_hex: Out of memory allocating line buffer.");
	}
	/* const char *mpeg_header="00 00 01 b2 43 43 01 f8 "; // Always present */
	FILE *fr = fopen(filename, "rt");
	unsigned char *bytes = NULL;
	unsigned byte_count = 0;
	int warning_shown = 0;
	struct demuxer_data *data = alloc_demuxer_data();
	while (fgets(line, max - 1, fr) != NULL)
	{
		char *c1, *c2 = NULL; // Positions for first and second colons
		/* int len; */
		long timing;
		if (line[0] == ';') // Skip comments
			continue;
		c1 = strchr(line, ':');
		if (c1)
			c2 = strchr(c1 + 1, ':');
		if (!c2) // Line doesn't contain what we want
			continue;
		*c1 = 0;
		*c2 = 0;
		/* len=atoi (line); */
		timing = atol(c1 + 2) * (MPEG_CLOCK_FREQ / 1000);
		current_pts = timing;
		if (pts_set == 0)
			pts_set = 1;
		set_fts();
		c2++;
		/*
				if (strlen (c2)==8)
				{
					unsigned char high1=c2[1];
					unsigned char low1=c2[2];
					int value1=hex_to_int (high1,low1);
					unsigned char high2=c2[4];
					unsigned char low2=c2[5];
					int value2=hex_to_int (high2,low2);
					buffer[0]=value1;
					buffer[1]=value2;
					data->windedx =2;
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
		byte_count = strlen(c2) / 3;
		/*
		if (atoi (line)!=byte_count+strlen (mpeg_header)/3) // Number of bytes reported don't match actual contents
			continue;
			*/
		if (atoi(line) != byte_count) // Number of bytes reported don't match actual contents
			continue;
		if (!byte_count) // Nothing to get from this line except timing info, already done.
			continue;
		bytes = (unsigned char *)malloc(byte_count);
		if (!bytes)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In process_hex: Out of memory to store processed hex value.\n");
		for (unsigned i = 0; i < byte_count; i++)
		{
			unsigned char high = c2[0];
			unsigned char low = c2[1];
			int value = hex_to_int(high, low);
			if (value == -1)
				fatal(EXIT_FAILURE, "In process_hex: Incorrect format, unexpected non-hex string.");
			bytes[i] = value;
			c2 += 3;
		}
		memcpy(data->buffer, bytes, byte_count);
		data->len = byte_count;
		process_raw();
		continue;
		// New wtv format, everything else hopefully obsolete

		int ok = 0; // Were we able to process the line?
		// Attempt to detect how the data is encoded.
		// Case 1 (seen in all elderman's samples):
		// 18 : 467 : 00 00 01 b2 43 43 01 f8 03 42 ff fd 54 80 fc 94 2c ff
		// Always 03 after header, then something unknown (seen 42, 43, c2, c3...),
		// then ff, then data with field info, and terminated with ff.
		if (byte_count > 3 && bytes[0] == 0x03 &&
		    bytes[2] == 0xff && bytes[byte_count - 1] == 0xff)
		{
			ok = 1;
			for (unsigned i = 3; i < byte_count - 2; i += 3)
			{
				data->len = 3;
				ctx->buffer[0] = bytes[i];
				ctx->buffer[1] = bytes[i + 1];
				ctx->buffer[2] = bytes[i + 2];
				process_raw_with_field();
			}
		}
		// Case 2 (seen in P2Pfiend samples):
		// Seems to match McPoodle's descriptions on DVD encoding
		else
		{
			unsigned char magic = bytes[0];
			/* unsigned extra_field_flag=magic&1; */
			unsigned caption_count = ((magic >> 1) & 0x1F);
			unsigned filler = ((magic >> 6) & 1);
			/* unsigned pattern=((magic>>7)&1); */
			int always_ff = 1;
			int current_field = 0;
			if (filler == 0 && caption_count * 6 == byte_count - 1) // Note that we are ignoring the extra field for now...
			{
				ok = 1;
				for (unsigned i = 1; i < byte_count - 2; i += 3)
					if (bytes[i] != 0xff)
					{
						// If we only find FF in the first byte then either there's only field 1 data, OR
						// there's alternating field 1 and field 2 data. Don't know how to tell apart. For now
						// let's assume that always FF means alternating.
						always_ff = 0;
						break;
					}

				for (unsigned i = 1; i < byte_count - 2; i += 3)
				{
					inbuf = 3;
					if (always_ff) // Try to tell apart the fields based on the pattern field.
					{
						ctx->buffer[0] = current_field | 4; // | 4 to enable the 'valid' bit
						current_field = !current_field;
					}
					else
						ctx->buffer[0] = bytes[i];

					ctx->buffer[1] = bytes[i + 1];
					ctx->buffer[2] = bytes[i + 2];
					process_raw_with_field();
				}
			}
		}
		if (!ok && !warning_shown)
		{
			warning_shown = 1;
			mprint("\nWarning: This file contains data I can't process: Please submit .hex for analysis!\n");
		}
		free(bytes);
	}
	fclose(fr);
}
#endif
// Raw file process
int raw_loop(struct lib_ccx_ctx *ctx)
{
	LLONG ret;
	struct demuxer_data *data = NULL;
	struct cc_subtitle *dec_sub = NULL;
	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);
	struct lib_cc_decode *dec_ctx = NULL;
	int caps = 0;

	dec_ctx = update_decoder_list(ctx);
	dec_sub = &dec_ctx->dec_sub;

	set_current_pts(dec_ctx->timing, 90);
	set_fts(dec_ctx->timing); // Now set the FTS related variables

	do
	{
		if (terminate_asap)
			break;

		ret = general_get_more_data(ctx, &data);
		if (ret == CCX_EOF)
			break;

		ret = process_raw(dec_ctx, dec_sub, data->buffer, data->len);
		if (dec_sub->got_output)
		{
			caps = 1;
			encode_sub(enc_ctx, dec_sub);
			dec_sub->got_output = 0;
		}

		// int ccblocks = cb_field1;
		add_current_pts(dec_ctx->timing, cb_field1 * 1001 / 30 * (MPEG_CLOCK_FREQ / 1000));
		set_fts(dec_ctx->timing); // Now set the FTS related variables including fts_max

	} while (data->len);
	free(data);
	return caps;
}

/* Process inbuf bytes in buffer holding raw caption data (three byte packets, the first being the field).
 * The number of processed bytes is returned. */
size_t process_raw_with_field(struct lib_cc_decode *dec_ctx, struct cc_subtitle *sub, unsigned char *buffer, size_t len)
{
	unsigned char data[3];
	size_t i;
	data[0] = 0x04; // Field 1
	dec_ctx->current_field = 1;

	for (i = 0; i < len; i = i + 3)
	{
		if (!dec_ctx->saw_caption_block && *(buffer + i) == 0xff && *(buffer + i + 1) == 0xff)
		{
			// Skip broadcast header
		}
		else
		{
			data[0] = buffer[i];
			data[1] = buffer[i + 1];
			data[2] = buffer[i + 2];

			// do_cb increases the cb_field1 counter so that get_fts()
			// is correct.
			do_cb(dec_ctx, data, sub);
		}
	}
	return len;
}

/* Process inbuf bytes in buffer holding raw caption data (two byte packets).
 * The number of processed bytes is returned. */
size_t process_raw(struct lib_cc_decode *ctx, struct cc_subtitle *sub, unsigned char *buffer, size_t len)
{
	unsigned char data[3];
	size_t i;
	data[0] = 0x04; // Field 1

	for (i = 0; i < len; i = i + 2)
	{
		if (!ctx->saw_caption_block && *(buffer + i) == 0xff && *(buffer + i + 1) == 0xff)
		{
			// Skip broadcast header
		}
		else
		{
			data[1] = buffer[i];
			data[2] = buffer[i + 1];

			// do_cb increases the cb_field1 counter so that get_fts()
			// is correct.
			do_cb(ctx, data, sub);
		}
	}
	return len;
}

void delete_datalist(struct demuxer_data *list)
{
	struct demuxer_data *slist = list;

	while (list)
	{
		slist = list;
		list = list->next_stream;
		delete_demuxer_data(slist);
	}
}
int process_data(struct lib_ccx_ctx *ctx, struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct demuxer_data *data_node)
{
	size_t got; // Means 'consumed' from buffer actually
	int ret = 0;
	static LLONG last_pts = 0x01FFFFFFFFLL;
	struct cc_subtitle *dec_sub = &dec_ctx->dec_sub;

	if (dec_ctx->hauppauge_mode)
	{
		got = process_raw_with_field(dec_ctx, dec_sub, data_node->buffer, data_node->len);
		if (dec_ctx->timing->pts_set)
			set_fts(dec_ctx->timing); // Try to fix timing from TS data
	}
	else if (data_node->bufferdatatype == CCX_DVB_SUBTITLE)
	{
		// Check for multi-stream DVB processing
		if (ctx->split_dvb_subs)
		{
			ret = process_dvb_multi_stream(ctx, data_node, dec_sub);
		}
		else
		{
			// Original single-stream processing
			ret = dvbsub_decode(enc_ctx, dec_ctx, data_node->buffer + 2, data_node->len - 2, dec_sub);
		}

		if (ret < 0)
			mprint("Return from DVB decode: %d\n", ret);
		set_fts(dec_ctx->timing);
		got = data_node->len;
	}
	else if (data_node->bufferdatatype == CCX_PES)
	{
		dec_ctx->in_bufferdatatype = CCX_PES;
		got = process_m2v(enc_ctx, dec_ctx, data_node->buffer, data_node->len, dec_sub);
	}
	else if (data_node->bufferdatatype == CCX_DVD_SUBTITLE)
	{
		if (dec_ctx->is_alloc == 0)
		{
			dec_ctx->private_data = init_dvdsub_decode();
			dec_ctx->is_alloc = 1;
		}
		process_spu(dec_ctx, data_node->buffer, data_node->len, dec_sub);
		got = data_node->len;
	}
	else if (data_node->bufferdatatype == CCX_TELETEXT)
	{
		// telxcc_update_gt(dec_ctx->private_data, ctx->demux_ctx->global_timestamp);

		/* Process Teletext packets even when no encoder context exists (e.g. -out=report).
		   This enables tlt_process_pes_packet() to detect subtitle pages by populating
		   the seen_sub_page[] array inside the teletext decoder. */
		int sentence_cap = enc_ctx ? enc_ctx->sentence_cap : 0;

		ret = tlt_process_pes_packet(
		    dec_ctx,
		    data_node->buffer,
		    data_node->len,
		    dec_sub,
		    sentence_cap);

		/* If Teletext decoding fails with invalid data, abort processing */
		if (ret == CCX_EINVAL)
			return ret;

		/* Mark processed byte count */
		got = data_node->len;
	}
	else if (data_node->bufferdatatype == CCX_PRIVATE_MPEG2_CC)
	{
		got = data_node->len; // Do nothing. Still don't know how to process it
	}
	else if (data_node->bufferdatatype == CCX_RAW) // Raw two byte 608 data from DVR-MS/ASF
	{
		// The asf_get_more_data() loop sets current_pts when possible
		if (dec_ctx->timing->pts_set == 0)
		{
			mprint("DVR-MS/ASF file without useful time stamps - count blocks.\n");
			// Otherwise rely on counting blocks
			dec_ctx->timing->current_pts = 12345; // Pick a valid PTS time
			dec_ctx->timing->pts_set = 1;
		}

		if (dec_ctx->timing->current_pts != last_pts)
		{
			// Only initialize the FTS values and reset the cb
			// counters when the PTS is different. This happens frequently
			// with ASF files.

			if (dec_ctx->timing->min_pts == 0x01FFFFFFFFLL)
			{
				// First call
				fts_at_gop_start = 0;
			}
			else
				fts_at_gop_start = get_fts(dec_ctx->timing, dec_ctx->current_field);

			frames_since_ref_time = 0;
			set_fts(dec_ctx->timing);

			last_pts = dec_ctx->timing->current_pts;
		}

		dbg_print(CCX_DMT_VIDES, "PTS: %s (%8u)",
			  print_mstime_static(dec_ctx->timing->current_pts / (MPEG_CLOCK_FREQ / 1000)),
			  (unsigned)(dec_ctx->timing->current_pts));
		dbg_print(CCX_DMT_VIDES, "  FTS: %s\n", print_mstime_static(get_fts(dec_ctx->timing, dec_ctx->current_field)));

		got = process_raw(dec_ctx, dec_sub, data_node->buffer, data_node->len);
	}
	else if (data_node->bufferdatatype == CCX_H264) // H.264 data from TS file
	{
		dec_ctx->in_bufferdatatype = CCX_H264;
		got = process_avc(enc_ctx, dec_ctx, data_node->buffer, data_node->len, dec_sub);
	}
	else if (data_node->bufferdatatype == CCX_RAW_TYPE)
	{
		got = process_raw_with_field(dec_ctx, dec_sub, data_node->buffer, data_node->len);
	}
	else if (data_node->bufferdatatype == CCX_ISDB_SUBTITLE)
	{
		isdbsub_decode(dec_ctx, data_node->buffer, data_node->len, dec_sub);
		got = data_node->len;
	}
	else
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In process_data: datanode->buffer is of unknown data type!");

	if (got > data_node->len)
	{
		mprint("BUG BUG\n");
	}

	//	ctx->demux_ctx->write_es(ctx->demux_ctx, data_node->buffer, (size_t) (data_node->len - got));

	/* Get rid of the bytes we already processed */
	if (data_node)
	{
		if (got > 0)
		{
			memmove(data_node->buffer, data_node->buffer + got, (size_t)(data_node->len - got));
			data_node->len -= got;
		}
	}

	if (data_node->bufferdatatype != CCX_DVB_SUBTITLE && dec_sub->got_output)
	{
		ret = 1;
		encode_sub(enc_ctx, dec_sub);
		dec_sub->got_output = 0;
	}
	return ret;
}

void segment_output_file(struct lib_ccx_ctx *ctx, struct lib_cc_decode *dec_ctx)
{
	LLONG cur_sec;
	LLONG t;
	struct encoder_ctx *enc_ctx;
	int segment_now = 0;

	t = get_fts(dec_ctx->timing, dec_ctx->current_field);
	if (!t && ctx->demux_ctx->global_timestamp_inited)
		t = ctx->demux_ctx->global_timestamp - ctx->demux_ctx->min_global_timestamp;

#if SEGMENT_BY_FILE_TIME
	cur_sec = t / 1000;
#else
	cur_sec = time(NULL);
#endif

	if (ctx->system_start_time == -1)
		ctx->system_start_time = cur_sec;

	cur_sec = cur_sec - ctx->system_start_time;

	if (ctx->out_interval < 1)
		return;

	if (cur_sec / ctx->out_interval > ctx->segment_counter)
	{
		enc_ctx = get_encoder_by_pn(ctx, dec_ctx->program_number);
		// segment_now = 1;
		if (!ctx->segment_on_key_frames_only ||
		    dec_ctx->picture_coding_type == CCX_FRAME_TYPE_I_FRAME) // Segment only when an I-Frame is found
		{
			segment_now = 1;
			enc_ctx->segment_pending = 0;
			enc_ctx->segment_last_key_frame = 0;
		}
		else
		{
			if (enc_ctx->segment_pending) // We already knew we had to segment but can we do it?
			{
				if (dec_ctx->num_key_frames > enc_ctx->segment_last_key_frame)
				{
					// Yes, there's been a key frame since we last checked
					segment_now = 1;
					enc_ctx->segment_pending = 0;
					enc_ctx->segment_last_key_frame = 0;
				}
			}
			else
			{
				// Make a note of current I frame count,
				enc_ctx->segment_pending = 1;
				enc_ctx->segment_last_key_frame = dec_ctx->num_key_frames;
			}
		}
	}

	if (segment_now)
	{
		{
			ctx->segment_counter++;
			if (enc_ctx)
			{
				// list_del(&enc_ctx->list);
				// dinit_encoder(&enc_ctx, t);
				const char *extension = get_file_extension(ccx_options.enc_cfg.write_format);
				// Format: "%s_%06d%s" needs: basefilename + '_' + up to 10 digits + extension + null
				size_t needed_len = strlen(ctx->basefilename) + 1 + 10 + strlen(extension) + 1;
				freep(&ccx_options.enc_cfg.output_filename);
				ccx_options.enc_cfg.output_filename = malloc(needed_len);
				if (!ccx_options.enc_cfg.output_filename)
				{
					fatal(EXIT_NOT_ENOUGH_MEMORY, "In segment handling: Out of memory allocating output filename.");
				}
				snprintf(ccx_options.enc_cfg.output_filename, needed_len, "%s_%06d%s", ctx->basefilename, ctx->segment_counter + 1, extension);
				reset_output_ctx(enc_ctx, &ccx_options.enc_cfg);
			}
		}
	}
}
int process_non_multiprogram_general_loop(struct lib_ccx_ctx *ctx,
					  struct demuxer_data **datalist,
					  struct demuxer_data **data_node,
					  struct lib_cc_decode **dec_ctx,
					  struct encoder_ctx **enc_ctx,
					  uint64_t *min_pts,
					  int ret,
					  int *caps)
{

	struct cap_info *cinfo = NULL;
	// struct encoder_ctx *enc_ctx = NULL;
	//  Find most promising stream: teletex, DVB, ISDB
	int pid = get_best_stream(ctx->demux_ctx);
	if (pid < 0)
	{
		*data_node = get_best_data(*datalist);
	}
	else
	{
		ignore_other_stream(ctx->demux_ctx, pid);
		*data_node = get_data_stream(*datalist, pid);
	}

	if (ccx_options.analyze_video_stream)
	{
		int video_pid = get_video_stream(ctx->demux_ctx);
		if (video_pid != pid && video_pid != -1)
		{
			struct cap_info *cinfo_video = get_cinfo(ctx->demux_ctx, pid);
			struct lib_cc_decode *dec_ctx_video = update_decoder_list_cinfo(ctx, cinfo_video);
			*enc_ctx = update_encoder_list_cinfo(ctx, cinfo_video);
			struct cc_subtitle *dec_sub_video = &dec_ctx_video->dec_sub;
			struct demuxer_data *data_node_video = get_data_stream(*datalist, video_pid);

			if (data_node_video)
			{
				if (data_node_video->pts != CCX_NOPTS)
				{
					struct ccx_rational tb = {1, MPEG_CLOCK_FREQ};
					LLONG pts;
					if (data_node_video->tb.num != 1 || data_node_video->tb.den != MPEG_CLOCK_FREQ)
					{
						pts = change_timebase(data_node_video->pts, data_node_video->tb, tb);
					}
					else
					{
						pts = data_node_video->pts;
					}

					set_current_pts(dec_ctx_video->timing, pts);
					set_fts(dec_ctx_video->timing);
				}
				size_t got = process_m2v(*enc_ctx, dec_ctx_video, data_node_video->buffer, data_node_video->len, dec_sub_video);
				if (got > 0)
				{
					memmove(data_node_video->buffer, data_node_video->buffer + got, (size_t)(data_node_video->len - got));
					data_node_video->len -= got;
				}
			}
		}
	}

	cinfo = get_cinfo(ctx->demux_ctx, pid);
	*enc_ctx = update_encoder_list_cinfo(ctx, cinfo);
	*dec_ctx = update_decoder_list_cinfo(ctx, cinfo);
	(*dec_ctx)->dtvcc->encoder = (void *)(*enc_ctx);

	if ((*dec_ctx)->timing->min_pts == 0x01FFFFFFFFLL) // if we didn't set the min_pts of the program
	{
		int p_index = 0; // program index
		for (int i = 0; i < ctx->demux_ctx->nb_program; i++)
		{
			if ((*dec_ctx)->program_number == ctx->demux_ctx->pinfo[i].program_number)
			{
				p_index = i;
				break;
			}
		}

		if ((*dec_ctx)->codec == CCX_CODEC_TELETEXT) // even if there's no sub data, we still need to set the min_pts
		{
			if (ctx->demux_ctx->pinfo[p_index].got_important_streams_min_pts[PRIVATE_STREAM_1] != UINT64_MAX) // Teletext is synced with subtitle packet PTS
			{
				*min_pts = ctx->demux_ctx->pinfo[p_index].got_important_streams_min_pts[PRIVATE_STREAM_1];
				set_current_pts((*dec_ctx)->timing, *min_pts);
				set_fts((*dec_ctx)->timing);
			}
		}
		if ((*dec_ctx)->codec == CCX_CODEC_DVB) // DVB will always have to be in sync with audio (no matter the min_pts of the other streams)
		{
			if (ctx->demux_ctx->pinfo[p_index].got_important_streams_min_pts[AUDIO] != UINT64_MAX) // it means we got the first pts for audio
			{
				*min_pts = ctx->demux_ctx->pinfo[p_index].got_important_streams_min_pts[AUDIO];
				set_current_pts((*dec_ctx)->timing, *min_pts);
				// For DVB subtitles, we need to directly set min_pts because set_fts()
				// relies on video frame type detection which doesn't work for DVB-only streams.
				// This fixes negative subtitle timestamps.
				if ((*dec_ctx)->timing->min_pts == 0x01FFFFFFFFLL)
				{
					(*dec_ctx)->timing->min_pts = *min_pts;
					(*dec_ctx)->timing->pts_set = 2; // MinPtsSet
					(*dec_ctx)->timing->sync_pts = *min_pts;
				}
				set_fts((*dec_ctx)->timing);
			}
		}
	}

	if (*enc_ctx)
		(*enc_ctx)->timing = (*dec_ctx)->timing;

	if (*data_node) // no sub data, no need to process non-existing data
	{
		if ((*data_node)->pts != CCX_NOPTS)
		{
			struct ccx_rational tb = {1, MPEG_CLOCK_FREQ};
			LLONG pts;
			if ((*data_node)->tb.num != 1 || (*data_node)->tb.den != MPEG_CLOCK_FREQ)
			{
				pts = change_timebase((*data_node)->pts, (*data_node)->tb, tb);
			}
			else
				pts = (*data_node)->pts;
			set_current_pts((*dec_ctx)->timing, pts);
			// For DVB subtitles, use the first subtitle PTS as min_pts if audio hasn't been seen yet
			if ((*dec_ctx)->codec == CCX_CODEC_DVB && (*dec_ctx)->timing->min_pts == 0x01FFFFFFFFLL)
			{
				(*dec_ctx)->timing->min_pts = pts;
				(*dec_ctx)->timing->pts_set = 2; // MinPtsSet
				(*dec_ctx)->timing->sync_pts = pts;
			}
			set_fts((*dec_ctx)->timing);
		}

		if ((*data_node)->bufferdatatype == CCX_ISDB_SUBTITLE)
		{
			uint64_t tstamp;
			if (ctx->demux_ctx->global_timestamp_inited)
			{
				tstamp = (ctx->demux_ctx->global_timestamp + ctx->demux_ctx->offset_global_timestamp) - ctx->demux_ctx->min_global_timestamp;
			}
			else
			{
				// Fix if global timestamp not inited
				tstamp = get_fts((*dec_ctx)->timing, (*dec_ctx)->current_field);
			}
			isdb_set_global_time(*dec_ctx, tstamp);
		}
		if ((*data_node)->bufferdatatype == CCX_TELETEXT && (*dec_ctx)->private_data) // if we have teletext subs, we set the min_pts here
			set_tlt_delta(*dec_ctx, (*dec_ctx)->timing->current_pts);
		ret = process_data(ctx, *enc_ctx, *dec_ctx, *data_node);
		if (*enc_ctx != NULL)
		{
			if ((*enc_ctx)->srt_counter || (*enc_ctx)->cea_708_counter || (*dec_ctx)->saw_caption_block || ret == 1)
				*caps = 1;
		}

		// Process the last subtitle for DVB
		if (!(!terminate_asap && !end_of_file && is_decoder_processed_enough(ctx) == CCX_FALSE))
		{
			if ((*data_node)->bufferdatatype == CCX_DVB_SUBTITLE && (*dec_ctx)->dec_sub.prev->end_time == 0)
			{
				// Use get_fts() which properly handles PTS jumps and maintains monotonic timing
				(*dec_ctx)->dec_sub.prev->end_time = get_fts((*dec_ctx)->timing, (*dec_ctx)->current_field);
				if ((*enc_ctx) != NULL)
					encode_sub((*enc_ctx)->prev, (*dec_ctx)->dec_sub.prev);
				(*dec_ctx)->dec_sub.prev->got_output = 0;
			}
		}
	}
	return ret;
}

int general_loop(struct lib_ccx_ctx *ctx)
{
	struct lib_cc_decode *dec_ctx = NULL;
	enum ccx_stream_mode_enum stream_mode;
	struct demuxer_data *datalist = NULL;
	struct demuxer_data *data_node = NULL;
	int (*get_more_data)(struct lib_ccx_ctx *c, struct demuxer_data **d);
	int ret;
	int caps = 0;

	uint64_t min_pts = UINT64_MAX;

	stream_mode = ctx->demux_ctx->get_stream_mode(ctx->demux_ctx);

	if (stream_mode == CCX_SM_TRANSPORT && ctx->write_format == CCX_OF_NULL)
		ctx->multiprogram = 1;

	switch (stream_mode)
	{
		case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
			get_more_data = &general_get_more_data;
			break;
		case CCX_SM_TRANSPORT:
			get_more_data = &ts_get_more_data;
			break;
		case CCX_SM_PROGRAM:
			get_more_data = &ps_get_more_data;
			break;
		case CCX_SM_ASF:
			get_more_data = &asf_get_more_data;
			break;
		case CCX_SM_WTV:
			get_more_data = &wtv_get_more_data;
			break;
		case CCX_SM_GXF:
			get_more_data = &ccx_gxf_get_more_data;
			break;
#ifdef ENABLE_FFMPEG
		case CCX_SM_FFMPEG:
			get_more_data = &ffmpeg_get_more_data;
			break;
#endif
		case CCX_SM_MXF:
			get_more_data = ccx_mxf_getmoredata;
			// The MXFContext might have already been initialized if the
			// stream type was autodetected
			if (ctx->demux_ctx->private_data == NULL)
			{
				ctx->demux_ctx->private_data = ccx_gxf_init(ctx->demux_ctx);
			}
			break;
		default:
			fatal(CCX_COMMON_EXIT_BUG_BUG, "In general_loop: Impossible value for stream_mode");
	}

	end_of_file = 0;

	while (!terminate_asap && !end_of_file && is_decoder_processed_enough(ctx) == CCX_FALSE)
	{
		// GET MORE DATA IN BUFFER
		position_sanity_check(ctx->demux_ctx);
		ret = get_more_data(ctx, &datalist);
		if (ret == CCX_EOF)
		{
			end_of_file = 1;
		}
		if (!datalist)
			continue;
		position_sanity_check(ctx->demux_ctx);
		if (!ctx->multiprogram)
		{
			struct encoder_ctx *enc_ctx = NULL;
			int ret = process_non_multiprogram_general_loop(ctx,
									&datalist,
									&data_node,
									&dec_ctx,
									&enc_ctx,
									&min_pts,
									ret,
									&caps);
			if (ret == CCX_EINVAL)
			{
				break;
			}
		}
		else
		{
			struct cap_info *cinfo = NULL;
			struct cap_info *program_iter;
			struct cap_info *ptr = &ctx->demux_ctx->cinfo_tree;
			struct encoder_ctx *enc_ctx = NULL;
			list_for_each_entry(program_iter, &ptr->pg_stream, pg_stream, struct cap_info)
			{
				cinfo = get_best_sib_stream(program_iter);
				if (!cinfo)
				{
					data_node = get_best_data(datalist);
				}
				else
				{
					ignore_other_sib_stream(program_iter, cinfo->pid);
					data_node = get_data_stream(datalist, cinfo->pid);
				}

				enc_ctx = update_encoder_list_cinfo(ctx, cinfo);
				dec_ctx = update_decoder_list_cinfo(ctx, cinfo);
				dec_ctx->dtvcc->encoder = (void *)enc_ctx; // WARN: otherwise cea-708 will not work

				if (dec_ctx->timing->min_pts == 0x01FFFFFFFFLL) // if we didn't set the min_pts of the program
				{
					int p_index = 0; // program index
					for (int i = 0; i < ctx->demux_ctx->nb_program; i++)
					{
						if (dec_ctx->program_number == ctx->demux_ctx->pinfo[i].program_number)
						{
							p_index = i;
							break;
						}
					}

					if (dec_ctx->codec == CCX_CODEC_TELETEXT) // even if there's no sub data, we still need to set the min_pts
					{
						if (ctx->demux_ctx->pinfo[p_index].got_important_streams_min_pts[PRIVATE_STREAM_1] != UINT64_MAX) // Teletext is synced with subtitle packet PTS
						{
							min_pts = ctx->demux_ctx->pinfo[p_index].got_important_streams_min_pts[PRIVATE_STREAM_1]; // it means we got the first pts for private stream 1
							set_current_pts(dec_ctx->timing, min_pts);
							set_fts(dec_ctx->timing);
						}
					}
					if (dec_ctx->codec == CCX_CODEC_DVB) // DVB will always have to be in sync with audio (no matter the min_pts of the other streams)
					{
						if (ctx->demux_ctx->pinfo[p_index].got_important_streams_min_pts[AUDIO] != UINT64_MAX) // it means we got the first pts for audio
						{
							min_pts = ctx->demux_ctx->pinfo[p_index].got_important_streams_min_pts[AUDIO];
							set_current_pts(dec_ctx->timing, min_pts);
							// For DVB subtitles, directly set min_pts to fix negative timestamps
							if (dec_ctx->timing->min_pts == 0x01FFFFFFFFLL)
							{
								dec_ctx->timing->min_pts = min_pts;
								dec_ctx->timing->pts_set = 2; // MinPtsSet
								dec_ctx->timing->sync_pts = min_pts;
							}
							set_fts(dec_ctx->timing);
						}
					}
				}

				if (enc_ctx)
					enc_ctx->timing = dec_ctx->timing;

				if (!data_node)
					continue;

				if (data_node->pts != CCX_NOPTS)
				{
					set_current_pts(dec_ctx->timing, data_node->pts);
					// For DVB subtitles, use the first subtitle PTS as min_pts if audio hasn't been seen yet
					if (dec_ctx->codec == CCX_CODEC_DVB && dec_ctx->timing->min_pts == 0x01FFFFFFFFLL)
					{
						dec_ctx->timing->min_pts = data_node->pts;
						dec_ctx->timing->pts_set = 2; // MinPtsSet
						dec_ctx->timing->sync_pts = data_node->pts;
					}
				}

				ret = process_data(ctx, enc_ctx, dec_ctx, data_node);
				if (enc_ctx != NULL)
				{
					if (
					    ((enc_ctx && (enc_ctx->srt_counter || enc_ctx->cea_708_counter)) ||
					     dec_ctx->saw_caption_block || ret == 1))
						caps = 1;
				}
				// Process the last subtitle for DVB
				if (!(!terminate_asap && !end_of_file && is_decoder_processed_enough(ctx) == CCX_FALSE))
				{
					if (data_node->bufferdatatype == CCX_DVB_SUBTITLE && dec_ctx && dec_ctx->dec_sub.prev && dec_ctx->dec_sub.prev->end_time == 0)
					{
						dec_ctx->dec_sub.prev->end_time = (dec_ctx->timing->current_pts - dec_ctx->timing->min_pts) / (MPEG_CLOCK_FREQ / 1000);
						if (enc_ctx != NULL)
							encode_sub(enc_ctx->prev, dec_ctx->dec_sub.prev);
						dec_ctx->dec_sub.prev->got_output = 0;
					}
				}
			}
			if (!data_node)
				continue;
		}
		if (ctx->live_stream)
		{
			int cur_sec = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);
			int th = cur_sec / 10;
			if (ctx->last_reported_progress != th)
			{
				activity_progress(-1, cur_sec / 60, cur_sec % 60);
				ctx->last_reported_progress = th;
			}
		}
		else
		{
			if (ctx->total_inputsize > 255) // Less than 255 leads to division by zero below.
			{
				int progress = (int)((((ctx->total_past + ctx->demux_ctx->past) >> 8) * 100) / (ctx->total_inputsize >> 8));
				if (ctx->last_reported_progress != progress)
				{
					LLONG t = get_fts(dec_ctx->timing, dec_ctx->current_field);
					if (!t && ctx->demux_ctx->global_timestamp_inited)
						t = ctx->demux_ctx->global_timestamp - ctx->demux_ctx->min_global_timestamp;
					int cur_sec = (int)(t / 1000);
					activity_progress(progress, cur_sec / 60, cur_sec % 60);
					ctx->last_reported_progress = progress;
				}
			}
		}

		// void segment_output_file(struct lib_ccx_ctx *ctx, struct lib_cc_decode *dec_ctx);
		segment_output_file(ctx, dec_ctx);

		if (ccx_options.send_to_srv)
			net_check_conn();
	}

	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);

	list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
	{

		if (dec_ctx->codec == CCX_CODEC_TELETEXT)
			telxcc_close(&dec_ctx->private_data, &dec_ctx->dec_sub);
		// Flush remaining HD captions
		if (dec_ctx->has_ccdata_buffered)
			process_hdcc(enc_ctx, dec_ctx, &dec_ctx->dec_sub);

		mprint("\nNumber of NAL_type_7: %ld\n", dec_ctx->avc_ctx->num_nal_unit_type_7);
		mprint("Number of VCL_HRD: %ld\n", dec_ctx->avc_ctx->num_vcl_hrd);
		mprint("Number of NAL HRD: %ld\n", dec_ctx->avc_ctx->num_nal_hrd);
		mprint("Number of jump-in-frames: %ld\n", dec_ctx->avc_ctx->num_jump_in_frames);
		mprint("Number of num_unexpected_sei_length: %ld", dec_ctx->avc_ctx->num_unexpected_sei_length);
		free(dec_ctx->xds_ctx);
	}

	delete_datalist(datalist);
	if (ctx->total_past != ctx->total_inputsize && ctx->binary_concat && is_decoder_processed_enough(ctx))
	{
		mprint("\n\n\n\nATTENTION!!!!!!\n");
		mprint("Processing of %s %d ended prematurely %lld < %lld, please send bug report.\n\n",
		       ctx->inputfile[ctx->current_file], ctx->current_file, ctx->demux_ctx->past, ctx->inputsize);
	}
	return caps;
}

// Raw caption with FTS file process
int rcwt_loop(struct lib_ccx_ctx *ctx)
{
	unsigned char *parsebuf;
	long parsebufsize = 1024;
	unsigned char buf[TELETEXT_CHUNK_LEN] = "";
	struct lib_cc_decode *dec_ctx = NULL;
	struct cc_subtitle *dec_sub = NULL;
	LLONG currfts;
	uint16_t cbcount = 0;
	int bread = 0; // Bytes read
	int caps = 0;
	LLONG result;
	struct encoder_ctx *enc_ctx = update_encoder_list(ctx);
	struct TeletextCtx *telctx;
	// As BUFSIZE is a macro this is just a reminder
	if (BUFSIZE < (3 * 0xFFFF + 10))
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In rcwt_loop: BUFSIZE too small for RCWT caption block.\n");

	// Generic buffer to hold some data
	parsebuf = (unsigned char *)malloc(1024);
	if (!parsebuf)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In rcwt_loop: Out of memory allocating parsebuf.");
	}

	result = buffered_read(ctx->demux_ctx, parsebuf, 11);
	ctx->demux_ctx->past += result;
	bread += (int)result;
	if (result != 11)
	{
		mprint("Premature end of file!\n");
		end_of_file = 1;
		free(parsebuf);
		return -1;
	}

	// Expecting RCWT header
	if (!memcmp(parsebuf, "\xCC\xCC\xED", 3))
	{
		dbg_print(CCX_DMT_PARSE, "\nRCWT header\n");
		dbg_print(CCX_DMT_PARSE, "File created by %02X version %02X%02X\nFile format revision: %02X%02X\n",
			  parsebuf[3], parsebuf[4], parsebuf[5],
			  parsebuf[6], parsebuf[7]);
	}
	else
	{
		fatal(EXIT_MISSING_RCWT_HEADER, "In rcwt_loop: Missing RCWT header. Abort.\n");
	}

	dec_ctx = update_decoder_list(ctx);
	dec_ctx->dtvcc->encoder = (void *)enc_ctx; // WARN: otherwise cea-708 will not work
	if (parsebuf[6] == 0 && parsebuf[7] == 2)
	{
		dec_ctx->codec = CCX_CODEC_TELETEXT;
		dec_ctx->private_data = telxcc_init();
	}
	dec_sub = &dec_ctx->dec_sub;
	telctx = dec_ctx->private_data;

	/* Set minimum and current pts since rcwt has correct time */
	dec_ctx->timing->min_pts = 0;
	dec_ctx->timing->current_pts = 0;

	// Loop until no more data is found
	while (1)
	{
		if (parsebuf[6] == 0 && parsebuf[7] == 2)
		{
			result = buffered_read(ctx->demux_ctx, buf, TELETEXT_CHUNK_LEN);
			ctx->demux_ctx->past += result;
			if (result != TELETEXT_CHUNK_LEN)
			{
				telxcc_dump_prev_page(telctx, dec_sub);
				break;
			}
			tlt_read_rcwt(dec_ctx->private_data, buf, dec_sub);
			if (dec_sub->got_output == CCX_TRUE)
			{
				encode_sub(enc_ctx, dec_sub);
				dec_sub->got_output = 0;
				caps = 1;
			}
			continue;
		}

		// Read the data header
		result = buffered_read(ctx->demux_ctx, parsebuf, 10);
		ctx->demux_ctx->past += result;
		bread += (int)result;

		if (result != 10)
		{
			if (result != 0)
				mprint("Premature end of file!\n");

			// We are done
			end_of_file = 1;
			break;
		}
		currfts = *((LLONG *)(parsebuf));
		cbcount = *((uint16_t *)(parsebuf + 8));

		dbg_print(CCX_DMT_PARSE, "RCWT data header FTS: %s  blocks: %u\n",
			  print_mstime_static(currfts), cbcount);

		if (cbcount > 0)
		{
			if (cbcount * 3 > parsebufsize)
			{
				unsigned char *new_parsebuf = (unsigned char *)realloc(parsebuf, cbcount * 3);
				if (!new_parsebuf)
				{
					free(parsebuf);
					fatal(EXIT_NOT_ENOUGH_MEMORY, "In rcwt_loop: Out of memory allocating parsebuf.");
				}
				parsebuf = new_parsebuf;
				parsebufsize = cbcount * 3;
			}
			result = buffered_read(ctx->demux_ctx, parsebuf, cbcount * 3);
			ctx->demux_ctx->past += result;
			bread += (int)result;
			if (result != cbcount * 3)
			{
				mprint("Premature end of file!\n");
				end_of_file = 1;
				break;
			}

			// Process the data
			set_current_pts(dec_ctx->timing, currfts * (MPEG_CLOCK_FREQ / 1000));
			set_fts(dec_ctx->timing); // Now set the FTS related variables

			for (int j = 0; j < cbcount * 3; j = j + 3)
			{
				do_cb(dec_ctx, parsebuf + j, dec_sub);
			}
		}
		if (dec_sub->got_output)
		{
			caps = 1;
			encode_sub(enc_ctx, dec_sub);
			dec_sub->got_output = 0;
		}
	} // end while(1)

	dbg_print(CCX_DMT_PARSE, "Processed %d bytes\n", bread);
	free(parsebuf);
	return caps;
}
