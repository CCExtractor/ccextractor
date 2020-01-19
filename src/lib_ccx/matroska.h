#ifndef MATROSKA_H
#define MATROSKA_H

/* Output specificators */
#if (defined (WIN32) || defined (_WIN32_WCE)) && (defined(__MINGW32__) || !defined(__GNUC__))
#define LLD_M "I64d"
#define LLU_M "I64u"
#define LLD "%I64d"
#define LLU "%I64u"
#elif defined (__SYMBIAN32__)
#define LLD_M "d"
#define LLU_M "u"
#define LLD "%d"
#define LLU "%u"
#elif defined(__DARWIN__) || defined(__APPLE__)
#define LLD_M "lld"
#define LLU_M "llu"
#define LLD "%lld"
#define LLU "%llu"
#elif defined(_LP64) /* Unix 64 bits */
#define LLD_M "ld"
#define LLU_M "lu"
#define LLD "%ld"
#define LLU "%lu"
#else /* Unix 32 bits */
#define LLD_M "lld"
#define LLU_M "llu"
#define LLD "%lld"
#define LLU "%llu"
#endif

/* EBML header ids */
#define MATROSKA_EBML_HEADER 0x1A45DFA3
#define MATROSKA_EBML_VERSION 0x4286
#define MATROSKA_EBML_READ_VERSION 0x42F7
#define MATROSKA_EBML_MAX_ID_LENGTH 0x42F2
#define MATROSKA_EBML_MAX_SIZE_LENGTH 0x42F3
#define MATROSKA_EBML_DOC_TYPE 0x4282
#define MATROSKA_EBML_DOC_TYPE_VERSION 0x4287
#define MATROSKA_EBML_DOC_TYPE_READ_VERSION 0x4285

/* Segment ids */
#define MATROSKA_SEGMENT_HEADER 0x18538067
#define MATROSKA_SEGMENT_SEEK_HEAD 0x114D9B74
#define MATROSKA_SEGMENT_INFO 0x1549A966
#define MATROSKA_SEGMENT_CLUSTER 0x1F43B675
#define MATROSKA_SEGMENT_TRACKS 0x1654AE6B
#define MATROSKA_SEGMENT_CUES 0x1C53BB6B
#define MATROSKA_SEGMENT_ATTACHMENTS 0x1941A469
#define MATROSKA_SEGMENT_CHAPTERS 0x1043A770
#define MATROSKA_SEGMENT_TAGS 0x1254C367

/* Segment info ids */
#define MATROSKA_SEGMENT_INFO_SEGMENT_UID 0x73A4
#define MATROSKA_SEGMENT_INFO_SEGMENT_FILENAME 0x7384
#define MATROSKA_SEGMENT_INFO_PREV_UID 0x3CB923
#define MATROSKA_SEGMENT_INFO_PREV_FILENAME 0x3C83AB
#define MATROSKA_SEGMENT_INFO_NEXT_UID 0x3EB923
#define MATROSKA_SEGMENT_INFO_NEXT_FILENAME 0x3E83BB
#define MATROSKA_SEGMENT_INFO_SEGMENT_FAMILY 0x4444
#define MATROSKA_SEGMENT_INFO_CHAPTER_TRANSLATE 0x6924
#define MATROSKA_SEGMENT_INFO_TIMECODE_SCALE 0x2AD7B1
#define MATROSKA_SEGMENT_INFO_DURATION 0x4489
#define MATROSKA_SEGMENT_INFO_DATE_UTC 0x4461
#define MATROSKA_SEGMENT_INFO_TITLE 0x7BA9
#define MATROSKA_SEGMENT_MUXING_APP 0x4D80
#define MATROSKA_SEGMENT_WRITING_APP 0x5741

/* Segment cluster ids */
#define MATROSKA_SEGMENT_CLUSTER_TIMECODE 0xE7
#define MATROSKA_SEGMENT_CLUSTER_SILENT_TRACKS 0x5854
#define MATROSKA_SEGMENT_CLUSTER_POSITION 0xA7
#define MATROSKA_SEGMENT_CLUSTER_PREV_SIZE 0xAB
#define MATROSKA_SEGMENT_CLUSTER_SIMPLE_BLOCK 0xA3
#define MATROSKA_SEGMENT_CLUSTER_ENCRYPTED_BLOCK 0xAF

/* Segment cluster block group ids */
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP 0xA0
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK 0xA1
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_VIRTUAL 0xA2
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_ADDITIONS 0x75A1
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_BLOCK_DURATION 0x9B
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_PRIORITY 0xFA
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_BLOCK 0xFB
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_CODEC_STATE 0xA4
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_DISCARD_PADDING 0x75A2
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_SLICES 0x8E
#define MATROSKA_SEGMENT_CLUSTER_BLOCK_GROUP_REFERENCE_FRAME 0xC8

