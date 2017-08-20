#include "lib_ccx.h"
#include "utility.h"
#include "matroska.h"
#include "ccx_encoders_helpers.h"
#include <limits.h>

void skip_bytes(FILE* file, ULLONG n) {
    FSEEK(file, n, SEEK_CUR);
}

void set_bytes(FILE* file, ULLONG n) {
    FSEEK(file, n, SEEK_SET);
}

ULLONG get_current_byte(FILE* file) {
    return (ULLONG) FTELL(file);
}

UBYTE* read_byte_block(FILE* file, ULLONG n) {
    UBYTE* buffer = malloc((size_t)(sizeof(UBYTE) * n));
    fread(buffer, 1, (size_t)n, file);
    return buffer;
}

char* read_bytes_signed(FILE* file, ULLONG n) {
    char* buffer = malloc((size_t)(sizeof(UBYTE) * (n + 1)));
    fread(buffer, 1, (size_t)n, file);
    buffer[n] = 0;
    return buffer;
}

UBYTE mkv_read_byte(FILE* file) {
    return (UBYTE)fgetc(file);
}

ULLONG read_vint_length(FILE* file) {
    UBYTE ch = mkv_read_byte(file);
    int cnt = 0;
    for (int i = 7; i >= 0; i--) {
        if ((ch & (1 << i)) != 0) {
            cnt = 8 - i;
            break;
        }
    }
    ch ^= (1 << (8 - cnt));
    ULLONG ret = ch;
    for (int i = 1; i < cnt; i++) {
        ret <<= 8;
        ret += mkv_read_byte(file);
    }
    return ret;
}

UBYTE* read_vint_block(FILE* file) {
    ULLONG len = read_vint_length(file);
    return read_byte_block(file, len);
}

char* read_vint_block_signed(FILE* file) {
    ULLONG len = read_vint_length(file);
    return read_bytes_signed(file, len);
}

ULLONG read_vint_block_int(FILE* file) {
    ULLONG len = read_vint_length(file);
    UBYTE* s = read_byte_block(file, len);

    ULLONG res = 0;
    for (int i = 0; i < len; i++) {
        res <<= 8;
        res += s[i];
    }

    free(s);
    return res;
}

char* read_vint_block_string(FILE* file) {
    return read_vint_block_signed(file);
}

void read_vint_block_skip(FILE* file) {
    ULLONG len = read_vint_length(file);
    skip_bytes(file, len);
}

