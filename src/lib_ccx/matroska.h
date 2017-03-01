#ifndef MATROSKA_H
#define MATROSKA_H

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

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
#define MATROSKA_MAX_TRACKS 128
#define MATROSKA_MAX_SENTENCES 8192

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

/* Messages */
#define MATROSKA_INFO "Matroska parser info: "
#define MATROSKA_WARNING "Matroska parser warning: "
#define MATROSKA_ERROR "Matroska parser error: "

/* Boilerplate code */
#define MATROSKA_SWITCH_BREAK(a,b) (a)=0;(b)=0;break

/* Typedefs */
typedef ULLONG matroska_int;
typedef unsigned char matroska_byte;

/* Structures */
struct matroska_sub_sentence {
    char* text;
    matroska_int text_size;
    matroska_int time_start;
    matroska_int time_end;
};

struct matroska_sub_track {
    char* header;   // Style header for ASS/SSA (and other) subtitles
    char* lang;
    matroska_int track_number;
    matroska_int lang_index;
    enum matroska_track_subtitle_codec_id codec_id;

    int sentence_count;
    struct matroska_sub_sentence* sentences[MATROSKA_MAX_SENTENCES];
};

struct matroska_ctx {
    struct matroska_sub_track* sub_tracks[MATROSKA_MAX_TRACKS];
    int sub_tracks_count;
    char* filename;
    FILE* file;

    // Context must be out of this stuff...
    struct lib_ccx_ctx* ctx;
};

/* Temporarily closed due to refactoring */

/* Bytestream and parser functions */
/*void skip_bytes(FILE* file, matroska_int n);
void set_bytes(FILE* file, matroska_int n);
matroska_int get_current_byte(FILE* file);
matroska_byte* read_byte_block(FILE* file, matroska_int n);
matroska_byte read_byte(FILE* file);
matroska_int read_vint_length(FILE* file);
matroska_byte* read_vint_block(FILE* file);
matroska_byte* read_vint_block_with_len(FILE* file, matroska_int* ptr_len);
matroska_int read_vint_block_int(FILE* file);
//matroska_byte* read_vint_block_string(FILE* file);
void read_vint_block_skip(FILE* file);
void parse_ebml(FILE* file);
void parse_segment_info(FILE* file);
struct matroska_sub_sentence* parse_segment_cluster_block_group_block(FILE* file, matroska_int cluster_timecode);
void parse_segment_cluster_block_group(FILE* file, matroska_int cluster_timecode);
void parse_segment_cluster(FILE* file);
void parse_segment_track_entry(FILE* file);
void parse_segment_tracks(FILE* file);
void parse_segment(FILE* file);
void matroska_parse(FILE* file);*/

/* Writing and helper functions */
/*char* get_track_entry_type_description(enum matroska_track_entry_type type);
int find_sub_track_index(matroska_int track_number);
char* generate_filename_from_track(struct matroska_sub_track* track);*/

#endif // MATROSKA_H