/* Segment tracks ids */
#define MATROSKA_SEGMENT_TRACK_ENTRY 0xAE
#define MATROSKA_SEGMENT_TRACK_TRACK_NUMBER 0xD7
#define MATROSKA_SEGMENT_TRACK_TRACK_UID 0x73C5
#define MATROSKA_SEGMENT_TRACK_TRACK_TYPE 0x83
#define MATROSKA_SEGMENT_TRACK_FLAG_ENABLED 0xB9
#define MATROSKA_SEGMENT_TRACK_FLAG_DEFAULT 0x88
#define MATROSKA_SEGMENT_TRACK_FLAG_FORCED 0x55AA
#define MATROSKA_SEGMENT_TRACK_FLAG_LACING 0x9C
#define MATROSKA_SEGMENT_TRACK_MIN_CACHE 0x6DE7
#define MATROSKA_SEGMENT_TRACK_MAX_CACHE 0x6DF8
#define MATROSKA_SEGMENT_TRACK_DEFAULT_DURATION 0x23E383
#define MATROSKA_SEGMENT_TRACK_DEFAULT_DECODED_FIELD_DURATION 0x234E7A
#define MATROSKA_SEGMENT_TRACK_MAX_BLOCK_ADDITION_ID 0x55EE
#define MATROSKA_SEGMENT_TRACK_NAME 0x536E
#define MATROSKA_SEGMENT_TRACK_LANGUAGE 0x22B59C
#define MATROSKA_SEGMENT_TRACK_CODEC_ID 0x86
#define MATROSKA_SEGMENT_TRACK_CODEC_PRIVATE 0x63A2
#define MATROSKA_SEGMENT_TRACK_CODEC_NAME 0x258688
#define MATROSKA_SEGMENT_TRACK_CODEC_ATTACHMENT_LINK 0x7446
#define MATROSKA_SEGMENT_TRACK_CODEC_DECODE_ALL 0xAA
#define MATROSKA_SEGMENT_TRACK_TRACK_OVERLAY 0x6FAB
#define MATROSKA_SEGMENT_TRACK_CODEC_DELAY 0x56AA
#define MATROSKA_SEGMENT_TRACK_SEEK_PRE_ROLL 0x56BB
#define MATROSKA_SEGMENT_TRACK_TRACK_TRANSLATE 0x6624
#define MATROSKA_SEGMENT_TRACK_VIDEO 0xE0
#define MATROSKA_SEGMENT_TRACK_AUDIO 0xE1
#define MATROSKA_SEGMENT_TRACK_TRACK_OPERATION 0xE2
#define MATROSKA_SEGMENT_TRACK_CONTENT_ENCODINGS 0x6D80

/* Misc ids */
#define MATROSKA_VOID 0xEC
#define MATROSKA_CRC32 0xBF

/* DEFENCE FROM THE FOOL - deprecated IDs */
#define MATROSKA_SEGMENT_TRACK_TRACK_TIMECODE_SCALE 0x23314F
#define MATROSKA_SEGMENT_TRACK_TRACK_OFFSET 0x537F

/* DivX trick track extenstions (in track entry) */
#define MATROSKA_SEGMENT_TRACK_TRICK_TRACK_UID 0xC0
#define MATROSKA_SEGMENT_TRACK_TRICK_TRACK_SEGMENT_UID 0xC1
#define MATROSKA_SEGMENT_TRACK_TRICK_TRACK_FLAG 0xC6
#define MATROSKA_SEGMENT_TRACK_TRICK_MASTER_TRACK_UID 0xC7
#define MATROSKA_SEGMENT_TRACK_TRICK_MASTER_TRACK_SEGMENT_UID 0xC4

/* Other defines */
#define MATROSKA_MAX_ID_LENGTH 4
#define MAX_FILE_NAME_SIZE 260

/* Enums */
enum matroska_track_entry_type {
    MATROSKA_TRACK_TYPE_VIDEO = 1,
    MATROSKA_TRACK_TYPE_AUDIO = 2,
    MATROSKA_TRACK_TYPE_COMPLEX = 3,
    MATROSKA_TRACK_TYPE_LOGO = 0x10,
    MATROSKA_TRACK_TYPE_SUBTITLE = 0x11,
    MATROSKA_TRACK_TYPE_BUTTONS = 0x12,
    MATROSKA_TRACK_TYPE_CONTROL = 0x20,
};

enum matroska_track_subtitle_codec_id {
    MATROSKA_TRACK_SUBTITLE_CODEC_ID_UTF8 = 0,
    MATROSKA_TRACK_SUBTITLE_CODEC_ID_SSA,
    MATROSKA_TRACK_SUBTITLE_CODEC_ID_ASS,
    MATROSKA_TRACK_SUBTITLE_CODEC_ID_USF,
    MATROSKA_TRACK_SUBTITLE_CODEC_ID_WEBVTT,
    MATROSKA_TRACK_SUBTITLE_CODEC_ID_BITMAP,
    MATROSKA_TRACK_SUBTITLE_CODEC_ID_VOBSUB,
    MATROSKA_TRACK_SUBTITLE_CODEC_ID_KATE
};

