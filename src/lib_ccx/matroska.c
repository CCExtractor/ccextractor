#include "lib_ccx.h"
#include "utility.h"
#include "matroska.h"
#include "ccx_encoders_helpers.h"
#include "ccx_common_timing.h"
#include <limits.h>
#include <assert.h>
#include "dvb_subtitle_decoder.h"
#include "vobsub_decoder.h"

void skip_bytes(FILE *file, ULLONG n)
{
	FSEEK(file, n, SEEK_CUR);
}

void set_bytes(FILE *file, ULLONG n)
{
	FSEEK(file, n, SEEK_SET);
}

ULLONG get_current_byte(FILE *file)
{
	return (ULLONG)FTELL(file);
}

UBYTE *read_byte_block(FILE *file, ULLONG n)
{
	UBYTE *buffer = malloc((size_t)(sizeof(UBYTE) * n));
	if (buffer == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In read_byte_block: Out of memory.");
	if (fread(buffer, 1, (size_t)n, file) != n)
		fatal(1, "reading from file");
	return buffer;
}

char *read_bytes_signed(FILE *file, ULLONG n)
{
	char *buffer = malloc((size_t)(sizeof(UBYTE) * (n + 1)));
	if (buffer == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In read_bytes_signed: Out of memory.");
	if (fread(buffer, 1, (size_t)n, file) != n)
		fatal(1, "reading from file");
	buffer[n] = 0;
	return buffer;
}

UBYTE mkv_read_byte(FILE *file)
{
	return (UBYTE)fgetc(file);
}

ULLONG read_vint_length(FILE *file)
{
	UBYTE ch = mkv_read_byte(file);
	int cnt = 0;
	for (int i = 7; i >= 0; i--)
	{
		if ((ch & (1 << i)) != 0)
		{
			cnt = 8 - i;
			break;
		}
	}
	ch ^= (1 << (8 - cnt));
	ULLONG ret = ch;
	for (int i = 1; i < cnt; i++)
	{
		ret <<= 8;
		ret += mkv_read_byte(file);
	}
	return ret;
}

UBYTE *read_vint_block(FILE *file)
{
	ULLONG len = read_vint_length(file);
	return read_byte_block(file, len);
}

char *read_vint_block_signed(FILE *file)
{
	ULLONG len = read_vint_length(file);
	return read_bytes_signed(file, len);
}

ULLONG read_vint_block_int(FILE *file)
{
	ULLONG len = read_vint_length(file);
	UBYTE *s = read_byte_block(file, len);

	ULLONG res = 0;
	for (int i = 0; i < len; i++)
	{
		res <<= 8;
		res += s[i];
	}

	free(s);
	return res;
}

char *read_vint_block_string(FILE *file)
{
	return read_vint_block_signed(file);
}

void read_vint_block_skip(FILE *file)
{
	ULLONG len = read_vint_length(file);
	skip_bytes(file, len);
}

void parse_ebml(FILE *file)
{
	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);

	printf("\n");

	int code = 0, code_len = 0;
	while (pos + len > get_current_byte(file))
	{
		code <<= 8;
		code += mkv_read_byte(file);
		if (feof(file))
			break;
		code_len++;

		switch (code)
		{
			/* EBML ids */
			case MATROSKA_EBML_VERSION:
				read_vint_block_int(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_EBML_READ_VERSION:
				read_vint_block_int(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_EBML_MAX_ID_LENGTH:
				read_vint_block_int(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_EBML_MAX_SIZE_LENGTH:
				read_vint_block_int(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_EBML_DOC_TYPE:
			{
				char *doc_type = read_vint_block_string(file);
				mprint("Document type: %s\n", doc_type);
				free(doc_type);
				MATROSKA_SWITCH_BREAK(code, code_len);
			}
			case MATROSKA_EBML_DOC_TYPE_VERSION:
				read_vint_block_int(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_EBML_DOC_TYPE_READ_VERSION:
				read_vint_block_int(file);
				MATROSKA_SWITCH_BREAK(code, code_len);

				/* Misc ids */
			case MATROSKA_VOID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_CRC32:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			default:
				if (code_len == MATROSKA_MAX_ID_LENGTH)
				{
					mprint(MATROSKA_WARNING "Unknown element 0x%x at position " LLD ", skipping this element\n", code,
					       get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
					// Skip just the unknown element, not the entire block
					read_vint_block_skip(file);
					// Reset code and code_len to start fresh with next element
					code = 0;
					code_len = 0;
				}
				break;
		}
	}
}

void parse_segment_info(FILE *file)
{
	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);

	int code = 0, code_len = 0;
	while (pos + len > get_current_byte(file))
	{
		code <<= 8;
		code += mkv_read_byte(file);
		if (feof(file))
			break;
		code_len++;

		switch (code)
		{
			/* Segment info ids */
			case MATROSKA_SEGMENT_INFO_SEGMENT_UID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO_SEGMENT_FILENAME:
			{
				char *filename = read_vint_block_string(file);
				mprint("Filename: %s\n", filename);
				free(filename);
				MATROSKA_SWITCH_BREAK(code, code_len);
			}
			case MATROSKA_SEGMENT_INFO_PREV_UID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO_PREV_FILENAME:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO_NEXT_UID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO_NEXT_FILENAME:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO_SEGMENT_FAMILY:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO_CHAPTER_TRANSLATE:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO_TIMECODE_SCALE:
				mprint("Timecode scale: " LLD "\n", read_vint_block_int(file));
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO_DURATION:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO_DATE_UTC:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO_TITLE:
			{
				char *title = read_vint_block_string(file);
				mprint("Title: %s\n", title);
				free(title);
				MATROSKA_SWITCH_BREAK(code, code_len);
			}
			case MATROSKA_SEGMENT_MUXING_APP:
			{
				char *muxing_app = read_vint_block_string(file);
				mprint("Muxing app: %s\n", muxing_app);
				free(muxing_app);
				MATROSKA_SWITCH_BREAK(code, code_len);
			}
			case MATROSKA_SEGMENT_WRITING_APP:
			{
				char *writing_app = read_vint_block_string(file);
				mprint("Writing app: %s\n", writing_app);
				free(writing_app);
				MATROSKA_SWITCH_BREAK(code, code_len);
			}

				/* Misc ids */
			case MATROSKA_VOID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_CRC32:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			default:
				if (code_len == MATROSKA_MAX_ID_LENGTH)
				{
					mprint(MATROSKA_WARNING "Unknown element 0x%x at position " LLD ", skipping this element\n", code,
					       get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
					// Skip just the unknown element, not the entire block
					read_vint_block_skip(file);
					// Reset code and code_len to start fresh with next element
					code = 0;
					code_len = 0;
				}
				break;
		}
	}
}

char *generate_timestamp_ass_ssa(ULLONG milliseconds)
{
	ULLONG millis = (milliseconds % 1000) / 10;
	milliseconds /= 1000;
	ULLONG seconds = milliseconds % 60;
	milliseconds /= 60;
	ULLONG minutes = milliseconds % 60;
	milliseconds /= 60;
	ULLONG hours = milliseconds;

	char *buf = malloc(sizeof(char) * 32);
	if (buf == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In generate_timestamp_ass_ssa: Out of memory.");
	snprintf(buf, 32, LLD ":%02" LLD_M ":%02" LLD_M ".%02" LLD_M, hours, minutes, seconds, millis);
	return buf;
}

int find_sub_track_index(struct matroska_ctx *mkv_ctx, ULLONG track_number)
{
	for (int i = mkv_ctx->sub_tracks_count - 1; i >= 0; i--)
		if (mkv_ctx->sub_tracks[i]->track_number == track_number)
			return i;
	return -1;
}

struct matroska_sub_sentence *parse_segment_cluster_block_group_block(struct matroska_ctx *mkv_ctx, ULLONG cluster_timecode)
{
	FILE *file = mkv_ctx->file;
	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);
	ULLONG track_number = read_vint_length(file); // track number is length, not int

	int sub_track_index = find_sub_track_index(mkv_ctx, track_number);
	if (sub_track_index == -1)
	{
		set_bytes(file, pos + len);
		return NULL;
	}
	mkv_ctx->block_index = sub_track_index;

	ULLONG timecode = mkv_read_byte(file);
	timecode <<= 8;
	timecode += mkv_read_byte(file);

	mkv_read_byte(file); // skip one byte

	ULLONG size = pos + len - get_current_byte(file);
	char *message = read_bytes_signed(file, size);
	struct matroska_sub_track *track = mkv_ctx->sub_tracks[sub_track_index];

	struct matroska_sub_sentence *sentence = malloc(sizeof(struct matroska_sub_sentence));
	if (sentence == NULL)
	{
		free(message);
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In parse_segment_cluster_block_group_block: Out of memory.");
	}
	ULLONG timestamp = timecode + cluster_timecode;
	sentence->blockaddition = NULL;
	sentence->time_end = 0; // Initialize time_end so that it is updated if it was not set

	if (strcmp(track->codec_id_string, dvb_codec_id) == 0)
	{
		struct encoder_ctx *enc_ctx = update_encoder_list(mkv_ctx->ctx);
		struct lib_cc_decode *dec_ctx = update_decoder_list(mkv_ctx->ctx);

		set_current_pts(dec_ctx->timing, timestamp * (MPEG_CLOCK_FREQ / 1000));

		int ret = dvbsub_decode(enc_ctx, dec_ctx, message, size, &mkv_ctx->dec_sub);
		// We use string produced by enc_ctx as a message
		free(message);

		/* Bear in mind that in DVB we are handling the text of the previous block.
		There can be 2 types of DVB in .mkv. One is when each display block is followed by empty block in order to
		allow gaps in time between display blocks. Another one is when display block is followed by another display block.
		This code handles both cases but we don't save and use empty blocks as sentences, only time_starts of them. */
		char *dvb_message = enc_ctx->last_string;
		if (ret < 0 || dvb_message == NULL)
		{
			// No text - no sentence is returned. Free the memory
			free(sentence);
			if (ret < 0)
				mprint("Return from dvbsub_decode: %d\n", ret);
			else
				track->last_timestamp = timestamp; // We save timestamp because we need to use it for the next sub as a timestart
			return NULL;
		}
		sentence->text = dvb_message;
		sentence->text_size = strlen(dvb_message);

		/* Update time.
		Time start - timestamp of the previous block
		Time end - timestamp of the current block */
		sentence->time_start = track->last_timestamp;
		sentence->time_end = timestamp;
		track->last_timestamp = timestamp;
	}
	else
	{
		sentence->time_start = timestamp;
		sentence->text = message;
		sentence->text_size = size;
	}

	if (track->sentence_count == 0)
	{
		track->sentences = malloc(sizeof(struct matroska_sub_sentence *));
		if (track->sentences == NULL)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In parse_segment_cluster_block_group_block: Out of memory allocating sentences.");
	}
	else
	{
		void *tmp = realloc(track->sentences, (track->sentence_count + 1) * sizeof(struct matroska_sub_sentence *));
		if (tmp == NULL)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In parse_segment_cluster_block_group_block: Out of memory reallocating sentences.");
		track->sentences = tmp;
	}
	track->sentences[track->sentence_count] = sentence;
	track->sentence_count++;

	mkv_ctx->current_second = max(mkv_ctx->current_second, sentence->time_start / 1000);
	activity_progress((int)(get_current_byte(file) * 100 / mkv_ctx->ctx->inputsize),
			  (int)(mkv_ctx->current_second / 60),
			  (int)(mkv_ctx->current_second % 60));

	return sentence;
}

struct matroska_sub_sentence *parse_segment_cluster_block_group_block_additions(struct matroska_ctx *mkv_ctx, ULLONG cluster_timecode)
{
	FILE *file = mkv_ctx->file;
	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);

	// gets WebVTT track
	int sub_track_index = mkv_ctx->block_index;
	if (sub_track_index == 0)
	{
		set_bytes(file, pos + len);
		return NULL;
	}
	struct matroska_sub_track *track = mkv_ctx->sub_tracks[sub_track_index];

	for (int i = 0; i < 4; i++) // skip four bytes
		mkv_read_byte(file);

	ULLONG size = pos + len - get_current_byte(file);
	char *message = read_bytes_signed(file, size);

	// parses message into block addition
	struct block_addition *newBA = calloc(1, sizeof(struct block_addition));
	if (newBA == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In parse_segment_cluster_block_group_block_additions: Out of memory.");
	char *current = message;
	int lastIndex = 0;
	int item = 0;
	int item_size;
	for (int i = 0; i < size && item < 2; i++)
	{
		if (*current == '\n')
		{
			*current = '\0';
			item_size = i - lastIndex;
			if (item_size != 0)
			{
				if (item == 0)
				{
					newBA->cue_settings_list = message + lastIndex;
					newBA->cue_settings_list_size = item_size;
				}
				else if (item == 1)
				{
					newBA->cue_identifier = message + lastIndex;
					newBA->cue_identifier_size = item_size;
				}
			}
			item++;
			lastIndex = i + 1;
		}
		current++;
	}
	if (lastIndex < size)
	{
		item_size = size - lastIndex;
		newBA->comment = message + lastIndex;
		newBA->comment_size = item_size;
	}

	// attaching BlockAddition to the cue
	struct matroska_sub_sentence *sentence = track->sentences[track->sentence_count - 1];
	sentence->blockaddition = newBA;

	// returns the sentence (cue) that this BlockAddition is attached to
	return sentence;
}

void parse_segment_cluster_block_group(struct matroska_ctx *mkv_ctx, ULLONG cluster_timecode)
{
	FILE *file = mkv_ctx->file;
	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);

	ULLONG block_duration = ULONG_MAX;
	struct matroska_sub_sentence *new_sentence;
	struct matroska_sub_sentence **sentence_list = NULL;
	int sentence_count = 0;

	int code = 0, code_len = 0;
	while (pos + len > get_current_byte(file))
	{
		code <<= 8;
		code += mkv_read_byte(file);
		if (feof(file))
			break;
		code_len++;

		switch (code)
		{
			/* Segment cluster block group ids */
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK:
				new_sentence = parse_segment_cluster_block_group_block(mkv_ctx, cluster_timecode);
				if (new_sentence != NULL)
				{
					void *tmp = realloc(sentence_list, sizeof(struct matroska_sub_track *) * (sentence_count + 1));
					if (tmp == NULL)
						fatal(EXIT_NOT_ENOUGH_MEMORY, "In parse_segment_cluster_block_group: Out of memory.");
					sentence_list = tmp;
					sentence_list[sentence_count] = new_sentence;
					sentence_count++;
				}
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_VIRTUAL:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_ADDITIONS:
				parse_segment_cluster_block_group_block_additions(mkv_ctx, cluster_timecode);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_DURATION:
				block_duration = read_vint_block_int(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_PRIORITY:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_BLOCK:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_CODEC_STATE:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_DISCARD_PADDING:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_SLICES:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_FRAME:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);

				/* Misc ids */
			case MATROSKA_VOID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_CRC32:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			default:
				if (code_len == MATROSKA_MAX_ID_LENGTH)
				{
					mprint(MATROSKA_WARNING "Unknown element 0x%x at position " LLD ", skipping this element\n", code,
					       get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
					// Skip just the unknown element, not the entire block
					read_vint_block_skip(file);
					// Reset code and code_len to start fresh with next element
					code = 0;
					code_len = 0;
				}
				break;
		}
	}

	for (int i = 0; i < sentence_count; i++)
	{
		// When BlockDuration is not written, the value is assumed to be the difference
		// between the timestamp of this Block and the timestamp of the next Block in "display" order
		if (block_duration == ULONG_MAX)
			sentence_list[i]->time_end = ULONG_MAX;
		else if (sentence_list[i]->time_end == 0)
			// If no time_end is set, set it according to block_duration.
			// We need this check for correct DVB timecodes
			sentence_list[i]->time_end = sentence_list[i]->time_start + block_duration;

		if (ccx_options.gui_mode_reports)
		{
			unsigned h1, m1, s1, ms1;
			millis_to_time(sentence_list[i]->time_start, &h1, &m1, &s1, &ms1);
			unsigned h2, m2, s2, ms2;
			millis_to_time(sentence_list[i]->time_end, &h2, &m2, &s2, &ms2);

			char *text = sentence_list[i]->text;
			int length = strlen(text);
			int pos = 0;
			while (true)
			{
				if (pos == 0)
				{
					fprintf(stderr, "###TIME###%02u:%02u-%02u:%02u\n###SUBTITLE###",
						h1 * 60 + m1, s1, h2 * 60 + m2, s2);
				}
				else
				{
					fprintf(stderr, "###SUBTITLE###");
				}
				int end_pos = pos;
				while (end_pos < length && text[end_pos] != '\n')
					end_pos++;
				fwrite(text + pos, 1, end_pos - pos, stderr);
				fwrite("\n", 1, 1, stderr);
				fflush(stderr);
				if (end_pos == length)
					break;
				pos = end_pos + 1;
			}
		}
	}

	free(sentence_list);
}

void parse_segment_cluster(struct matroska_ctx *mkv_ctx)
{
	FILE *file = mkv_ctx->file;
	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);

	ULLONG timecode = 0;

	int code = 0, code_len = 0;
	while (pos + len > get_current_byte(file))
	{
		code <<= 8;
		code += mkv_read_byte(file);
		if (feof(file))
			break;
		code_len++;

		switch (code)
		{
			/* Segment cluster ids */
			case MATROSKA_SEGMENT_CLUSTER_TIMECODE:
				timecode = read_vint_block_int(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_SILENT_TRACKS:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_POSITION:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_PREV_SIZE:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_SIMPLE_BLOCK:
				// Same as Block inside the Block Group
				parse_simple_block(mkv_ctx, timecode);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP:
				parse_segment_cluster_block_group(mkv_ctx, timecode);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER_ENCRYPTED_BLOCK:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);

				/* Misc ids */
			case MATROSKA_VOID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_CRC32:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			default:
				if (code_len == MATROSKA_MAX_ID_LENGTH)
				{
					mprint(MATROSKA_WARNING "Unknown element 0x%x at position " LLD ", skipping this element\n", code,
					       get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
					// Skip just the unknown element, not the entire block
					read_vint_block_skip(file);
					// Reset code and code_len to start fresh with next element
					code = 0;
					code_len = 0;
				}
				break;
		}
	}

	// We already update activity progress in subtitle block, but we also need to show percents
	// in samples without captions
	activity_progress((int)(get_current_byte(file) * 100 / mkv_ctx->ctx->inputsize),
			  (int)(mkv_ctx->current_second / 60),
			  (int)(mkv_ctx->current_second % 60));
}

void parse_simple_block(struct matroska_ctx *mkv_ctx, ULLONG frame_timestamp)
{
	FILE *file = mkv_ctx->file;

	struct matroska_avc_frame frame;
	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);

	ULLONG track = read_vint_length(file);

	// Check if this is an AVC or HEVC track
	int is_avc = (track == mkv_ctx->avc_track_number);
	int is_hevc = (track == mkv_ctx->hevc_track_number);

	if (!is_avc && !is_hevc)
	{
		// Skip everything except AVC/HEVC tracks
		skip_bytes(file, len - 1); // 1 byte for track
		return;
	}

	ULLONG timecode = mkv_read_byte(file);
	timecode <<= 8;
	timecode += mkv_read_byte(file);
	mkv_read_byte(file); // skip flags byte

	// Construct the frame
	frame.len = pos + len - get_current_byte(file);
	frame.data = read_byte_block(file, frame.len);
	frame.FTS = frame_timestamp + timecode;

	if (is_hevc)
		process_hevc_frame_mkv(mkv_ctx, frame);
	else
		process_avc_frame_mkv(mkv_ctx, frame);

	free(frame.data);
}

static long bswap32(long v)
{
	// For 0x12345678 returns 78563412
	long swapped = ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24);
	return swapped;
}

int process_avc_frame_mkv(struct matroska_ctx *mkv_ctx, struct matroska_avc_frame frame)
{
	int status = 0;
	uint32_t i;
	struct lib_cc_decode *dec_ctx = update_decoder_list(mkv_ctx->ctx);
	struct encoder_ctx *enc_ctx = update_encoder_list(mkv_ctx->ctx);

	// Delete
	// Inspired by set_fts(struct ccx_common_timing_ctx *ctx)
	set_current_pts(dec_ctx->timing, frame.FTS * (MPEG_CLOCK_FREQ / 1000));
	set_fts(dec_ctx->timing);

	// NAL unit length is assumed to be 4
	uint8_t nal_unit_size = 4;

	for (i = 0; i < frame.len;)
	{
		uint32_t nal_length;

		if (i + nal_unit_size > frame.len)
			break;

		nal_length =
		    ((uint32_t)frame.data[i] << 24) |
		    ((uint32_t)frame.data[i + 1] << 16) |
		    ((uint32_t)frame.data[i + 2] << 8) |
		    (uint32_t)frame.data[i + 3];

		i += nal_unit_size;

		if (nal_length > frame.len - i)
			break;

		if (nal_length > 0)
			do_NAL(enc_ctx, dec_ctx, (unsigned char *)&frame.data[i], nal_length, &mkv_ctx->dec_sub);
		i += nal_length;
	} // outer for

	mkv_ctx->current_second = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);

	return status;
}

int process_hevc_frame_mkv(struct matroska_ctx *mkv_ctx, struct matroska_avc_frame frame)
{
	int status = 0;
	uint32_t i;
	struct lib_cc_decode *dec_ctx = update_decoder_list(mkv_ctx->ctx);
	struct encoder_ctx *enc_ctx = update_encoder_list(mkv_ctx->ctx);

	// Set timing
	set_current_pts(dec_ctx->timing, frame.FTS * (MPEG_CLOCK_FREQ / 1000));
	set_fts(dec_ctx->timing);

	// Set HEVC mode for NAL parsing
	dec_ctx->avc_ctx->is_hevc = 1;

	// NAL unit length is assumed to be 4 (same as AVC in Matroska)
	uint8_t nal_unit_size = 4;

	for (i = 0; i < frame.len;)
	{
		uint32_t nal_length;

		if (i + nal_unit_size > frame.len)
			break;

		nal_length =
		    ((uint32_t)frame.data[i] << 24) |
		    ((uint32_t)frame.data[i + 1] << 16) |
		    ((uint32_t)frame.data[i + 2] << 8) |
		    (uint32_t)frame.data[i + 3];

		i += nal_unit_size;

		if (nal_length > frame.len - i)
			break;

		if (nal_length > 0)
			do_NAL(enc_ctx, dec_ctx, (unsigned char *)&frame.data[i], nal_length, &mkv_ctx->dec_sub);
		i += nal_length;
	}

	// Flush any accumulated CC data after processing this frame
	// This is critical for HEVC because store_hdcc() is normally called from
	// slice_header() which is AVC-only
	if (dec_ctx->avc_ctx->cc_count > 0)
	{
		store_hdcc(enc_ctx, dec_ctx, dec_ctx->avc_ctx->cc_data, dec_ctx->avc_ctx->cc_count,
			   dec_ctx->timing->current_tref, dec_ctx->timing->fts_now, &mkv_ctx->dec_sub);
		dec_ctx->avc_ctx->cc_buffer_saved = CCX_TRUE;
		dec_ctx->avc_ctx->cc_count = 0;
	}

	mkv_ctx->current_second = (int)(get_fts(dec_ctx->timing, dec_ctx->current_field) / 1000);

	return status;
}

char *get_track_entry_type_description(enum matroska_track_entry_type type)
{
	switch (type)
	{
		case MATROSKA_TRACK_TYPE_VIDEO:
			return "video";
		case MATROSKA_TRACK_TYPE_AUDIO:
			return "audio";
		case MATROSKA_TRACK_TYPE_COMPLEX:
			return "complex";
		case MATROSKA_TRACK_TYPE_LOGO:
			return "logo";
		case MATROSKA_TRACK_TYPE_SUBTITLE:
			return "subtitle";
		case MATROSKA_TRACK_TYPE_BUTTONS:
			return "buttons";
		case MATROSKA_TRACK_TYPE_CONTROL:
			return "control";
		default:
			return NULL;
	}
}

enum matroska_track_subtitle_codec_id get_track_subtitle_codec_id(char *codec_id)
{
	for (int i = MATROSKA_TRACK_SUBTITLE_CODEC_ID_UTF8; i <= MATROSKA_TRACK_SUBTITLE_CODEC_ID_KATE; i++)
		if (strcmp(codec_id, matroska_track_text_subtitle_id_strings[i]) == 0)
			return (enum matroska_track_subtitle_codec_id)i;
	return (enum matroska_track_subtitle_codec_id)0;
}

void parse_segment_track_entry(struct matroska_ctx *mkv_ctx)
{
	FILE *file = mkv_ctx->file;
	mprint("\nTrack entry:\n");

	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);

	ULLONG track_number = 0;
	enum matroska_track_entry_type track_type = MATROSKA_TRACK_TYPE_VIDEO;
	char *lang = strdup("eng");
	char *header = NULL;
	char *lang_ietf = NULL;
	char *codec_id_string = NULL;
	enum matroska_track_subtitle_codec_id codec_id = MATROSKA_TRACK_SUBTITLE_CODEC_ID_UTF8;

	int code = 0, code_len = 0;
	while (pos + len > get_current_byte(file))
	{
		code <<= 8;
		code += mkv_read_byte(file);
		if (feof(file))
			break;
		code_len++;

		switch (code)
		{
			/* Track entry ids*/
			case MATROSKA_SEGMENT_TRACK_TRACK_NUMBER:
				track_number = read_vint_block_int(file);
				mprint("    Track number: " LLD "\n", track_number);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_TRACK_UID:
				mprint("    UID: " LLU "\n", read_vint_block_int(file));
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_TRACK_TYPE:
				track_type = (enum matroska_track_entry_type)read_vint_block_int(file);
				mprint("    Type: %s\n", get_track_entry_type_description(track_type));
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_FLAG_ENABLED:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_FLAG_DEFAULT:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_FLAG_FORCED:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_FLAG_LACING:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_MIN_CACHE:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_MAX_CACHE:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_DEFAULT_DURATION:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_DEFAULT_DECODED_FIELD_DURATION:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_MAX_BLOCK_ADDITION_ID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_NAME:
			{
				char *name = read_vint_block_string(file);
				mprint("    Name: %s\n", name);
				free(name);
				MATROSKA_SWITCH_BREAK(code, code_len);
			}
			case MATROSKA_SEGMENT_TRACK_LANGUAGE:
				free(lang); // Free previous value (strdup or previous read)
				lang = read_vint_block_string(file);
				mprint("    Language: %s\n", lang);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_CODEC_ID:
				codec_id_string = read_vint_block_string(file);
				codec_id = get_track_subtitle_codec_id(codec_id_string);
				mprint("    Codec ID: %s\n", codec_id_string);
				// Detect AVC and HEVC tracks for EIA-608/708 caption extraction
				if (strcmp((const char *)codec_id_string, (const char *)avc_codec_id) == 0)
					mkv_ctx->avc_track_number = track_number;
				else if (strcmp((const char *)codec_id_string, (const char *)hevc_codec_id) == 0)
					mkv_ctx->hevc_track_number = track_number;
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_CODEC_PRIVATE:
				// We handle DVB's private data differently
				if (track_type == MATROSKA_TRACK_TYPE_SUBTITLE && strcmp((const char *)codec_id_string, (const char *)dvb_codec_id) != 0)
					header = read_vint_block_string(file);
				else
					parse_private_codec_data(mkv_ctx, codec_id_string, track_number, lang);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_CODEC_NAME:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_CODEC_ATTACHMENT_LINK:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_CODEC_DECODE_ALL:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_TRACK_OVERLAY:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_CODEC_DELAY:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_SEEK_PRE_ROLL:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_TRACK_TRANSLATE:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_VIDEO:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_AUDIO:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_TRACK_OPERATION:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_CONTENT_ENCODINGS:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);

				/* Deprecated IDs */
			case MATROSKA_SEGMENT_TRACK_TRACK_TIMECODE_SCALE:
				mprint(MATROSKA_WARNING "Deprecated element 0x%x at position " LLD "\n", code,
				       get_current_byte(file) - 3); // minus length of the ID
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_TRACK_OFFSET:
				mprint(MATROSKA_WARNING "Deprecated element 0x%x at position " LLD "\n", code,
				       get_current_byte(file) - 2); // minus length of the ID
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);

				/* DivX trick track extensions */
			case MATROSKA_SEGMENT_TRACK_TRICK_TRACK_UID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_TRICK_TRACK_SEGMENT_UID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_TRICK_TRACK_FLAG:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_TRICK_MASTER_TRACK_UID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_TRICK_MASTER_TRACK_SEGMENT_UID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACK_LANGUAGE_IETF:
				lang_ietf = read_vint_block_string(file);
				mprint("    Language IETF: %s\n", lang_ietf);
				// We'll store this for later use rather than freeing it immediately
				if (track_type == MATROSKA_TRACK_TYPE_SUBTITLE)
				{
					// Don't free lang_ietf here, store in track
					if (lang != NULL)
					{
						// If we previously allocated lang, free it as we'll prefer IETF
						free(lang);
						lang = NULL;
					}
					// Default to "eng" if we somehow don't have a language yet
					if (lang == NULL)
					{
						lang = strdup("eng");
					}
				}
				else
				{
					free(lang_ietf); // Free if not a subtitle track
					lang_ietf = NULL;
				}
				MATROSKA_SWITCH_BREAK(code, code_len);

				/* Misc ids */
			case MATROSKA_VOID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_CRC32:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			default:
				if (code_len == MATROSKA_MAX_ID_LENGTH)
				{
					mprint(MATROSKA_WARNING "Unknown element 0x%x at position " LLD ", skipping this element\n", code,
					       get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
					// Skip just the unknown element, not the entire block
					read_vint_block_skip(file);
					// Reset code and code_len to start fresh with next element
					code = 0;
					code_len = 0;
				}
				break;
		}
	}

	if (track_type == MATROSKA_TRACK_TYPE_SUBTITLE)
	{
		struct matroska_sub_track *sub_track = malloc(sizeof(struct matroska_sub_track));
		if (sub_track == NULL)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In parse_segment_track_entry: Out of memory allocating sub_track.");
		sub_track->header = header;
		sub_track->lang = lang;
		sub_track->lang_ietf = lang_ietf;
		sub_track->track_number = track_number;
		sub_track->lang_index = 0;
		sub_track->codec_id = codec_id;
		sub_track->codec_id_string = codec_id_string;
		sub_track->sentence_count = 0;
		sub_track->last_timestamp = 0;
		sub_track->sentences = NULL;
		for (int i = 0; i < mkv_ctx->sub_tracks_count; i++)
			if (strcmp((const char *)mkv_ctx->sub_tracks[i]->lang, (const char *)lang) == 0)
				sub_track->lang_index++;
		void *tmp = realloc(mkv_ctx->sub_tracks, sizeof(struct matroska_sub_track *) * (mkv_ctx->sub_tracks_count + 1));
		if (tmp == NULL)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In parse_segment_track_entry: Out of memory reallocating sub_tracks.");
		mkv_ctx->sub_tracks = tmp;
		mkv_ctx->sub_tracks[mkv_ctx->sub_tracks_count] = sub_track;
		mkv_ctx->sub_tracks_count++;
	}
	else
	{
		free(lang);
		if (lang_ietf)
			free(lang_ietf);
		if (codec_id_string)
			free(codec_id_string);
	}
}

// Read sequence parameter set for AVC
void parse_private_codec_data(struct matroska_ctx *mkv_ctx, char *codec_id_string, ULLONG track_number, char *lang)
{
	FILE *file = mkv_ctx->file;
	ULLONG len = read_vint_length(file);
	unsigned char *data = NULL;

	struct lib_cc_decode *dec_ctx = update_decoder_list(mkv_ctx->ctx);
	struct encoder_ctx *enc_ctx = update_encoder_list(mkv_ctx->ctx);

	if ((strcmp((const char *)codec_id_string, (const char *)avc_codec_id) == 0) && mkv_ctx->avc_track_number == track_number)
	{
		// Skip reserved data
		ULLONG reserved_len = 8;
		skip_bytes(file, reserved_len);

		ULLONG size = len - reserved_len;

		data = read_byte_block(file, size);
		do_NAL(enc_ctx, dec_ctx, data, size, &mkv_ctx->dec_sub);
	}
	else if ((strcmp((const char *)codec_id_string, (const char *)hevc_codec_id) == 0) && mkv_ctx->hevc_track_number == track_number)
	{
		// HEVC uses HEVCDecoderConfigurationRecord format
		// We need to parse this to extract VPS/SPS/PPS NAL units
		dec_ctx->avc_ctx->is_hevc = 1;

		data = read_byte_block(file, len);

		// HEVCDecoderConfigurationRecord structure:
		// - configurationVersion (1 byte)
		// - general_profile_space, general_tier_flag, general_profile_idc (1 byte)
		// - general_profile_compatibility_flags (4 bytes)
		// - general_constraint_indicator_flags (6 bytes)
		// - general_level_idc (1 byte)
		// - reserved + min_spatial_segmentation_idc (2 bytes)
		// - reserved + parallelismType (1 byte)
		// - reserved + chromaFormat (1 byte)
		// - reserved + bitDepthLumaMinus8 (1 byte)
		// - reserved + bitDepthChromaMinus8 (1 byte)
		// - avgFrameRate (2 bytes)
		// - constantFrameRate, numTemporalLayers, temporalIdNested, lengthSizeMinusOne (1 byte)
		// - numOfArrays (1 byte)
		// Total header: 23 bytes

		if (len >= 23)
		{
			uint8_t num_arrays = data[22];
			size_t offset = 23;

			for (uint8_t arr = 0; arr < num_arrays && offset < len; arr++)
			{
				if (offset + 3 > len)
					break;

				// uint8_t array_completeness = (data[offset] >> 7) & 1;
				// uint8_t nal_unit_type = data[offset] & 0x3F;
				offset++;

				uint16_t num_nalus = (data[offset] << 8) | data[offset + 1];
				offset += 2;

				for (uint16_t n = 0; n < num_nalus && offset < len; n++)
				{
					if (offset + 2 > len)
						break;

					uint16_t nal_unit_length = (data[offset] << 8) | data[offset + 1];
					offset += 2;

					if (offset + nal_unit_length > len)
						break;

					// Process this NAL unit (VPS, SPS, or PPS)
					do_NAL(enc_ctx, dec_ctx, &data[offset], nal_unit_length, &mkv_ctx->dec_sub);
					offset += nal_unit_length;
				}
			}
		}
	}
	else if (strcmp((const char *)codec_id_string, (const char *)dvb_codec_id) == 0)
	{
		enc_ctx->write_previous = 0;
		enc_ctx->is_mkv = 1;

		data = read_byte_block(file, len);

		unsigned char *codec_data = malloc(sizeof(char) * 8);
		if (codec_data == NULL)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In parse_private_codec_data: Out of memory.");
		// 1.ISO_639_language_code (3 bytes)
		// Use memcpy with bounds check instead of strcpy to prevent buffer overflow
		size_t lang_len = lang ? strlen(lang) : 0;
		if (lang_len > 3)
			lang_len = 3;
		memset(codec_data, ' ', 3); // Initialize with spaces (valid padding for language codes)
		if (lang_len > 0)
			memcpy(codec_data, lang, lang_len);
		// 2.subtitling_type (1 byte)
		codec_data[3] = data[4];
		// 3.composition_page_id (2 bytes)
		codec_data[4] = data[0];
		codec_data[5] = data[1];
		// 4.ancillary_page_id (2 bytes)
		codec_data[6] = data[2];
		codec_data[7] = data[3];

		struct dvb_config cnf;
		memset((void *)&cnf, 0, sizeof(struct dvb_config));

		parse_dvb_description(&cnf, codec_data, 8);
		dec_ctx->private_data = dvbsub_init_decoder(&cnf);

		free(codec_data);
	}
	else
	{
		skip_bytes(file, len);
		return;
	}

	free(data);
}

void parse_segment_tracks(struct matroska_ctx *mkv_ctx)
{
	FILE *file = mkv_ctx->file;
	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);

	int code = 0, code_len = 0;
	while (pos + len > get_current_byte(file))
	{
		code <<= 8;
		code += mkv_read_byte(file);
		if (feof(file))
			break;
		code_len++;

		switch (code)
		{
			/* Tracks ids*/
			case MATROSKA_SEGMENT_TRACK_ENTRY:

				parse_segment_track_entry(mkv_ctx);
				MATROSKA_SWITCH_BREAK(code, code_len);

				/* Misc ids */
			case MATROSKA_VOID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_CRC32:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			default:
				if (code_len == MATROSKA_MAX_ID_LENGTH)
				{
					mprint(MATROSKA_WARNING "Unknown element 0x%x at position " LLD ", skipping this element\n", code,
					       get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
					// Skip just the unknown element, not the entire block
					read_vint_block_skip(file);
					// Reset code and code_len to start fresh with next element
					code = 0;
					code_len = 0;
				}
				break;
		}
	}
}

void parse_segment(struct matroska_ctx *mkv_ctx)
{
	FILE *file = mkv_ctx->file;
	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);

	int code = 0, code_len = 0;
	while (pos + len > get_current_byte(file))
	{
		code <<= 8;
		code += mkv_read_byte(file);
		if (feof(file))
			break;
		code_len++;
		switch (code)
		{
			/* Segment ids */
			case MATROSKA_SEGMENT_SEEK_HEAD:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_INFO:
				parse_segment_info(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CLUSTER:
				// read_vint_block_skip(file);
				parse_segment_cluster(mkv_ctx);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TRACKS:
				parse_segment_tracks(mkv_ctx);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CUES:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_ATTACHMENTS:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_CHAPTERS:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_TAGS:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);

				/* Misc ids */
			case MATROSKA_VOID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_CRC32:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			default:
				if (code_len == MATROSKA_MAX_ID_LENGTH)
				{
					mprint(MATROSKA_WARNING "Unknown element 0x%x at position " LLD ", skipping this element\n", code,
					       get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
					// Skip just the unknown element, not the entire block
					read_vint_block_skip(file);
					// Reset code and code_len to start fresh with next element
					code = 0;
					code_len = 0;
				}
				break;
		}
	}
}

char *generate_filename_from_track(struct matroska_ctx *mkv_ctx, struct matroska_sub_track *track)
{
	// Use lang_ietf if available, otherwise fall back to lang
	const char *lang_to_use = track->lang_ietf ? track->lang_ietf : track->lang;
	const char *basename = get_basename(mkv_ctx->filename);
	const char *extension = matroska_track_text_subtitle_id_extensions[track->codec_id];

	// Calculate needed size: basename + "_" + lang + "_" + index + "." + extension + null
	size_t needed = strlen(basename) + strlen(lang_to_use) + strlen(extension) + 32;
	char *buf = malloc(needed);
	if (buf == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In generate_filename_from_track: Out of memory.");

	if (track->lang_index == 0)
		snprintf(buf, needed, "%s_%s.%s", basename, lang_to_use, extension);
	else
		snprintf(buf, needed, "%s_%s_" LLD ".%s", basename, lang_to_use,
			 track->lang_index, extension);
	return buf;
}

char *ass_ssa_sentence_erase_read_order(char *text)
{
	// crop text after second ','
	int cnt = 0;
	int index = 0;
	while (cnt < 2)
	{
		if (text[index] == ',')
			cnt++;
		index++;
	}
	size_t len = strlen(text) - index;
	char *buf = malloc(sizeof(char) * (len + 1));
	if (buf == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In ass_ssa_sentence_erase_read_order: Out of memory.");
	memcpy(buf, &text[index], len);
	buf[len] = '\0';
	return buf;
}

/* VOBSUB support: Generate PS Pack header
 * The PS Pack header is 14 bytes:
 * - 4 bytes: start code (00 00 01 ba)
 * - 6 bytes: SCR (System Clock Reference) in MPEG-2 format
 * - 3 bytes: mux rate
 * - 1 byte: stuffing length (0)
 */
static void generate_ps_pack_header(unsigned char *buf, ULLONG pts_90khz)
{
	// PS Pack start code
	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x01;
	buf[3] = 0xBA;

	// SCR (System Clock Reference) - use PTS as SCR base, SCR extension = 0
	// MPEG-2 format: 01 SCR[32:30] 1 SCR[29:15] 1 SCR[14:0] 1 SCR_ext[8:0] 1
	ULLONG scr = pts_90khz;
	ULLONG scr_base = scr;
	int scr_ext = 0;

	buf[4] = 0x44 | ((scr_base >> 27) & 0x38) | ((scr_base >> 28) & 0x03);
	buf[5] = (scr_base >> 20) & 0xFF;
	buf[6] = 0x04 | ((scr_base >> 12) & 0xF8) | ((scr_base >> 13) & 0x03);
	buf[7] = (scr_base >> 5) & 0xFF;
	buf[8] = 0x04 | ((scr_base << 3) & 0xF8) | ((scr_ext >> 7) & 0x03);
	buf[9] = ((scr_ext << 1) & 0xFE) | 0x01;

	// Mux rate (10080 = standard DVD rate)
	int mux_rate = 10080;
	buf[10] = (mux_rate >> 14) & 0xFF;
	buf[11] = (mux_rate >> 6) & 0xFF;
	buf[12] = ((mux_rate << 2) & 0xFC) | 0x03;

	// Stuffing length = 0, with marker bits
	buf[13] = 0xF8;
}

/* VOBSUB support: Generate PES header for private stream 1
 * Returns the total header size (variable based on PTS)
 */
static int generate_pes_header(unsigned char *buf, ULLONG pts_90khz, int payload_size, int stream_id)
{
	// PES start code for private stream 1
	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x01;
	buf[3] = 0xBD; // Private stream 1

	// PES packet length = header data (3 + 5 for PTS) + 1 (substream ID) + payload
	int pes_header_data_len = 5; // PTS only
	int pes_packet_len = 3 + pes_header_data_len + 1 + payload_size;
	buf[4] = (pes_packet_len >> 8) & 0xFF;
	buf[5] = pes_packet_len & 0xFF;

	// PES flags: MPEG-2, original
	buf[6] = 0x81;
	// PTS_DTS_flags = 10 (PTS only)
	buf[7] = 0x80;
	// PES header data length
	buf[8] = pes_header_data_len;

	// PTS (5 bytes): '0010' | PTS[32:30] | '1' | PTS[29:15] | '1' | PTS[14:0] | '1'
	buf[9] = 0x21 | ((pts_90khz >> 29) & 0x0E);
	buf[10] = (pts_90khz >> 22) & 0xFF;
	buf[11] = 0x01 | ((pts_90khz >> 14) & 0xFE);
	buf[12] = (pts_90khz >> 7) & 0xFF;
	buf[13] = 0x01 | ((pts_90khz << 1) & 0xFE);

	// Substream ID (0x20 = first VOBSUB stream)
	buf[14] = 0x20 + stream_id;

	return 15; // Total PES header size
}

/* VOBSUB support: Generate timestamp string for .idx file
 * Format: HH:MM:SS:mmm (where mmm is milliseconds)
 */
static void generate_vobsub_timestamp(char *buf, size_t bufsize, ULLONG milliseconds)
{
	ULLONG ms = milliseconds % 1000;
	milliseconds /= 1000;
	ULLONG seconds = milliseconds % 60;
	milliseconds /= 60;
	ULLONG minutes = milliseconds % 60;
	milliseconds /= 60;
	ULLONG hours = milliseconds;

	snprintf(buf, bufsize, "%02" LLU_M ":%02" LLU_M ":%02" LLU_M ":%03" LLU_M,
		 hours, minutes, seconds, ms);
}

/* Check if output format is text-based (requires OCR for bitmap subtitles) */
static int is_text_output_format(enum ccx_output_format format)
{
	return (format == CCX_OF_SRT || format == CCX_OF_SSA ||
		format == CCX_OF_WEBVTT || format == CCX_OF_TRANSCRIPT ||
		format == CCX_OF_SAMI || format == CCX_OF_SMPTETT);
}

/* VOBSUB support: Process VOBSUB track with OCR and output text format */
static void process_vobsub_track_ocr(struct matroska_ctx *mkv_ctx, struct matroska_sub_track *track)
{
	if (track->sentence_count == 0)
	{
		mprint("\nNo VOBSUB subtitles to process");
		return;
	}

	/* Check if OCR is available */
	if (!vobsub_ocr_available())
	{
		fatal(EXIT_NOT_CLASSIFIED,
		      "VOBSUB to text conversion requires OCR support.\n"
		      "Please rebuild CCExtractor with -DWITH_OCR=ON or use raw output (--out=idx)");
	}

	/* Initialize VOBSUB decoder */
	struct vobsub_ctx *vob_ctx = init_vobsub_decoder();
	if (!vob_ctx)
	{
		fatal(EXIT_NOT_CLASSIFIED,
		      "VOBSUB to text conversion requires OCR, but initialization failed.\n"
		      "Please ensure Tesseract is installed with language data.");
	}

	/* Parse palette from track header (CodecPrivate) */
	if (track->header)
	{
		vobsub_parse_palette(vob_ctx, track->header);
	}

	mprint("\nProcessing VOBSUB track with OCR (%d subtitles)", track->sentence_count);

	/* Get encoder context for output */
	struct encoder_ctx *enc_ctx = update_encoder_list(mkv_ctx->ctx);

	/* Process each subtitle */
	for (int i = 0; i < track->sentence_count; i++)
	{
		struct matroska_sub_sentence *sentence = track->sentences[i];
		mkv_ctx->sentence_count++;

		/* Calculate end time (use next subtitle start if not specified) */
		ULLONG end_time = sentence->time_end;
		if (end_time == 0 && i + 1 < track->sentence_count)
		{
			end_time = track->sentences[i + 1]->time_start - 1;
		}
		else if (end_time == 0)
		{
			end_time = sentence->time_start + 5000; /* Default 5 second duration */
		}

		/* Decode SPU and run OCR */
		struct cc_subtitle sub;
		memset(&sub, 0, sizeof(sub));

		int ret = vobsub_decode_spu(vob_ctx,
					    (unsigned char *)sentence->text,
					    sentence->text_size,
					    sentence->time_start,
					    end_time,
					    &sub);

		if (ret == 0 && sub.got_output)
		{
			/* Encode the subtitle to output format */
			encode_sub(enc_ctx, &sub);

			/* Free subtitle data */
			if (sub.data)
			{
				struct cc_bitmap *rect = (struct cc_bitmap *)sub.data;
				for (int j = 0; j < sub.nb_data; j++)
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
				free(sub.data);
			}
		}

		/* Progress indicator */
		if ((i + 1) % 50 == 0 || i + 1 == track->sentence_count)
		{
			mprint("\rProcessing VOBSUB: %d/%d subtitles", i + 1, track->sentence_count);
		}
	}

	delete_vobsub_decoder(&vob_ctx);
	mprint("\nVOBSUB OCR processing complete");
}

/* VOBSUB support: Save VOBSUB track to .idx and .sub files */
#define VOBSUB_BLOCK_SIZE 2048
static void save_vobsub_track(struct matroska_ctx *mkv_ctx, struct matroska_sub_track *track)
{
	if (track->sentence_count == 0)
	{
		mprint("\nNo VOBSUB subtitles to write");
		return;
	}

	// Generate base filename (without extension)
	const char *lang_to_use = track->lang_ietf ? track->lang_ietf : track->lang;
	const char *basename = get_basename(mkv_ctx->filename);
	size_t needed = strlen(basename) + strlen(lang_to_use) + 32;
	char *base_filename = malloc(needed);
	if (base_filename == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In save_vobsub_track: Out of memory.");

	if (track->lang_index == 0)
		snprintf(base_filename, needed, "%s_%s", basename, lang_to_use);
	else
		snprintf(base_filename, needed, "%s_%s_" LLD, basename, lang_to_use, track->lang_index);

	// Create .sub filename
	char *sub_filename = malloc(needed + 5);
	if (sub_filename == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In save_vobsub_track: Out of memory.");
	snprintf(sub_filename, needed + 5, "%s.sub", base_filename);

	// Create .idx filename
	char *idx_filename = malloc(needed + 5);
	if (idx_filename == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In save_vobsub_track: Out of memory.");
	snprintf(idx_filename, needed + 5, "%s.idx", base_filename);

	mprint("\nOutput files: %s, %s", idx_filename, sub_filename);

	// Open .sub file
	int sub_desc;
#ifdef WIN32
	sub_desc = open(sub_filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
#else
	sub_desc = open(sub_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
#endif
	if (sub_desc < 0)
	{
		mprint("\nError: Cannot create .sub file");
		free(base_filename);
		free(sub_filename);
		free(idx_filename);
		return;
	}

	// Open .idx file
	int idx_desc;
#ifdef WIN32
	idx_desc = open(idx_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
#else
	idx_desc = open(idx_filename, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
#endif
	if (idx_desc < 0)
	{
		mprint("\nError: Cannot create .idx file");
		close(sub_desc);
		free(base_filename);
		free(sub_filename);
		free(idx_filename);
		return;
	}

	// Write .idx header (from CodecPrivate)
	if (track->header != NULL)
		write_wrapped(idx_desc, track->header, strlen(track->header));

	// Add language identifier line
	char lang_line[128];
	snprintf(lang_line, sizeof(lang_line), "\nid: %s, index: 0\n", lang_to_use);
	write_wrapped(idx_desc, lang_line, strlen(lang_line));

	// Buffer for PS/PES headers and padding
	unsigned char header_buf[32];
	unsigned char zero_buf[VOBSUB_BLOCK_SIZE];
	memset(zero_buf, 0, VOBSUB_BLOCK_SIZE);

	ULLONG file_pos = 0;

	// Write each subtitle
	for (int i = 0; i < track->sentence_count; i++)
	{
		struct matroska_sub_sentence *sentence = track->sentences[i];
		mkv_ctx->sentence_count++;

		// Convert timestamp to 90kHz PTS
		ULLONG pts_90khz = sentence->time_start * 90;

		// Write timestamp entry to .idx
		char timestamp[32];
		generate_vobsub_timestamp(timestamp, sizeof(timestamp), sentence->time_start);
		char idx_entry[128];
		snprintf(idx_entry, sizeof(idx_entry), "timestamp: %s, filepos: %09" LLX_M "\n",
			 timestamp, file_pos);
		write_wrapped(idx_desc, idx_entry, strlen(idx_entry));

		// Generate PS Pack header (14 bytes)
		generate_ps_pack_header(header_buf, pts_90khz);
		write_wrapped(sub_desc, (char *)header_buf, 14);

		// Generate PES header (15 bytes)
		int pes_header_len = generate_pes_header(header_buf, pts_90khz, sentence->text_size, 0);
		write_wrapped(sub_desc, (char *)header_buf, pes_header_len);

		// Write SPU data
		write_wrapped(sub_desc, sentence->text, sentence->text_size);

		// Calculate bytes written and pad to block boundary
		ULLONG bytes_written = 14 + pes_header_len + sentence->text_size;
		ULLONG padding_needed = VOBSUB_BLOCK_SIZE - (bytes_written % VOBSUB_BLOCK_SIZE);
		if (padding_needed < VOBSUB_BLOCK_SIZE)
		{
			write_wrapped(sub_desc, (char *)zero_buf, padding_needed);
			bytes_written += padding_needed;
		}

		file_pos += bytes_written;
	}

	close(sub_desc);
	close(idx_desc);
	free(base_filename);
	free(sub_filename);
	free(idx_filename);
}

void save_sub_track(struct matroska_ctx *mkv_ctx, struct matroska_sub_track *track)
{
	char *filename;
	int desc;

	// VOBSUB tracks need special handling
	if (track->codec_id == MATROSKA_TRACK_SUBTITLE_CODEC_ID_VOBSUB)
	{
		// Check if user wants text output (SRT, SSA, WebVTT, etc.)
		if (ccx_options.write_format_rewritten &&
		    is_text_output_format(ccx_options.enc_cfg.write_format))
		{
			// Use OCR to convert VOBSUB to text
			process_vobsub_track_ocr(mkv_ctx, track);
		}
		else
		{
			// Output raw idx/sub files
			save_vobsub_track(mkv_ctx, track);
		}
		return;
	}

	if (mkv_ctx->ctx->cc_to_stdout == CCX_TRUE)
	{
		desc = 1; // file descriptor of stdout
	}
	else
	{
		filename = generate_filename_from_track(mkv_ctx, track);
		mprint("\nOutput file: %s", filename);
#ifdef WIN32
		desc = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IREAD | S_IWRITE);
#else
		desc = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IWUSR | S_IRUSR);
#endif
		free(filename);
	}

	if (desc < 0)
	{
		mprint("\nError: Cannot create output file for subtitle track\n");
		return;
	}

	if (track->header != NULL)
		write_wrapped(desc, track->header, strlen(track->header));

	for (int i = 0; i < track->sentence_count; i++)
	{
		struct matroska_sub_sentence *sentence = track->sentences[i];
		mkv_ctx->sentence_count++;

		if (track->codec_id == MATROSKA_TRACK_SUBTITLE_CODEC_ID_WEBVTT)
		{
			write_wrapped(desc, "\n\n", 2);

			struct block_addition *blockaddition = sentence->blockaddition;

			// writing comment
			if (blockaddition != NULL)
			{
				if (blockaddition->comment != NULL)
				{
					write_wrapped(desc, sentence->blockaddition->comment, sentence->blockaddition->comment_size);
					write_wrapped(desc, "\n", 1);
				}
			}

			// writing cue identifier
			if (blockaddition != NULL)
			{
				if (blockaddition->cue_identifier != NULL)
				{
					write_wrapped(desc, blockaddition->cue_identifier, blockaddition->cue_identifier_size);
					write_wrapped(desc, "\n", 1);
				}
				else if (blockaddition->comment != NULL)
				{
					write_wrapped(desc, "\n", 1);
				}
			}

			// writing cue
			char *timestamp_start = malloc(sizeof(char) * 80); // being generous
			if (timestamp_start == NULL)
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In save_sub_track: Out of memory.");
			timestamp_to_vtttime(sentence->time_start, timestamp_start);
			ULLONG time_end = sentence->time_end;
			if (i + 1 < track->sentence_count)
				time_end = MIN(time_end, track->sentences[i + 1]->time_start - 1);
			char *timestamp_end = malloc(sizeof(char) * 80);
			if (timestamp_end == NULL)
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In save_sub_track: Out of memory.");
			timestamp_to_vtttime(time_end, timestamp_end);

			write_wrapped(desc, timestamp_start, strlen(timestamp_start));
			write_wrapped(desc, " --> ", 5);
			write_wrapped(desc, timestamp_end, strlen(timestamp_start));

			// writing cue settings list
			if (blockaddition != NULL)
			{
				if (blockaddition->cue_settings_list != NULL)
				{
					write_wrapped(desc, " ", 1);
					write_wrapped(desc, blockaddition->cue_settings_list, blockaddition->cue_settings_list_size);
				}
			}
			write_wrapped(desc, "\n", 1);

			int size = 0;
			while (*(sentence->text + size) == '\n' || *(sentence->text + size) == '\r')
				size++;
			write_wrapped(desc, sentence->text + size, sentence->text_size - size);

			free(timestamp_start);
			free(timestamp_end);
		}
		else if (track->codec_id == MATROSKA_TRACK_SUBTITLE_CODEC_ID_UTF8)
		{
			char number[16];
			snprintf(number, sizeof(number), "%d", i + 1);
			char *timestamp_start = malloc(sizeof(char) * 80); // being generous
			if (timestamp_start == NULL)
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In save_sub_track: Out of memory.");
			timestamp_to_srttime(sentence->time_start, timestamp_start);
			ULLONG time_end = sentence->time_end;
			if (i + 1 < track->sentence_count)
				time_end = MIN(time_end, track->sentences[i + 1]->time_start - 1);
			char *timestamp_end = malloc(sizeof(char) * 80);
			if (timestamp_end == NULL)
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In save_sub_track: Out of memory.");
			timestamp_to_srttime(time_end, timestamp_end);

			write_wrapped(desc, number, strlen(number));
			write_wrapped(desc, "\n", 1);
			write_wrapped(desc, timestamp_start, strlen(timestamp_start));
			write_wrapped(desc, " --> ", 5);
			write_wrapped(desc, timestamp_end, strlen(timestamp_start));
			write_wrapped(desc, "\n", 1);
			int size = 0;
			while (*(sentence->text + size) == '\n' || *(sentence->text + size) == '\r')
				size++;
			write_wrapped(desc, sentence->text + size, sentence->text_size - size);

			if (sentence->text[sentence->text_size - 1] == '\n')
			{
				write_wrapped(desc, "\n", 1);
			}
			else
			{
				write_wrapped(desc, "\n\n", 2);
			}

			free(timestamp_start);
			free(timestamp_end);
		}
		else if (track->codec_id == MATROSKA_TRACK_SUBTITLE_CODEC_ID_ASS || track->codec_id == MATROSKA_TRACK_SUBTITLE_CODEC_ID_SSA)
		{
			char *timestamp_start = generate_timestamp_ass_ssa(sentence->time_start);
			ULLONG time_end = sentence->time_end;
			if (i + 1 < track->sentence_count)
				time_end = MIN(time_end, track->sentences[i + 1]->time_start - 1);
			char *timestamp_end = generate_timestamp_ass_ssa(time_end);

			write_wrapped(desc, "Dialogue: Marked=0,", strlen("Dialogue: Marked=0,"));
			write_wrapped(desc, timestamp_start, strlen(timestamp_start));
			write_wrapped(desc, ",", 1);
			write_wrapped(desc, timestamp_end, strlen(timestamp_start));
			write_wrapped(desc, ",", 1);
			char *text = ass_ssa_sentence_erase_read_order(sentence->text);
			char *text_to_free = text; // Save original pointer for freeing
			while ((text[0] == '\\') && (text[1] == 'n' || text[1] == 'N'))
				text += 2;
			write_wrapped(desc, text, strlen(text));
			write_wrapped(desc, "\n", 1);

			free(text_to_free);
			free(timestamp_start);
			free(timestamp_end);
		}
	}

	if (desc != 1)
		close(desc);
}

void free_sub_track(struct matroska_sub_track *track)
{
	if (track->header != NULL)
		free(track->header);
	if (track->lang != NULL)
		free(track->lang);
	if (track->lang_ietf != NULL)
		free(track->lang_ietf);
	if (track->codec_id_string != NULL)
		free(track->codec_id_string);
	for (int i = 0; i < track->sentence_count; i++)
	{
		struct matroska_sub_sentence *sentence = track->sentences[i];
		free(sentence->text);
		free(sentence);
	}
	if (track->sentences != NULL)
		free(track->sentences);
	free(track);
}

void matroska_save_all(struct matroska_ctx *mkv_ctx, char *lang)
{
	char *match;
	for (int i = 0; i < mkv_ctx->sub_tracks_count; i++)
	{
		if (lang)
		{
			// Try to match against IETF tag first if available
			if (mkv_ctx->sub_tracks[i]->lang_ietf &&
			    (match = strstr(lang, mkv_ctx->sub_tracks[i]->lang_ietf)) != NULL)
				save_sub_track(mkv_ctx, mkv_ctx->sub_tracks[i]);
			// Fall back to 3-letter code
			else if ((match = strstr(lang, mkv_ctx->sub_tracks[i]->lang)) != NULL)
				save_sub_track(mkv_ctx, mkv_ctx->sub_tracks[i]);
		}
		else
			save_sub_track(mkv_ctx, mkv_ctx->sub_tracks[i]);
	}

	// EIA-608
	update_decoder_list(mkv_ctx->ctx);
	if (mkv_ctx->dec_sub.got_output)
	{
		struct encoder_ctx *enc_ctx = update_encoder_list(mkv_ctx->ctx);
		encode_sub(enc_ctx, &mkv_ctx->dec_sub);
	}
}

void matroska_free_all(struct matroska_ctx *mkv_ctx)
{
	for (int i = 0; i < mkv_ctx->sub_tracks_count; i++)
		free_sub_track(mkv_ctx->sub_tracks[i]);
	free(mkv_ctx->sub_tracks);
	free(mkv_ctx);
}

void matroska_parse(struct matroska_ctx *mkv_ctx)
{
	int code = 0, code_len = 0;

	mprint("\n");

	FILE *file = mkv_ctx->file;
	while (!feof(file))
	{
		code <<= 8;
		code += mkv_read_byte(file);
		// Check for EOF after reading - feof() is only set after a failed read
		if (feof(file))
			break;
		code_len++;

		switch (code)
		{
			/* Header ids*/
			case MATROSKA_EBML_HEADER:
				parse_ebml(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_SEGMENT_HEADER:
				parse_segment(mkv_ctx);
				MATROSKA_SWITCH_BREAK(code, code_len);

				/* Misc ids */
			case MATROSKA_VOID:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			case MATROSKA_CRC32:
				read_vint_block_skip(file);
				MATROSKA_SWITCH_BREAK(code, code_len);
			default:
				if (code_len == MATROSKA_MAX_ID_LENGTH)
				{
					mprint(MATROSKA_WARNING "Unknown element 0x%x at position " LLD ", skipping this element\n", code,
					       get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
					// Skip just the unknown element, not the entire block
					read_vint_block_skip(file);
					// Reset code and code_len to start fresh with next element
					code = 0;
					code_len = 0;
				}
				break;
		}
	}

	// Close file stream
	fclose(file);

	mprint("\n");
}

FILE *create_file(struct lib_ccx_ctx *ctx)
{
	char *filename = ctx->inputfile[ctx->current_file];
	FILE *file = fopen(filename, "rb");
	return file;
}

int matroska_loop(struct lib_ccx_ctx *ctx)
{
	if (ccx_options.write_format_rewritten)
	{
		/* Note: For VOBSUB tracks, text output formats (SRT, SSA, etc.) are
		 * supported via OCR. For other subtitle types, the native format is used. */
		if (!is_text_output_format(ccx_options.enc_cfg.write_format))
		{
			mprint(MATROSKA_WARNING "You are using --out=<format>, but Matroska parser extracts subtitles in their recorded format\n");
			mprint("--out=<format> will be ignored for non-VOBSUB tracks\n");
		}
	}

	// Don't need generated input file
	// Will read bytes by FILE*
	close_input_file(ctx);

	struct matroska_ctx *mkv_ctx = malloc(sizeof(struct matroska_ctx));
	if (mkv_ctx == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In matroska_loop: Out of memory allocating mkv_ctx.");
	mkv_ctx->ctx = ctx;
	mkv_ctx->sub_tracks_count = 0;
	mkv_ctx->sentence_count = 0;
	mkv_ctx->current_second = 0;
	mkv_ctx->filename = ctx->inputfile[ctx->current_file];
	mkv_ctx->file = create_file(ctx);
	mkv_ctx->sub_tracks = malloc(sizeof(struct matroska_sub_track **));
	if (mkv_ctx->sub_tracks == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In matroska_loop: Out of memory allocating sub_tracks.");
	// EIA-608/708
	memset(&mkv_ctx->dec_sub, 0, sizeof(mkv_ctx->dec_sub));
	mkv_ctx->avc_track_number = -1;
	mkv_ctx->hevc_track_number = -1;

	matroska_parse(mkv_ctx);

	// 100% done
	activity_progress(100, (int)(mkv_ctx->current_second / 60),
			  (int)(mkv_ctx->current_second % 60));

	matroska_save_all(mkv_ctx, ccx_options.mkvlang);

	// Save values before freeing mkv_ctx
	int sentence_count = mkv_ctx->sentence_count;
	int avc_track_found = mkv_ctx->avc_track_number > -1;
	int hevc_track_found = mkv_ctx->hevc_track_number > -1;
	int got_output = mkv_ctx->dec_sub.got_output;

	matroska_free_all(mkv_ctx);

	mprint("\n\n");

	// Report video tracks found
	if (avc_track_found && hevc_track_found)
		mprint("Found AVC and HEVC tracks. ");
	else if (avc_track_found)
		mprint("Found AVC track. ");
	else if (hevc_track_found)
		mprint("Found HEVC track. ");
	else
		mprint("Found no AVC/HEVC track. ");

	if (got_output)
		return 1;
	return sentence_count;
}
