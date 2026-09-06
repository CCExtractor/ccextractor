#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "activity.h"

// Functions to parse a mpeg-2 data stream, see ISO/IEC 13818-2 6.2
static uint8_t search_start_code(struct bitstream *esstream);
static uint8_t next_start_code(struct bitstream *esstream);
static int es_video_sequence(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *esstream, struct cc_subtitle *sub);
static int read_seq_info(struct lib_cc_decode *ctx, struct bitstream *esstream);
static int read_gop_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *esstream, struct cc_subtitle *sub);
static int read_pic_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *esstream, struct cc_subtitle *sub);
static int read_eau_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *esstream, int udtype, struct cc_subtitle *sub);
static int read_pic_data(struct bitstream *esstream);

#define debug(...) ccx_common_logging.debug_ftn(CCX_DMT_VERBOSE, __VA_ARGS__)

extern uint8_t ccxr_search_start_code(struct bitstream *esstream);
extern uint8_t ccxr_next_start_code(struct bitstream *esstream);
extern int ccxr_read_seq_info(struct lib_cc_decode *ctx, struct bitstream *esstream);
extern int ccxr_read_gop_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, struct cc_subtitle *sub);
extern int ccxr_read_pic_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, struct cc_subtitle *sub);
extern int ccxr_read_eau_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, int udtype, struct cc_subtitle *sub);
extern int ccxr_read_pic_data(struct bitstream *esstream);

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
	return ccxr_search_start_code(esstream);
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
	return ccxr_next_start_code(esstream);
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
	return ccxr_read_seq_info(ctx, esstream);
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
static int read_gop_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, struct cc_subtitle *sub)
{
	return ccxr_read_gop_info(enc_ctx, dec_ctx, esstream, sub);
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
static int read_pic_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, struct cc_subtitle *sub)
{
	return ccxr_read_pic_info(enc_ctx, dec_ctx, esstream, sub);
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
static int read_eau_info(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, struct bitstream *esstream, int udtype, struct cc_subtitle *sub)
{
	return ccxr_read_eau_info(enc_ctx, dec_ctx, esstream, udtype, sub);
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
static int read_pic_data(struct bitstream *esstream)
{
	return ccxr_read_pic_data(esstream);
}