char* matroska_track_text_subtitle_id_strings[] = {
        "S_TEXT/UTF8",
        "S_TEXT/SSA",
        "S_TEXT/ASS",
        "S_TEXT/USF",
        "S_TEXT/WEBVTT",
        "S_IMAGE/BMP",
        "S_VOBSUB",
        "S_KATE"
};

char* matroska_track_text_subtitle_id_extensions[] = {
        "srt", "ssa", "ass",
        "usf", "vtt", "bmp",
        NULL, NULL  // Unknown
};

char* avc_codec_id = "V_MPEG4/ISO/AVC";
char* dvb_codec_id = "S_DVBSUB";

/* Messages */
#define MATROSKA_INFO "\nMatroska parser info: "
#define MATROSKA_WARNING "\nMatroska parser warning: "
#define MATROSKA_ERROR "\nMatroska parser error: "

/* Boilerplate code */
#define MATROSKA_SWITCH_BREAK(a,b) (a)=0;(b)=0;break

/* Structures */

struct block_addition {
	char* cue_settings_list;
	ULLONG cue_settings_list_size;
	char* cue_identifier;
	ULLONG cue_identifier_size;
	char* comment;
	ULLONG comment_size;
};

struct matroska_sub_sentence {
    char* text;
    ULLONG text_size;
    ULLONG time_start;
    ULLONG time_end;
	struct block_addition* blockaddition;
};

struct matroska_avc_frame {
    UBYTE *data;
    ULLONG len;
    ULLONG FTS;
};

struct matroska_sub_track {
    char* header;   // Style header for ASS/SSA (and other) subtitles
    char* lang;
    ULLONG track_number;
    ULLONG lang_index;
    enum matroska_track_subtitle_codec_id codec_id;
    char* codec_id_string;
    ULLONG last_timestamp;

    int sentence_count;
    struct matroska_sub_sentence** sentences;
};

struct matroska_ctx {
    struct matroska_sub_track** sub_tracks;
    struct lib_ccx_ctx* ctx;
    struct cc_subtitle dec_sub;
    int avc_track_number; // ID of AVC track. -1 if there is none
    int sub_tracks_count;
	int block_index;
    int sentence_count;
    char* filename;
    ULLONG current_second;
    FILE* file;
};

/* Bytestream and parser functions */
void skip_bytes(FILE* file, ULLONG n);
void set_bytes(FILE* file, ULLONG n);
ULLONG get_current_byte(FILE* file);
UBYTE* read_byte_block(FILE* file, ULLONG n);
char* read_bytes_signed(FILE* file, ULLONG n);
UBYTE mkv_read_byte(FILE* file);

ULLONG read_vint_length(FILE* file);
UBYTE* read_vint_block(FILE* file);
char* read_vint_block_signed(FILE* file);
ULLONG read_vint_block_int(FILE* file);
char* read_vint_block_string(FILE* file);
void read_vint_block_skip(FILE* file);

void parse_ebml(FILE* file);
void parse_segment_info(FILE* file);
struct matroska_sub_sentence* parse_segment_cluster_block_group_block(struct matroska_ctx* mkv_ctx, ULLONG cluster_timecode);
void parse_segment_cluster_block_group(struct matroska_ctx* mkv_ctx, ULLONG cluster_timecode);
void parse_segment_cluster(struct matroska_ctx* mkv_ctx);
void parse_simple_block(struct matroska_ctx* mkv_ctx, ULLONG frame_timestamp);
int process_avc_frame_mkv(struct matroska_ctx* mkv_ctx, struct matroska_avc_frame frame);
void parse_segment_track_entry(struct matroska_ctx* mkv_ctx);
void parse_private_codec_data(struct matroska_ctx* mkv_ctx, char* codec_id_string, ULLONG track_number, char* lang);
void parse_segment_tracks(struct matroska_ctx* mkv_ctx);
void parse_segment(struct matroska_ctx* mkv_ctx);

/* Writing and helper functions */
char* generate_timestamp_utf8(ULLONG milliseconds);
char* generate_timestamp_ass_ssa(ULLONG milliseconds);
int find_sub_track_index(struct matroska_ctx* mkv_ctx, ULLONG track_number);
char*   get_track_entry_type_description(enum matroska_track_entry_type type);
enum matroska_track_subtitle_codec_id get_track_subtitle_codec_id(char* codec_id);
char* generate_filename_from_track(struct matroska_ctx* mkv_ctx, struct matroska_sub_track* track);
char* ass_ssa_sentence_erase_read_order(char* text);
void save_sub_track(struct matroska_ctx* mkv_ctx, struct matroska_sub_track* track);
void free_sub_track(struct matroska_sub_track* track);
void matroska_save_all(struct matroska_ctx* mkv_ctx,char* lang);
void matroska_free_all(struct matroska_ctx* mkv_ctx);
void matroska_parse(struct matroska_ctx* mkv_ctx);
FILE* create_file(struct lib_ccx_ctx *ctx);

#endif // MATROSKA_H
