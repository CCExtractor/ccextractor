#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "activity.h"

// Functions to parse a mpeg-2 data stream, see ISO/IEC 13818-2 6.2
static uint8_t search_start_code(struct bitstream *esstream);
static uint8_t next_start_code(struct bitstream *esstream);
static int es_video_sequence(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *esstream, struct cc_subtitle *sub);
static int read_seq_info(struct lib_cc_decode *ctx, struct bitstream *esstream);
static int sequence_header(struct lib_cc_decode *ctx, struct bitstream *esstream);
static int sequence_ext(struct lib_cc_decode *ctx, struct bitstream *esstream);
static int read_gop_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *esstream, struct cc_subtitle *sub);
static int gop_header(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *esstream, struct cc_subtitle *sub);
static int read_pic_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *esstream, struct cc_subtitle *sub);
static int pic_header(struct lib_cc_decode *ctx, struct bitstream *esstream);
static int pic_coding_ext(struct lib_cc_decode *ctx, struct bitstream *esstream);
static int read_eau_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *esstream, int udtype, struct cc_subtitle *sub);
static int extension_and_user_data(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *esstream, int udtype, struct cc_subtitle *sub);
static int read_pic_data(struct bitstream *esstream);

#define debug(...) ccx_common_logging.debug_ftn(CCX_DMT_VERBOSE, __VA_ARGS__)
/* Process a mpeg-2 data stream with "length" bytes in buffer "data".
 * The number of processed bytes is returned.
 * Defined in ISO/IEC 13818-2 6.2 */
size_t process_m2v(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, unsigned char *data, size_t length, struct cc_subtitle *sub)
{
	if (length < 8) // Need to look ahead 8 bytes
		return length;

	// Init bitstream
	struct bitstream esstream;
	init_bitstream(&esstream, data, data + length);

	// Process data. The return value is ignored as esstream.pos holds
	// the information how far the parsing progressed.
	es_video_sequence(enc_ctx, dec_ctx, &esstream, sub);

	// This returns how many bytes were processed and can therefore
	// be discarded from "buffer". "esstream.pos" points to the next byte
	// where processing will continue.
	return (LLONG)(esstream.pos - data);
}

// Return the next startcode or sequence_error_code if not enough
// data was left in the bitstream. Also set esstream->bitsleft.
// The bitstream pointer shall be moved to the begin of the start
// code if found, or to the position where a search would continue
// would more data be made available.
// This function discards all data until the start code is
// found
static uint8_t search_start_code(struct bitstream *esstream)
{
	make_byte_aligned(esstream);

	// Keep a negative esstream->bitsleft, but correct it.
	if (esstream->bitsleft <= 0)
	{
		debug("search_start_code: bitsleft <= 0\n");
		esstream->bitsleft -= 8 * 4;
		return 0xB4;
	}

	unsigned char *tstr = esstream->pos;

	// Scan for 0x000001xx in header
	while (1)
	{
		// The loop is exited by break only, and if tstr+3 is less than
		// esstream->end we found the start code prefix
		tstr = (unsigned char *)memchr(tstr, 0x00,
					       esstream->end - tstr);
		if (tstr == NULL)
		{
			// We don't even have the starting 0x00
			tstr = esstream->end;
			esstream->bitsleft = -8 * 4;
			break;
		}
		else if (tstr + 3 >= esstream->end)
		{
			// Not enough bytes left to check for 0x000001??
			esstream->bitsleft = 8 * (esstream->end - (tstr + 4));
			break;
		}
		else if (tstr[1] == 0x00 && tstr[2] == 0x01)
		{
			// Found 0x000001??
			esstream->bitsleft = 8 * (esstream->end - (tstr + 4));
			break;
		}
		// Keep searching
		tstr++;
	}
	esstream->pos = tstr;
	if (esstream->bitsleft < 0)
	{
		debug("search_start_code: bitsleft <= 0\n");
		return 0xB4;
	}
	else
	{
		debug("search_start_code: Found %02X\n", tstr[3]);
		return tstr[3];
	}
}