void parse_ebml(FILE* file) {
    ULLONG len = read_vint_length(file);
    ULLONG pos = get_current_byte(file);

    printf("\n");

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += mkv_read_byte(file);
        code_len++;

        switch (code) {
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
                mprint("Document type: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
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
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    mprint(MATROSKA_ERROR "Unknown element 0x%x at position " LLD ", skipping EBML block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse_segment_info(FILE* file) {
    ULLONG len = read_vint_length(file);
    ULLONG pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += mkv_read_byte(file);
        code_len++;

        switch (code) {
            /* Segment info ids */
            case MATROSKA_SEGMENT_INFO_SEGMENT_UID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO_SEGMENT_FILENAME:
                mprint("Filename: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
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
                mprint("Title: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_MUXING_APP:
                mprint("Muxing app: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_WRITING_APP:
                mprint("Writing app: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);

                /* Misc ids */
            case MATROSKA_VOID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_CRC32:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    mprint(MATROSKA_ERROR "Unknown element 0x%x at position " LLD ", skipping segment info block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

char* generate_timestamp_ass_ssa(ULLONG milliseconds) {
    ULLONG millis = (milliseconds % 1000) / 10;
    milliseconds /= 1000;
    ULLONG seconds = milliseconds % 60;
    milliseconds /= 60;
    ULLONG minutes = milliseconds % 60;
    milliseconds /= 60;
    ULLONG hours = milliseconds;

    char* buf = malloc(sizeof(char) * 15);
    sprintf(buf, LLD ":%02" LLD_M ":%02" LLD_M ".%02" LLD_M, hours, minutes, seconds, millis);
    return buf;
}

int find_sub_track_index(struct matroska_ctx* mkv_ctx, ULLONG track_number) {
    for (int i = mkv_ctx->sub_tracks_count-1; i >=0 ; i--)
        if (mkv_ctx->sub_tracks[i]->track_number == track_number)
            return i;
    return -1;
}

struct matroska_sub_sentence* parse_segment_cluster_block_group_block(struct matroska_ctx* mkv_ctx, ULLONG cluster_timecode) {
    FILE* file = mkv_ctx->file;
    ULLONG len = read_vint_length(file);
    ULLONG pos = get_current_byte(file);
    ULLONG track_number = read_vint_length(file);     // track number is length, not int

    int sub_track_index = find_sub_track_index(mkv_ctx, track_number);
    if (sub_track_index == -1) {
        set_bytes(file, pos + len);
        return NULL;
    }
	mkv_ctx->block_index = sub_track_index;

    ULLONG timecode = mkv_read_byte(file);
    timecode <<= 8; timecode += mkv_read_byte(file);

    mkv_read_byte(file);    // skip one byte

    ULLONG size = pos + len - get_current_byte(file);
    char* message = read_bytes_signed(file, size);

    struct matroska_sub_sentence* sentence = malloc(sizeof(struct matroska_sub_sentence));
    sentence->text = message;
    sentence->text_size = size;
    sentence->time_start = timecode + cluster_timecode;
	sentence->blockaddition = NULL;

    struct matroska_sub_track* track = mkv_ctx->sub_tracks[sub_track_index];
    if (track->sentence_count==0){
      track->sentences = malloc(sizeof(struct matroska_sub_sentence*));
    }
    else
      track->sentences = realloc(track->sentences,(track->sentence_count+1)*sizeof(struct matroska_sub_sentence*));
    track->sentences[track->sentence_count] = sentence;
    track->sentence_count++;

    mkv_ctx->current_second = max(mkv_ctx->current_second, sentence->time_start / 1000);
    activity_progress((int) (get_current_byte(file) * 100 / mkv_ctx->ctx->inputsize),
                      (int) (mkv_ctx->current_second / 60),
                      (int) (mkv_ctx->current_second % 60));

    return sentence;
}

struct matroska_sub_sentence* parse_segment_cluster_block_group_block_additions(struct matroska_ctx* mkv_ctx, ULLONG cluster_timecode) {
	FILE* file = mkv_ctx->file;
	ULLONG len = read_vint_length(file);
	ULLONG pos = get_current_byte(file);

	// gets WebVTT track
	int sub_track_index = mkv_ctx->block_index;
	if (sub_track_index == 0) {
		set_bytes(file, pos + len);
		return NULL;
	}
	struct matroska_sub_track* track = mkv_ctx->sub_tracks[sub_track_index];

	for (int i = 0; i < 4; i++) // skip four bytes
		mkv_read_byte(file);

	ULLONG size = pos + len - get_current_byte(file);
	char* message = read_bytes_signed(file, size);

	// parses message into block addition
	struct block_addition *newBA = calloc(1, sizeof(struct block_addition)); 
	char* current = message;
	int lastIndex = 0;
	int item = 0;
	int item_size;
	for(int i=0; i<size && item<2; i++){
		if (*current == '\n') {
			*current = '\0';
			item_size = i - lastIndex;
			if (item_size != 0) {
				if (item == 0) {
					newBA->cue_settings_list = message + lastIndex;
					newBA->cue_settings_list_size = item_size;
				}
				else if (item == 1) {
					newBA->cue_identifier = message + lastIndex;
					newBA->cue_identifier_size = item_size;
				}
			}
			item++;
			lastIndex = i+1;
		}
		current++;
	}
	if (lastIndex < size) {
		item_size = size - lastIndex;
		newBA->comment = message + lastIndex;
		newBA->comment_size = item_size;
	}

	// attaching BlockAddition to the cue
	struct matroska_sub_sentence* sentence = track->sentences[track->sentence_count - 1];
	sentence->blockaddition = newBA;

	// returns the sentence (cue) that this BlockAddition is attached to
	return sentence;
}

void parse_segment_cluster_block_group(struct matroska_ctx* mkv_ctx, ULLONG cluster_timecode) {
    FILE* file = mkv_ctx->file;
    ULLONG len = read_vint_length(file);
    ULLONG pos = get_current_byte(file);

    ULLONG block_duration = ULONG_MAX;
    struct matroska_sub_sentence* new_sentence;
    struct matroska_sub_sentence** sentence_list = NULL;
    int sentence_count = 0;

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file))
    {
        code <<= 8;
        code += mkv_read_byte(file);
        code_len++;

        switch (code) {
            /* Segment cluster block group ids */
            case MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK:
                new_sentence = parse_segment_cluster_block_group_block(mkv_ctx, cluster_timecode);
                if (new_sentence != NULL) {
                    sentence_list = realloc(sentence_list, sizeof(struct matroska_sub_track*) * (sentence_count + 1));
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
                    mprint(MATROSKA_ERROR "Unknown element 0x%x at position " LLD ", skipping segment cluster block group\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
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
        else
            sentence_list[i]->time_end = sentence_list[i]->time_start + block_duration;

        if (ccx_options.gui_mode_reports) {
            ULLONG h1, m1, s1, ms1;
            millis_to_time(sentence_list[i]->time_start, &h1, &m1, &s1, &ms1);
            ULLONG h2, m2, s2, ms2;
            millis_to_time(sentence_list[i]->time_end, &h2, &m2, &s2, &ms2);

            char *text = sentence_list[i]->text;
            int length = strlen(text);
            int pos = 0;
            while (true) {
                fprintf(stderr, "###SUBTITLE#");
                if (pos == 0) {
                    fprintf(stderr, "%02u:%02u#%02u:%02u#",
                            h1 * 60 + m1, s1, h2 * 60 + m2, s2);
                } else {
                    fprintf(stderr, "##");
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

void parse_segment_cluster(struct matroska_ctx* mkv_ctx) {
    FILE* file = mkv_ctx->file;
    ULLONG len = read_vint_length(file);
    ULLONG pos = get_current_byte(file);

    ULLONG timecode = 0;

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += mkv_read_byte(file);
        code_len++;

        switch (code) {
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
                // Same as Block inside the Block Group, but we can't save subs in this structure
                read_vint_block_skip(file);
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
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    mprint(MATROSKA_ERROR "Unknown element 0x%x at position " LLD ", skipping segment cluster block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }

    // We already update activity progress in subtitle block, but we also need to show percents
    // in samples without captions
    activity_progress((int) (get_current_byte(file) * 100 / mkv_ctx->ctx->inputsize),
                      (int) (mkv_ctx->current_second / 60),
                      (int) (mkv_ctx->current_second % 60));
}

char* get_track_entry_type_description(enum matroska_track_entry_type type) {
    switch (type) {
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

enum matroska_track_subtitle_codec_id get_track_subtitle_codec_id(char* codec_id) {
    for (int i = MATROSKA_TRACK_SUBTITLE_CODEC_ID_UTF8; i <= MATROSKA_TRACK_SUBTITLE_CODEC_ID_KATE; i++)
        if (strcmp(codec_id, matroska_track_text_subtitle_id_strings[i]) == 0)
            return (enum matroska_track_subtitle_codec_id) i;
    return (enum matroska_track_subtitle_codec_id) 0;
}

void parse_segment_track_entry(struct matroska_ctx* mkv_ctx) {
    FILE* file = mkv_ctx->file;
    mprint("\nTrack entry:\n");

    ULLONG len = read_vint_length(file);
    ULLONG pos = get_current_byte(file);

    ULLONG track_number = 0;
    enum matroska_track_entry_type track_type = MATROSKA_TRACK_TYPE_VIDEO;
    char* lang = strdup("eng");
    char* header = NULL;
    char* codec_id_string = NULL;
    enum matroska_track_subtitle_codec_id codec_id = MATROSKA_TRACK_SUBTITLE_CODEC_ID_UTF8;

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += mkv_read_byte(file);
        code_len++;

        switch (code) {
            /* Track entry ids*/
            case MATROSKA_SEGMENT_TRACK_TRACK_NUMBER:
                track_number = read_vint_block_int(file);
                mprint("    Track number: " LLD "\n", track_number);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRACK_UID:
                mprint("    UID: " LLU "\n", read_vint_block_int(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_TRACK_TYPE:
                track_type = (enum matroska_track_entry_type) read_vint_block_int(file);
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
                mprint("    Name: %s\n", read_vint_block_string(file));
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_LANGUAGE:
                lang = read_vint_block_string(file);
                mprint("    Language: %s\n", lang);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CODEC_ID:
                codec_id_string = read_vint_block_string(file);
                codec_id = get_track_subtitle_codec_id(codec_id_string);
                mprint("    Codec ID: %s\n", codec_id_string);
                free(codec_id_string);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_TRACK_CODEC_PRIVATE:
                if (track_type == MATROSKA_TRACK_TYPE_SUBTITLE)
                    header = read_vint_block_string(file);
                else
                    read_vint_block_skip(file);
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

                /* DivX trick track extenstions */
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

                /* Misc ids */
            case MATROSKA_VOID:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_CRC32:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            default:
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    mprint(MATROSKA_ERROR "Unknown element 0x%x at position " LLD ", skipping segment track entry block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }

    if (track_type == MATROSKA_TRACK_TYPE_SUBTITLE)
    {
        struct matroska_sub_track* sub_track = malloc(sizeof(struct matroska_sub_track));
        sub_track->header = header;
        sub_track->lang = lang;
        sub_track->track_number = track_number;
        sub_track->lang_index = 0;
        sub_track->codec_id = codec_id;
        sub_track->sentence_count = 0;
		for (int i = 0; i < mkv_ctx->sub_tracks_count; i++)
			if (strcmp((const char *)mkv_ctx->sub_tracks[i]->lang, (const char *)lang) == 0)
				sub_track->lang_index++;
		mkv_ctx->sub_tracks = realloc(mkv_ctx->sub_tracks, sizeof(struct matroska_sub_track*) * (mkv_ctx->sub_tracks_count + 1));
		mkv_ctx->sub_tracks[mkv_ctx->sub_tracks_count] = sub_track;
        mkv_ctx->sub_tracks_count++;
    }
    else
        free(lang);
}

void parse_segment_tracks(struct matroska_ctx* mkv_ctx)
{
    FILE* file = mkv_ctx->file;
    ULLONG len = read_vint_length(file);
    ULLONG pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += mkv_read_byte(file);
        code_len++;

        switch (code) {
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
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    mprint(MATROSKA_ERROR "Unknown element 0x%x at position " LLD ", skipping segment tracks block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

void parse_segment(struct matroska_ctx* mkv_ctx)
{
    FILE* file = mkv_ctx->file;
    ULLONG len = read_vint_length(file);
    ULLONG pos = get_current_byte(file);

    int code = 0, code_len = 0;
    while (pos + len > get_current_byte(file)) {
        code <<= 8;
        code += mkv_read_byte(file);
        code_len++;
        switch (code) {
            /* Segment ids */
            case MATROSKA_SEGMENT_SEEK_HEAD:
                read_vint_block_skip(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_INFO:
                parse_segment_info(file);
                MATROSKA_SWITCH_BREAK(code, code_len);
            case MATROSKA_SEGMENT_CLUSTER:
                //read_vint_block_skip(file);
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
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    mprint(MATROSKA_ERROR "Unknown element 0x%x at position " LLD ", skipping segment block\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    set_bytes(file, pos + len);
                    return;
                }
                break;
        }
    }
}

char* generate_filename_from_track(struct matroska_ctx* mkv_ctx, struct matroska_sub_track* track)
{
    char* buf = malloc(sizeof(char) * 200);
    if (track->lang_index == 0)
        sprintf(buf, "%s_%s.%s", get_basename(mkv_ctx->filename), track->lang, matroska_track_text_subtitle_id_extensions[track->codec_id]);
    else
        sprintf(buf, "%s_%s_" LLD ".%s", get_basename(mkv_ctx->filename), track->lang, track->lang_index,
                matroska_track_text_subtitle_id_extensions[track->codec_id]);
    return buf;
}

char* ass_ssa_sentence_erase_read_order(char* text)
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
    char* buf = malloc(sizeof(char) * (len + 1));
    memcpy(buf, &text[index], len);
    buf[len] = '\0';
    return buf;
}

void save_sub_track(struct matroska_ctx* mkv_ctx, struct matroska_sub_track* track)
{
    char* filename = generate_filename_from_track(mkv_ctx, track);
    mprint("\nOutput file: %s", filename);
    int desc;
#ifdef WIN32
    desc = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IREAD | S_IWRITE);
#else
    desc = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IWUSR | S_IRUSR);
#endif
    free(filename);

    if (track->header != NULL)
        write(desc, track->header, strlen(track->header));

    for (int i = 0; i < track->sentence_count; i++)
    {
        struct matroska_sub_sentence* sentence = track->sentences[i];
        mkv_ctx->sentence_count++;

        if(track->codec_id == MATROSKA_TRACK_SUBTITLE_CODEC_ID_WEBVTT)
		{
			write(desc, "\n\n", 2);

			struct block_addition* blockaddition = sentence->blockaddition;

			// writing comment
			if (blockaddition!=NULL) {
				if (blockaddition->comment != NULL) {
					write(desc, sentence->blockaddition->comment, sentence->blockaddition->comment_size);
					write(desc, "\n", 1);
				}
			}

			// writing cue identifier
			if (blockaddition != NULL) {
				if (blockaddition->cue_identifier != NULL) {
					write(desc, blockaddition->cue_identifier, blockaddition->cue_identifier_size);
					write(desc, "\n", 1);
				}
				else if (blockaddition->comment != NULL) {
					write(desc, "\n", 1);
				}
			}

			// writing cue
			char *timestamp_start = malloc(sizeof(char) * 80);	//being generous
			timestamp_to_vtttime(sentence->time_start, timestamp_start);
			ULLONG time_end = sentence->time_end;
			if (i + 1 < track->sentence_count)
				time_end = MIN(time_end, track->sentences[i + 1]->time_start - 1);
			char *timestamp_end = malloc(sizeof(char) * 80);
			timestamp_to_vtttime(time_end, timestamp_end);

			write(desc, timestamp_start, strlen(timestamp_start));
			write(desc, " --> ", 5);
			write(desc, timestamp_end, strlen(timestamp_start));

			// writing cue settings list
			if (blockaddition != NULL) {
				if (blockaddition->cue_settings_list != NULL) {
					write(desc, " ", 1);
					write(desc, blockaddition->cue_settings_list, blockaddition->cue_settings_list_size);
				}
			}
			write(desc, "\n", 1);

			int size = 0;
			while (*(sentence->text + size) == '\n' || *(sentence->text + size) == '\r')
				size++;
			write(desc, sentence->text + size, sentence->text_size - size);

			free(timestamp_start);
			free(timestamp_end);
		}
		else if (track->codec_id == MATROSKA_TRACK_SUBTITLE_CODEC_ID_UTF8)
        {
            char number[9];
            sprintf(number, "%d", i + 1);
            char *timestamp_start = malloc(sizeof(char) * 80);	//being generous
            timestamp_to_srttime(sentence->time_start, timestamp_start);
            ULLONG time_end = sentence->time_end;
            if (i + 1 < track->sentence_count)
                time_end = MIN(time_end, track->sentences[i + 1]->time_start - 1);
            char *timestamp_end = malloc(sizeof(char) * 80);
            timestamp_to_srttime(time_end, timestamp_end);

            write(desc, number, strlen(number));
            write(desc, "\n", 1);
            write(desc, timestamp_start, strlen(timestamp_start));
            write(desc, " --> ", 5);
            write(desc, timestamp_end, strlen(timestamp_start));
            write(desc, "\n", 1);
            int size=0;
            while (*(sentence->text+size)=='\n' || *(sentence->text+size)=='\r' )
              size++;
            write(desc, sentence->text+size, sentence->text_size-size);
            write(desc, "\n\n", 2);

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

            write(desc, "Dialogue: Marked=0,", strlen("Dialogue: Marked=0,"));
            write(desc, timestamp_start, strlen(timestamp_start));
            write(desc, ",", 1);
            write(desc, timestamp_end, strlen(timestamp_start));
            write(desc, ",", 1);
            char* text = ass_ssa_sentence_erase_read_order(sentence->text);
            while((text[0]=='\\') &&  (text[1]=='n' || text[1]=='N'))
              text+=2;
            write(desc, text, strlen(text));
            write(desc, "\n", 1);

            free(timestamp_start);
            free(timestamp_end);
        }
    }
}

void free_sub_track(struct matroska_sub_track* track)
{
    if (track->header != NULL)
        free(track->header);
    if (track->lang != NULL)
        free(track->lang);
    for (int i = 0; i < track->sentence_count; i++)
    {
        struct matroska_sub_sentence* sentence = track->sentences[i];
        free(sentence->text);
        free(sentence);
    }
    free(track);
}

void matroska_save_all(struct matroska_ctx* mkv_ctx,char* lang)
{
  char* match;
    for (int i = 0; i < mkv_ctx->sub_tracks_count; i++){
      if (lang){
        if (match = strstr(lang,mkv_ctx->sub_tracks[i]->lang) != NULL)
          save_sub_track(mkv_ctx, mkv_ctx->sub_tracks[i]);
                }
      else
        save_sub_track(mkv_ctx, mkv_ctx->sub_tracks[i]);
      }
}

void matroska_free_all(struct matroska_ctx* mkv_ctx)
{
    for (int i = 0; i < mkv_ctx->sub_tracks_count; i++)
        free_sub_track(mkv_ctx->sub_tracks[i]);
    free(mkv_ctx);
}

void matroska_parse(struct matroska_ctx* mkv_ctx)
{
    int code = 0, code_len = 0;

    mprint("\n");

    FILE* file = mkv_ctx->file;
    while (!feof(file)) {
        code <<= 8;
        code += mkv_read_byte(file);
        code_len++;

        switch (code) {
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
                if (code_len == MATROSKA_MAX_ID_LENGTH) {
                    mprint(MATROSKA_ERROR "Unknown element 0x%x at position " LLD ", skipping file parsing\n", code,
                           get_current_byte(file) - MATROSKA_MAX_ID_LENGTH);
                    return;
                }
                break;
        }
    }


    // Close file stream
    fclose(file);

    mprint("\n");
}

FILE* create_file(struct lib_ccx_ctx *ctx)
{
    char* filename = ctx->inputfile[ctx->current_file];
    FILE* file = fopen(filename, "rb");
    return file;
}

int matroska_loop(struct lib_ccx_ctx *ctx)
{
    if (ccx_options.write_format_rewritten)
    {
        mprint(MATROSKA_WARNING "You are using -out=<format>, but Matroska parser extract subtitles in a recorded format\n");
        mprint("-out=<format> will be ignored\n");
    }

    // Don't need generated input file
    // Will read bytes by FILE*
    close_input_file(ctx);

    struct matroska_ctx *mkv_ctx = malloc(sizeof(struct matroska_ctx));
    mkv_ctx->ctx = ctx;
    mkv_ctx->sub_tracks_count = 0;
    mkv_ctx->sentence_count = 0;
    mkv_ctx->current_second = 0;
    mkv_ctx->filename = ctx->inputfile[ctx->current_file];
    mkv_ctx->file = create_file(ctx);
    mkv_ctx->sub_tracks = malloc(sizeof(struct matroska_sub_track**));

    matroska_parse(mkv_ctx);

    // 100% done
    activity_progress(100, (int) (mkv_ctx->current_second / 60),
                      (int) (mkv_ctx->current_second % 60));

    matroska_save_all(mkv_ctx,ccx_options.mkvlang);
    int sentence_count = mkv_ctx->sentence_count;
    matroska_free_all(mkv_ctx);

    mprint("\n");

    return sentence_count;
}