// Return the next startcode or sequence_error_code if not enough
// data was left in the bitstream. Also set esstream->bitsleft.
// The bitstream pointer shall be moved to the begin of the start
// code if found, or to the position where a search would continue
// would more data be made available.
// Only NULL bytes before the start code are discarded, if a non
// NULL byte is encountered esstream->error is set to TRUE and the
// function returns sequence_error_code with the pointer set after
// that byte.
static uint8_t next_start_code(struct bitstream *esstream)
{
	if (esstream->error || esstream->bitsleft < 0)
	{
		return 0xB4;
	}

	make_byte_aligned(esstream);

	// Only start looking if there is enough data. Adjust bitsleft.
	if (esstream->bitsleft < 4 * 8)
	{
		debug("next_start_code: bitsleft %lld < 32\n", esstream->bitsleft);
		esstream->bitsleft -= 8 * 4;
		return 0xB4;
	}

	uint8_t tmp;
	while ((next_u32(esstream) & 0x00FFFFFF) != 0x00010000 // LSB 0x000001??
	       && esstream->bitsleft > 0)
	{
		tmp = read_u8(esstream);
		if (tmp)
		{
			debug("next_start_code: Non zero stuffing\n");
			esstream->error = 1;
			return 0xB4;
		}
	}

	if (esstream->bitsleft < 8)
	{
		esstream->bitsleft -= 8;
		debug("next_start_code: bitsleft <= 0\n");
		return 0xB4;
	}
	else
	{
		debug("next_start_code: Found %02X\n", *(esstream->pos + 3));

		if (*(esstream->pos + 3) == 0xB4)
		{
			debug("B4: assume bitstream syntax error!\n");
			esstream->error = 1;
		}

		return *(esstream->pos + 3);
	}
}

// Return TRUE if the video sequence was finished, FALSE
// Otherwise.  estream->pos shall point to the position where
// the next call will continue, i.e. the possible begin of an
// unfinished video sequence or after the finished sequence.
static int es_video_sequence(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, struct cc_subtitle *sub)
{
	// Avoid "Skip forward" message on first call and later only
	// once per search.
	static int noskipmessage = 1;
	uint8_t startcode;

	debug("es_video_sequence()\n");

	esstream->error = 0;

	// Analyze sequence header ...
	if (!dec_ctx->no_bitstream_error)
	{
		// We might start here because of a syntax error. Discard
		// all data until a new sequence_header_code or group_start_code
		// is found.

		if (!noskipmessage) // Avoid unnecessary output.
			mprint("\nSkip forward to the next Sequence or GOP start.\n");
		else
			noskipmessage = 0;

		uint8_t startcode;
		while (1)
		{
			// search_start_code() cannot produce esstream->error
			startcode = search_start_code(esstream);
			if (esstream->bitsleft < 0)
			{
				noskipmessage = 1;
				return 0;
			}

			if (startcode == 0xB3 || startcode == 0xB8) // found it
				break;

			skip_bits(esstream, 4 * 8);
		}

		dec_ctx->no_bitstream_error = 1;
		dec_ctx->saw_seqgoppic = 0;
		dec_ctx->in_pic_data = 0;
	}

	do
	{
		startcode = next_start_code(esstream);

		debug("\nM2V - next start code %02X %d\n", startcode, dec_ctx->in_pic_data);

		// Syntax check - also returns on bitsleft < 0
		if (startcode == 0xB4)
		{
			if (esstream->error)
			{
				dec_ctx->no_bitstream_error = 0;
				debug("es_video_sequence: syntax problem.\n");
			}

			debug("es_video_sequence: return on B4 startcode.\n");

			return 0;
		}

		// Sequence_end_code
		if (startcode == 0xB7)
		{
			skip_u32(esstream); // Advance bitstream
			dec_ctx->no_bitstream_error = 0;
			break;
		}
		// Sequence header
		if (!dec_ctx->in_pic_data && startcode == 0xB3)
		{
			if (!read_seq_info(dec_ctx, esstream))
			{
				if (esstream->error)
					dec_ctx->no_bitstream_error = 0;
				return 0;
			}
			dec_ctx->saw_seqgoppic = 1;
			continue;
		}
		// Group of pictures
		if (!dec_ctx->in_pic_data && startcode == 0xB8)
		{
			if (!read_gop_info(enc_ctx, dec_ctx, esstream, sub))
			{
				if (esstream->error)
					dec_ctx->no_bitstream_error = 0;
				return 0;
			}
			dec_ctx->saw_seqgoppic = 2;
			continue;
		}
		// Picture
		if (!dec_ctx->in_pic_data && startcode == 0x00)
		{
			if (!read_pic_info(enc_ctx, dec_ctx, esstream, sub))
			{
				if (esstream->error)
					dec_ctx->no_bitstream_error = 0;
				return 0;
			}
			dec_ctx->saw_seqgoppic = 3;
			dec_ctx->in_pic_data = 1;
			continue;
		}

		// Only looks for extension and user data if we saw sequence, gop
		// or picture info before.
		// This check needs to be before the "dec_ctx->in_pic_data" part.
		if (dec_ctx->saw_seqgoppic && (startcode == 0xB2 || startcode == 0xB5))
		{
			if (!read_eau_info(enc_ctx, dec_ctx, esstream, dec_ctx->saw_seqgoppic - 1, sub))
			{
				if (esstream->error)
					dec_ctx->no_bitstream_error = 0;
				return 0;
			}
			dec_ctx->saw_seqgoppic = 0;
			continue;
		}

		if (dec_ctx->in_pic_data) // See comment in read_pic_data()
		{
			if (!read_pic_data(esstream))
			{
				if (esstream->error)
					dec_ctx->no_bitstream_error = 0;
				return 0;
			}
			dec_ctx->saw_seqgoppic = 0;
			dec_ctx->in_pic_data = 0;
			continue;
		}

		// Nothing found - bitstream error
		if (startcode == 0xBA)
		{
			mprint("\nFound PACK header in ES data.  Probably wrong stream mode!\n");
		}
		else
		{
			mprint("\nUnexpected startcode: %02X\n", startcode);
		}
		dec_ctx->no_bitstream_error = 0;
		return 0;
	} while (1);

	return 1;
}

// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
static int read_seq_info(struct lib_cc_decode *ctx, struct bitstream *esstream)
{
	debug("Read Sequence Info\n");

	// We only get here after seeing that start code
	if (next_u32(esstream) != 0xB3010000) // LSB first (0x000001B3)
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In read_seq_info: next_u32(esstream) != 0xB3010000. Please file a bug report on GitHub.\n");

	// If we get here esstream points to the start of a sequence_header_code
	// should we run out of data in esstream this is where we want to restart
	// after getting more.
	unsigned char *video_seq_start = esstream->pos;

	sequence_header(ctx, esstream);
	sequence_ext(ctx, esstream);
	// FIXME: if sequence extension is missing this is not MPEG-2,
	// or broken.  Set bitstream error.
	//extension_and_user_data(esstream);

	if (esstream->error)
		return 0;

	if (esstream->bitsleft < 0)
	{
		init_bitstream(esstream, video_seq_start, esstream->end);
		return 0;
	}

	debug("Read Sequence Info - processed\n\n");

	return 1;
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
static int sequence_header(struct lib_cc_decode *ctx, struct bitstream *esstream)
{
	debug("Sequence header\n");

	if (esstream->error || esstream->bitsleft <= 0)
		return 0;

	// We only get here after seeing that start code
	if (read_u32(esstream) != 0xB3010000) // LSB first (0x000001B3)
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In sequence_header: read_u32(esstream) != 0xB3010000. Please file a bug report on GitHub.\n");

	unsigned hor_size = (unsigned)read_bits(esstream, 12);
	unsigned vert_size = (unsigned)read_bits(esstream, 12);
	unsigned aspect_ratio = (unsigned)read_bits(esstream, 4);
	unsigned frame_rate = (unsigned)read_bits(esstream, 4);

#if 0
	ctx->freport.width = hor_size;
	ctx->freport.height = vert_size;
	ctx->freport.aspect_ratio = aspect_ratio;
	ctx->freport.frame_rate = frame_rate;
#endif

	// Discard some information
	read_bits(esstream, 18 + 1 + 10 + 1);

	// load_intra_quantiser_matrix
	if (read_bits(esstream, 1))
		skip_bits(esstream, 8 * 64);
	// load_non_intra_quantiser_matrix
	if (read_bits(esstream, 1))
		skip_bits(esstream, 8 * 64);

	if (esstream->bitsleft < 0)
		return 0;

	// If we got the whole sequence, process
	if (hor_size != ctx->current_hor_size ||
	    vert_size != ctx->current_vert_size ||
	    aspect_ratio != ctx->current_aspect_ratio ||
	    frame_rate != ctx->current_frame_rate)
	{
		// If horizontal/vertical size, framerate and/or aspect
		// ratio are illegal, we discard the
		// whole sequence info.
		if (vert_size >= 288 && vert_size <= 1088 &&
		    hor_size >= 352 && hor_size <= 1920 &&
		    (hor_size * 100) / vert_size >= (352 * 100) / 576 && // The weird *100 is to avoid using floats
		    hor_size / vert_size <= 2 &&
		    frame_rate > 0 && frame_rate < 9 &&
		    aspect_ratio > 0 && aspect_ratio < 5)
		{
			mprint("\n\nNew video information found");
			mprint("\n");
			mprint("[%u * %u] [AR: %s] [FR: %s]",
			       hor_size, vert_size,
			       aspect_ratio_types[aspect_ratio],
			       framerates_types[frame_rate]);
			// No newline, force the output of progressive info in picture
			// info part.
			ctx->current_progressive_sequence = 2;

			ctx->current_hor_size = hor_size;
			ctx->current_vert_size = vert_size;
			ctx->current_aspect_ratio = aspect_ratio;
			ctx->current_frame_rate = frame_rate;
			current_fps = framerates_values[ctx->current_frame_rate];
			activity_video_info(hor_size, vert_size,
					    aspect_ratio_types[aspect_ratio],
					    framerates_types[frame_rate]);
		}
		else
		{
			debug("\nInvalid sequence header:\n");
			debug("V: %u H: %u FR: %u AS: %u\n",
			      vert_size, hor_size, frame_rate, aspect_ratio);
			esstream->error = 1;
			return 0;
		}
	}

	// Read complete
	return 1;
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
static int sequence_ext(struct lib_cc_decode *ctx, struct bitstream *esstream)
{
	debug("Sequence extension\n");

	if (esstream->error || esstream->bitsleft <= 0)
		return 0;

	// Syntax check
	if (next_start_code(esstream) != 0xB5)
	{
		debug("sequence_ext: syntax problem.\n");
		return 0;
	}

	skip_u32(esstream); // Advance

	// Read extension_start_code_identifier
	unsigned extension_id = (unsigned)read_bits(esstream, 4);
	if (extension_id != 0x1) // Sequence Extension ID
	{
		if (esstream->bitsleft >= 0) // When bits left, this is wrong
			esstream->error = 1;

		if (esstream->error)
			debug("sequence_ext: syntax problem.\n");
		return 0;
	}

	// Discard some information
	skip_bits(esstream, 8);
	unsigned progressive_sequence = (unsigned)read_bits(esstream, 1);

	if (progressive_sequence != ctx->current_progressive_sequence)
	{
		ctx->current_progressive_sequence = progressive_sequence;
		mprint(" [progressive: %s]\n\n",
		       (progressive_sequence ? "yes" : "no"));
	}

	skip_bits(esstream, 2 + 2 + 2 + 12 + 1 + 8 + 1 + 2 + 5);

	if (esstream->bitsleft < 0)
		return 0;

	// Read complete
	return 1;
}

// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
static int read_gop_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, struct cc_subtitle *sub)
{
	debug("Read GOP Info\n");

	// We only get here after seeing that start code
	if (next_u32(esstream) != 0xB8010000) // LSB first (0x000001B8)
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In read_gop_info: next_u32(esstream) != 0xB8010000. Please file a bug report on GitHub.\n");

	// If we get here esstream points to the start of a group_start_code
	// should we run out of data in esstream this is where we want to restart
	// after getting more.
	unsigned char *gop_info_start = esstream->pos;

	gop_header(enc_ctx, dec_ctx, esstream, sub);
	//extension_and_user_data(esstream);

	if (esstream->error)
		return 0;

	if (esstream->bitsleft < 0)
	{
		init_bitstream(esstream, gop_info_start, esstream->end);
		return 0;
	}

	debug("Read GOP Info - processed\n\n");

	return 1;
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
static int gop_header(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, struct cc_subtitle *sub)
{
	debug("GOP header\n");

	if (esstream->error || esstream->bitsleft <= 0)
		return 0;

	// We only get here after seeing that start code
	if (read_u32(esstream) != 0xB8010000) // LSB first (0x000001B8)
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In gop_header: read_u32(esstream) != 0xB8010000. Please file a bug report on GitHub.\n");

	unsigned drop_frame_flag = (unsigned)read_bits(esstream, 1);
	struct gop_time_code gtc;
	gtc.time_code_hours = (int)read_bits(esstream, 5);
	gtc.time_code_minutes = (int)read_bits(esstream, 6);
	skip_bits(esstream, 1); // Marker bit
	gtc.time_code_seconds = (int)read_bits(esstream, 6);
	gtc.time_code_pictures = (int)read_bits(esstream, 6);
	gtc.inited = 1;
	calculate_ms_gop_time(&gtc);

	if (esstream->bitsleft < 0)
		return 0;

	if (gop_accepted(&gtc))
	{
		// Do GOP padding during GOP header. The previous GOP and all
		// included captions are written. Use the current GOP time to
		// do the padding.

		// Flush buffered cc blocks before doing the housekeeping
		if (dec_ctx->has_ccdata_buffered)
		{
			process_hdcc(enc_ctx, dec_ctx, sub);
		}

		// Last GOPs pulldown frames
		if ((dec_ctx->current_pulldownfields > 0) != (dec_ctx->pulldownfields > 0))
		{
			dec_ctx->current_pulldownfields = dec_ctx->pulldownfields;
			debug("Pulldown: %s", (dec_ctx->pulldownfields ? "on" : "off"));
			if (dec_ctx->pulldownfields)
				debug(" - %u fields in last GOP", dec_ctx->pulldownfields);
			debug("\n");
		}
		dec_ctx->pulldownfields = 0;

		// Report synchronization jumps between GOPs. Warn if there
		// are 20% or more deviation.
		if ((ccx_options.debug_mask & CCX_DMT_TIME) && ((gtc.ms - gop_time.ms // more than 20% longer
								 > dec_ctx->frames_since_last_gop * 1000.0 / current_fps * 1.2) ||
								(gtc.ms - gop_time.ms // or 20% shorter
								 < dec_ctx->frames_since_last_gop * 1000.0 / current_fps * 0.8)) &&
		    first_gop_time.inited)
		{
			mprint("\rWarning: Jump in GOP timing.\n");
			mprint("  (old) %s",
			       print_mstime_static(gop_time.ms));
			mprint("  +  %s (%uF)",
			       print_mstime_static((LLONG)(dec_ctx->frames_since_last_gop * 1000.0 / current_fps)),
			       dec_ctx->frames_since_last_gop);
			mprint("  !=  (new) %s\n",
			       print_mstime_static(gtc.ms));
		}

		if (first_gop_time.inited == 0)
		{
			first_gop_time = gtc;

			// It needs to be "+1" because the frame count starts at 0 and we
			// need the length of all frames.
			if (total_frames_count == 0)
			{ // If this is the first frame there cannot be an offset
				dec_ctx->timing->fts_fc_offset = 0;
				// first_gop_time.ms stays unchanged
			}
			else
			{
				dec_ctx->timing->fts_fc_offset = (LLONG)((total_frames_count + 1) * 1000.0 / current_fps);
				// Compensate for those written before
				first_gop_time.ms -= dec_ctx->timing->fts_fc_offset;
			}

			dbg_print(CCX_DMT_TIME, "\nFirst GOP time: %02u:%02u:%02u:%03u %+lldms\n",
				  gtc.time_code_hours,
				  gtc.time_code_minutes, gtc.time_code_seconds,
				  (unsigned)(1000.0 * gtc.time_code_pictures / current_fps),
				  dec_ctx->timing->fts_fc_offset);
		}

		gop_time = gtc;

		dec_ctx->frames_since_last_gop = 0;
		// Indicate that we read a gop header (since last frame number 0)
		dec_ctx->saw_gop_header = 1;

		// If we use GOP timing, reconstruct the PTS from the GOP
		if (ccx_options.use_gop_as_pts == 1)
		{
			set_current_pts(dec_ctx->timing, gtc.ms * (MPEG_CLOCK_FREQ / 1000));
			dec_ctx->timing->current_tref = 0;
			frames_since_ref_time = 0;
			set_fts(dec_ctx->timing);
			fts_at_gop_start = get_fts_max(dec_ctx->timing);
		}
		else
		{
			// FIXME: Wrong when PTS are not increasing but are identical
			// throughout the GOP and then jump to the next time for the
			// next GOP.
			// This effect will also lead to captions being one GOP early
			// for DVD captions.
			fts_at_gop_start = get_fts_max(dec_ctx->timing) + (LLONG)(1000.0 / current_fps);
		}

		if (ccx_options.debug_mask & CCX_DMT_TIME)
		{
			dbg_print(CCX_DMT_TIME, "\nNew GOP:\n");
			dbg_print(CCX_DMT_TIME, "\nDrop frame flag: %u:\n", drop_frame_flag);
			print_debug_timing(dec_ctx->timing);
		}
	}

	return 1;
}

// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
static int read_pic_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, struct cc_subtitle *sub)
{
	debug("Read PIC Info\n");

	// We only get here after seeing that start code
	if (next_u32(esstream) != 0x00010000) // LSB first (0x00000100)
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In read_pic_info: next_u32(esstream) != 0x00010000. Please file a bug report on GitHub.\n");

	// If we get here esstream points to the start of a group_start_code
	// should we run out of data in esstream this is where we want to restart
	// after getting more.
	unsigned char *pic_info_start = esstream->pos;

	pic_header(dec_ctx, esstream);
	pic_coding_ext(dec_ctx, esstream);

	if (esstream->error)
		return 0;

	if (esstream->bitsleft < 0)
	{
		init_bitstream(esstream, pic_info_start, esstream->end);
		return 0;
	}

	// A new anchor frame - flush buffered caption data. Might be flushed
	// in GOP header already.
	if (dec_ctx->picture_coding_type == CCX_FRAME_TYPE_I_FRAME || dec_ctx->picture_coding_type == CCX_FRAME_TYPE_P_FRAME)
	{
		if (((dec_ctx->picture_structure != 0x1) && (dec_ctx->picture_structure != 0x2)) ||
		    (dec_ctx->temporal_reference != dec_ctx->timing->current_tref))
		{
			// NOTE: process_hdcc() needs to be called before set_fts() as it
			// uses fts_now to re-create the timeline !!!!!
			if (dec_ctx->has_ccdata_buffered)
			{
				process_hdcc(enc_ctx, dec_ctx, sub);
			}
			anchor_hdcc(dec_ctx, dec_ctx->temporal_reference);
		}
	}

	dec_ctx->timing->current_tref = dec_ctx->temporal_reference;
	dec_ctx->timing->current_picture_coding_type = dec_ctx->picture_coding_type;

	// We mostly use PTS, but when the GOP mode is enabled do not set
	// the FTS time here.
	if (ccx_options.use_gop_as_pts != 1)
	{
		set_fts(dec_ctx->timing); // Initialize fts
	}

	// Set min_pts/sync_pts according to the current time stamp.
	// Use fts_at_gop_start as reference when a GOP header was seen
	// since the last frame 0. If not this is most probably a
	// TS without GOP headers but with USER DATA after each picture
	// header. Use the current FTS values as reference.
	// Note: If a GOP header was present the reference time is from
	// the beginning of the GOP, otherwise it is now.
	if (dec_ctx->temporal_reference == 0)
	{
		dec_ctx->last_gop_length = dec_ctx->maxtref + 1;
		dec_ctx->maxtref = dec_ctx->temporal_reference;

		// frames_since_ref_time is used in set_fts()

		if (dec_ctx->saw_gop_header)
		{
			// This time (fts_at_gop_start) that was set in the
			// GOP header and it might be off by one GOP. See the comment there.
			frames_since_ref_time = dec_ctx->frames_since_last_gop; // Should this be 0?
		}
		else
		{
			// No GOP header, use the current values
			fts_at_gop_start = get_fts(dec_ctx->timing, dec_ctx->current_field);
			frames_since_ref_time = 0;
		}

		if (ccx_options.debug_mask & CCX_DMT_TIME)
		{
			dbg_print(CCX_DMT_TIME, "\nNew temporal reference:\n");
			print_debug_timing(dec_ctx->timing);
		}

		dec_ctx->saw_gop_header = 0; // Reset the value
	}

	if (!dec_ctx->saw_gop_header && dec_ctx->picture_coding_type == CCX_FRAME_TYPE_I_FRAME)
	{
		// A new GOP begins with an I-frame. Lets hope there are
		// never more than one per GOP
		dec_ctx->frames_since_last_gop = 0;
	}

	// Set maxtref
	if (dec_ctx->temporal_reference > dec_ctx->maxtref)
	{
		dec_ctx->maxtref = dec_ctx->temporal_reference;
		if (dec_ctx->maxtref + 1 > dec_ctx->max_gop_length)
			dec_ctx->max_gop_length = dec_ctx->maxtref + 1;
	}

	unsigned extraframe = 0;
	if (dec_ctx->repeat_first_field)
	{
		dec_ctx->pulldownfields++;
		dec_ctx->total_pulldownfields++;
		if (dec_ctx->current_progressive_sequence || !(dec_ctx->total_pulldownfields % 2))
			extraframe = 1;
		if (dec_ctx->current_progressive_sequence && dec_ctx->top_field_first)
			extraframe = 2;
		dbg_print(CCX_DMT_VIDES, "Pulldown: total pd fields: %d - %d extra frames\n",
			  dec_ctx->total_pulldownfields, extraframe);
	}

	dec_ctx->total_pulldownframes += extraframe;
	total_frames_count += 1 + extraframe;
	dec_ctx->frames_since_last_gop += 1 + extraframe;
	frames_since_ref_time += 1 + extraframe;

	debug("Read PIC Info - processed\n\n");

	return 1;
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
static int pic_header(struct lib_cc_decode *ctx, struct bitstream *esstream)
{
	debug("PIC header\n");

	if (esstream->error || esstream->bitsleft <= 0)
		return 0;

	// We only get here after seeing that start code
	if (read_u32(esstream) != 0x00010000) // LSB first (0x00000100)
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In pic_header: read_u32(esstream) != 0x00010000. Please file a bug report on GitHub.\n");

	ctx->temporal_reference = (int)read_bits(esstream, 10);
	ctx->picture_coding_type = (enum ccx_frame_type)read_bits(esstream, 3);

	if (ctx->picture_coding_type == 1) // Write I-Frame in ffprobe format for easy comparison
	{
		ctx->num_key_frames++;
		dbg_print(CCX_DMT_VIDES, "key_frame=1|pkt_pts=%lld|pict_type=I\n", ctx->timing->current_pts);
	}

	// Discard vbv_delay
	skip_bits(esstream, 16);

	// Discard some information
	if (ctx->picture_coding_type == 2 || ctx->picture_coding_type == 3)
		skip_bits(esstream, 4);
	if (ctx->picture_coding_type == 3)
		skip_bits(esstream, 4);

	// extra_information
	while (read_bits(esstream, 1) == 1)
	{
		skip_bits(esstream, 8);
	}

	if (esstream->bitsleft < 0)
		return 0;

	if (!(ctx->picture_coding_type == CCX_FRAME_TYPE_I_FRAME || ctx->picture_coding_type == CCX_FRAME_TYPE_P_FRAME || ctx->picture_coding_type == CCX_FRAME_TYPE_B_FRAME))
	{
		if (esstream->bitsleft >= 0) // When bits left, this is wrong
			esstream->error = 1;

		if (esstream->error)
			debug("pic_header: syntax problem.\n");
		return 0;
	}

	return 1;
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
static int pic_coding_ext(struct lib_cc_decode *ctx, struct bitstream *esstream)
{
	debug("Picture coding extension %lld\n", esstream->bitsleft);

	if (esstream->error || esstream->bitsleft <= 0)
		return 0;

	// Syntax check
	if (next_start_code(esstream) != 0xB5)
	{
		debug("pic_coding_ext: syntax problem.\n");
		return 0;
	}

	skip_u32(esstream); // Advance

	// Read extension_start_code_identifier
	unsigned extension_id = (unsigned int)read_bits(esstream, 4);
	if (extension_id != 0x8) // Picture Coding Extension ID
	{
		if (esstream->bitsleft >= 0) // When bits left, this is wrong
			esstream->error = 1;

		if (esstream->error)
			debug("pic_coding_ext: syntax problem.\n");
		return 0;
	}

	// Discard some information
	skip_bits(esstream, 4 * 4 + 2);
	ctx->picture_structure = (unsigned int)read_bits(esstream, 2);
	ctx->top_field_first = (unsigned int)read_bits(esstream, 1);
	skip_bits(esstream, 5 * 1);
	ctx->repeat_first_field = (unsigned int)read_bits(esstream, 1);
	skip_bits(esstream, 1); // chroma
	ctx->progressive_frame = (unsigned int)read_bits(esstream, 1);
	unsigned composite_display = (unsigned int)read_bits(esstream, 1);
	if (composite_display)
		skip_bits(esstream, 1 + 3 + 1 + 7 + 8);

	if (esstream->bitsleft < 0)
		return 0;

	debug("Picture coding extension - processed\n");

	// Read complete
	return 1;
}

// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
static int read_eau_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, int udtype, struct cc_subtitle *sub)
{
	debug("Read Extension and User Info\n");

	// We only get here after seeing that start code
	unsigned char *tst = next_bytes(esstream, 4);
	if (!tst || tst[0] != 0x00 || tst[1] != 0x00 || tst[2] != 0x01 || (tst[3] != 0xB2 && tst[3] != 0xB5)) // (0x000001 B2||B5)
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In read_eau_info: Impossible values for tst. Please file a bug report on GitHub.\n");

	// The following extension_and_user_data() function makes sure that
	// user data is not evaluated twice. Should the function run out of
	// data it will make sure that esstream points to where we want to
	// continue after getting more.
	if (!extension_and_user_data(enc_ctx, dec_ctx, esstream, udtype, sub))
	{
		if (esstream->error)
			debug("\nWarning: Retry while reading Extension and User Data!\n");
		else
			debug("\nBitstream problem while reading Extension and User Data!\n");

		return 0;
	}

	debug("Read Extension and User Info - processed\n\n");

	return 1;
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
static int extension_and_user_data(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, int udtype, struct cc_subtitle *sub)
{
	debug("Extension and user data(%d)\n", udtype);

	if (esstream->error || esstream->bitsleft <= 0)
		return 0;

	// Remember where to continue
	unsigned char *eau_start = esstream->pos;

	uint8_t startcode;

	do
	{
		startcode = next_start_code(esstream);

		if (startcode == 0xB2 || startcode == 0xB5)
		{
			skip_u32(esstream); // Advance bitstream
			unsigned char *dstart = esstream->pos;

			// Advance esstream to the next startcode.  Verify that
			// the whole extension was available and discard blocks
			// followed by PACK headers.  The latter usually indicates
			// a PS treated as an ES.
			uint8_t nextstartcode = search_start_code(esstream);
			if (nextstartcode == 0xBA)
			{
				mprint("\nFound PACK header in ES data. Probably wrong stream mode!\n");
				esstream->error = 1;
				return 0;
			}

			if (esstream->error)
			{
				debug("Extension and user data - syntax problem\n");
				return 0;
			}

			if (esstream->bitsleft < 0)
			{
				debug("Extension and user data - incomplete\n");
				// Restore to where we need to continue
				init_bitstream(esstream, eau_start, esstream->end);
				esstream->bitsleft = -1; // Redundant
				return 0;
			}

			if (startcode == 0xB2)
			{
				struct bitstream ustream;
				init_bitstream(&ustream, dstart, esstream->pos);
				user_data(enc_ctx, dec_ctx, &ustream, udtype, sub);
			}
			else
			{
				debug("Skip %d bytes extension data.\n",
				      esstream->pos - dstart);
			}
			// If we get here esstream points to the end of a block
			// of extension or user data.  Should we run out of data in
			// this loop this is where we want to restart after getting more.
			eau_start = esstream->pos;
		}
	} while (startcode == 0xB2 || startcode == 0xB5);

	if (esstream->error)
	{
		debug("Extension and user data - syntax problem\n");
		return 0;
	}
	if (esstream->bitsleft < 0)
	{
		debug("Extension and user data - incomplete\n");
		// Restore to where we need to continue
		init_bitstream(esstream, eau_start, esstream->end);
		esstream->bitsleft = -1; // Redundant
		return 0;
	}

	debug("Extension and user data - processed\n");

	// Read complete
	return 1;
}

// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
static int read_pic_data(struct bitstream *esstream)
{
	debug("Read PIC Data\n");

	uint8_t startcode = next_start_code(esstream);

	// Possibly the last call to this function ended with the last
	// bit of the slice? I.e. in_pic_data is still true, but we are
	// seeing the next start code.

	// We only get here after seeing that start code
	if (startcode < 0x01 || startcode > 0xAF)
	{
		debug("Read Pic Data - processed0\n");

		return 1;
	}

	// If we get here esstream points to the start of a slice_start_code
	// should we run out of data in esstream this is where we want to restart
	// after getting more.
	unsigned char *slice_start = esstream->pos;

	do
	{
		startcode = next_start_code(esstream);
		// Syntax check
		if (startcode == 0xB4)
		{
			if (esstream->bitsleft < 0)
				init_bitstream(esstream, slice_start, esstream->end);

			if (esstream->error)
				debug("read_pic_data: syntax problem.\n");
			else
				debug("read_pic_data: reached end of bitstream.\n");

			return 0;
		}

		slice_start = esstream->pos; // No need to come back

		if (startcode >= 0x01 && startcode <= 0xAF)
		{
			skip_u32(esstream);	     // Advance bitstream
			search_start_code(esstream); // Skip this slice
		}
	} while (startcode >= 0x01 && startcode <= 0xAF);

	if (esstream->bitsleft < 0)
	{
		init_bitstream(esstream, slice_start, esstream->end);
		return 0;
	}

	debug("Read Pic Data - processed\n");

	return 1;
}
