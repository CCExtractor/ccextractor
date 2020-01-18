#ifndef CCX_CONSTANTS_H
#define CCX_CONSTANTS_H

#include "stdio.h"

#ifndef __cplusplus
#define false 0
#define true 1
#endif

extern const char *framerates_types[16];
extern const double framerates_values[16];

extern const char *aspect_ratio_types[16];
extern const char *pict_types[8];
extern const char *slice_types[10];
extern const char *cc_types[4];

extern const unsigned char BROADCAST_HEADER[4];
extern const unsigned char LITTLE_ENDIAN_BOM[2];
extern const unsigned char UTF8_BOM[3];
extern const unsigned char DVD_HEADER[8];
extern const unsigned char lc1[1];
extern const unsigned char lc2[1];
extern const unsigned char lc3[2];
extern const unsigned char lc4[2];
extern const unsigned char lc5[1];
extern const unsigned char lc6[1];

extern unsigned char rcwt_header[11];

#define ONEPASS 120                          /* Bytes we can always look ahead without going out of limits */
#define BUFSIZE (2048*1024+ONEPASS)          /* 2 Mb plus the safety pass */
#define MAX_CLOSED_CAPTION_DATA_PER_PICTURE 32
#define EIA_708_BUFFER_LENGTH   2048         // TODO: Find out what the real limit is
#define TS_PACKET_PAYLOAD_LENGTH     184     // From specs
#define SUBLINESIZE 2048                     // Max. length of a .srt line - TODO: Get rid of this
#define STARTBYTESLENGTH	(1024*1024)
#define UTF8_MAX_BYTES 6
#define XMLRPC_CHUNK_SIZE (64*1024)          // 64 Kb per chunk, to avoid too many realloc()

enum ccx_debug_message_types
{
	/* Each debug message now belongs to one of these types. Use bitmaps in case
	   we want one message to belong to more than one type. */
	CCX_DMT_PARSE = 1,               // Show information related to parsing the container
	CCX_DMT_VIDES = 2,               // Show video stream related information
	CCX_DMT_TIME = 4,                // Show GOP and PTS timing information
	CCX_DMT_VERBOSE = 8,             // Show lots of debugging output
	CCX_DMT_DECODER_608 = 0x10,      // Show CC-608 decoder debug?
	CCX_DMT_708 = 0x20,              // Show CC-708 decoder debug?
	CCX_DMT_DECODER_XDS = 0x40,      // Show XDS decoder debug?
	CCX_DMT_CBRAW = 0x80,            // Caption blocks with FTS timing
	CCX_DMT_GENERIC_NOTICES = 0x100, // Generic, always displayed even if no debug is selected
	CCX_DMT_TELETEXT = 0x200,        // Show teletext debug?
	CCX_DMT_PAT = 0x400,             // Program Allocation Table dump
	CCX_DMT_PMT = 0x800,             // Program Map Table dump
	CCX_DMT_LEVENSHTEIN = 0x1000,    // Levenshtein distance calculations
	CCX_DMT_DVB = 0x2000,			 // DVB 
#ifdef ENABLE_SHARING
	CCX_DMT_SHARE = 0x2000,          // Extracted captions sharing service
#endif //ENABLE_SHARING
	CCX_DMT_DUMPDEF = 0x4000         // Dump defective TS packets
};

// AVC NAL types
enum ccx_avc_nal_types
{
	CCX_NAL_TYPE_UNSPECIFIED_0 = 0,
	CCX_NAL_TYPE_CODED_SLICE_NON_IDR_PICTURE_1 = 1,
	CCX_NAL_TYPE_CODED_SLICE_PARTITION_A = 2,
	CCX_NAL_TYPE_CODED_SLICE_PARTITION_B = 3,
	CCX_NAL_TYPE_CODED_SLICE_PARTITION_C = 4,
	CCX_NAL_TYPE_CODED_SLICE_IDR_PICTURE = 5,
	CCX_NAL_TYPE_SEI = 6,
	CCX_NAL_TYPE_SEQUENCE_PARAMETER_SET_7 = 7,
	CCX_NAL_TYPE_PICTURE_PARAMETER_SET = 8,
	CCX_NAL_TYPE_ACCESS_UNIT_DELIMITER_9 = 9,
	CCX_NAL_TYPE_END_OF_SEQUENCE = 10,
	CCX_NAL_TYPE_END_OF_STREAM = 11,
	CCX_NAL_TYPE_FILLER_DATA = 12,
	CCX_NAL_TYPE_SEQUENCE_PARAMETER_SET_EXTENSION = 13,
	CCX_NAL_TYPE_PREFIX_NAL_UNIT = 14,
	CCX_NAL_TYPE_SUBSET_SEQUENCE_PARAMETER_SET = 15,
	CCX_NAL_TYPE_RESERVED_16 = 16,
	CCX_NAL_TYPE_RESERVED_17 = 18,
	CCX_NAL_TYPE_RESERVED_18 = 18,
	CCX_NAL_TYPE_CODED_SLICE_AUXILIARY_PICTURE = 19,
	CCX_NAL_TYPE_CODED_SLICE_EXTENSION = 20,
	CCX_NAL_TYPE_RESERVED_21 = 21,
	CCX_NAL_TYPE_RESERVED_22 = 22,
	CCX_NAL_TYPE_RESERVED_23 = 23,
	CCX_NAL_TYPE_UNSPECIFIED_24 = 24,
	CCX_NAL_TYPE_UNSPECIFIED_25 = 25,
	CCX_NAL_TYPE_UNSPECIFIED_26 = 26,
	CCX_NAL_TYPE_UNSPECIFIED_27 = 27,
	CCX_NAL_TYPE_UNSPECIFIED_28 = 28,
	CCX_NAL_TYPE_UNSPECIFIED_29 = 29,
	CCX_NAL_TYPE_UNSPECIFIED_30 = 30,
	CCX_NAL_TYPE_UNSPECIFIED_31 = 31
};

// MPEG-2 TS stream types
enum ccx_stream_type
{
	CCX_STREAM_TYPE_UNKNOWNSTREAM = 0,
	
	/* 
	The later constants are defined by MPEG-TS standard
	Explore at: https://exiftool.org/TagNames/M2TS.html 
	*/
	CCX_STREAM_TYPE_VIDEO_MPEG1            = 0x01,
	CCX_STREAM_TYPE_VIDEO_MPEG2            = 0x02,
	CCX_STREAM_TYPE_AUDIO_MPEG1            = 0x03,
	CCX_STREAM_TYPE_AUDIO_MPEG2            = 0x04,
	CCX_STREAM_TYPE_PRIVATE_TABLE_MPEG2    = 0x05,
	CCX_STREAM_TYPE_PRIVATE_MPEG2          = 0x06,
	CCX_STREAM_TYPE_MHEG_PACKETS           = 0x07,
	CCX_STREAM_TYPE_MPEG2_ANNEX_A_DSM_CC   = 0x08,
	CCX_STREAM_TYPE_ITU_T_H222_1           = 0x09,
	CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_A = 0x0A,
	CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_B = 0x0B,
	CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_C = 0x0C,
	CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_D = 0x0D,
	CCX_STREAM_TYPE_AUDIO_AAC              = 0x0f,
	CCX_STREAM_TYPE_VIDEO_MPEG4            = 0x10,
	CCX_STREAM_TYPE_VIDEO_H264             = 0x1b,
	CCX_STREAM_TYPE_PRIVATE_USER_MPEG2     = 0x80,
	CCX_STREAM_TYPE_AUDIO_AC3              = 0x81,
	CCX_STREAM_TYPE_AUDIO_HDMV_DTS         = 0x82,
	CCX_STREAM_TYPE_AUDIO_DTS              = 0x8a
};

enum ccx_mpeg_descriptor
{
	/* 
	The later constants are defined by ETSI EN 300 468 standard
	Explore at: https://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.11.01_60/en_300468v011101p.pdf 
	*/
	CCX_MPEG_DSC_REGISTRATION            = 0x05,
	CCX_MPEG_DSC_DATA_STREAM_ALIGNMENT   = 0x06,
	CCX_MPEG_DSC_ISO639_LANGUAGE         = 0x0A,
	CCX_MPEG_DSC_VBI_DATA_DESCRIPTOR     = 0x45,
	CCX_MPEG_DSC_VBI_TELETEXT_DESCRIPTOR = 0x46,
	CCX_MPEG_DSC_TELETEXT_DESCRIPTOR     = 0x56,
	CCX_MPEG_DSC_DVB_SUBTITLE            = 0x59,
	/* User defined */
	CCX_MPEG_DSC_CAPTION_SERVICE         = 0x86,
	CCX_MPEG_DESC_DATA_COMP              = 0xfd  // Consider to change DESC to DSC
};


enum
{
	CCX_MESSAGES_QUIET  = 0,
	CCX_MESSAGES_STDOUT = 1,
	CCX_MESSAGES_STDERR = 2
};

enum ccx_datasource
{
	CCX_DS_FILE    = 0,
	CCX_DS_STDIN   = 1,
	CCX_DS_NETWORK = 2,
	CCX_DS_TCP     = 3
};

enum ccx_output_format
{
	CCX_OF_RAW        = 0,
	CCX_OF_SRT        = 1,
	CCX_OF_SAMI       = 2,
	CCX_OF_TRANSCRIPT = 3,
	CCX_OF_RCWT       = 4,
	CCX_OF_NULL       = 5,
	CCX_OF_SMPTETT    = 6,
	CCX_OF_SPUPNG     = 7,
	CCX_OF_DVDRAW     = 8, // See -d at http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/SCC_TOOLS.HTML#CCExtract
	CCX_OF_WEBVTT     = 9,
	CCX_OF_SIMPLE_XML = 10,
	CCX_OF_G608       = 11,
	CCX_OF_CURL       = 12,
	CCX_OF_SSA        = 13,
	CCX_OF_MCC        = 14,
	CCX_OF_SCC        = 15,
	CCX_OF_CCD        = 16,
};

enum ccx_output_date_format
{
	ODF_NONE     = 0,
	ODF_HHMMSS   = 1,
	ODF_SECONDS  = 2,
	ODF_DATE     = 3,
	ODF_HHMMSSMS = 4    // HH:MM:SS,MILIS (.srt style)
};

enum ccx_stream_mode_enum
{
	CCX_SM_ELEMENTARY_OR_NOT_FOUND = 0,
	CCX_SM_TRANSPORT = 1,
	CCX_SM_PROGRAM = 2,
	CCX_SM_ASF = 3,
	CCX_SM_MCPOODLESRAW = 4,
	CCX_SM_RCWT = 5,     // Raw Captions With Time, not used yet.
	CCX_SM_MYTH = 6,     // Use the myth loop
	CCX_SM_MP4  = 7,     // MP4, ISO-
#ifdef WTV_DEBUG
	CCX_SM_HEX_DUMP = 8, // Hexadecimal dump generated by wtvccdump
#endif
	CCX_SM_WTV = 9,
#ifdef ENABLE_FFMPEG
	CCX_SM_FFMPEG = 10,
#endif
	CCX_SM_GXF = 11,
	CCX_SM_MKV = 12,
	CCX_SM_MXF = 13,

	CCX_SM_AUTODETECT = 16
};

enum ccx_encoding_type
{
	CCX_ENC_UNICODE = 0,
	CCX_ENC_LATIN_1 = 1,
	CCX_ENC_UTF_8   = 2,
	CCX_ENC_ASCII   = 3
};

enum ccx_bufferdata_type
{
	CCX_UNKNOWN = 0,
	CCX_PES = 1,
	CCX_RAW = 2,
	CCX_H264 = 3,
	CCX_HAUPPAGE = 4,
	CCX_TELETEXT = 5,
	CCX_PRIVATE_MPEG2_CC = 6,
	CCX_DVB_SUBTITLE = 7,
	CCX_ISDB_SUBTITLE = 8,
	/* BUffer where cc data contain 3 byte cc_valid ccdata 1 ccdata 2 */
	CCX_RAW_TYPE = 9,
	CCX_DVD_SUBTITLE = 10
};

enum ccx_frame_type
{
	CCX_FRAME_TYPE_RESET_OR_UNKNOWN = 0,
	CCX_FRAME_TYPE_I_FRAME = 1,
	CCX_FRAME_TYPE_P_FRAME = 2,
	CCX_FRAME_TYPE_B_FRAME = 3,
	CCX_FRAME_TYPE_D_FRAME = 4
};

typedef enum {
	NO  = 0,
	YES = 1,
	UNDEFINED = 0xff
} bool_t;

enum ccx_code_type
{
	CCX_CODEC_ANY      = 0,
	CCX_CODEC_TELETEXT = 1,
	CCX_CODEC_DVB      = 2,
	CCX_CODEC_ISDB_CC  = 3,
	CCX_CODEC_ATSC_CC  = 4,
	CCX_CODEC_NONE     = 5
};

/* Caption Distribution Packet */
enum cdp_section_type
{
	/* 
	The later constants are defined by SMPTE ST 334
	Purchase for 80$ at: https://ieeexplore.ieee.org/document/8255806
	*/
	CDP_SECTION_DATA     = 0x72,
	CDP_SECTION_SVC_INFO = 0x73,
	CDP_SECTION_FOOTER   = 0x74
};

/*
 * This Macro check whether descriptor tag is valid for teletext
 * codec or not.
 *
 * @param desc descriptor tag given for each stream
 *
 * @return if descriptor tag is valid then it return 1 otherwise 0
 *
 */

#define IS_VALID_TELETEXT_DESC(desc) ( ((desc) == CCX_MPEG_DSC_VBI_DATA_DESCRIPTOR )|| \
		( (desc) == CCX_MPEG_DSC_VBI_TELETEXT_DESCRIPTOR ) || \
		( (desc) == CCX_MPEG_DSC_TELETEXT_DESCRIPTOR ) )

/*
 * This  macro to be used when you want to find out whether you
 * should parse f_sel subtitle codec type or not
 *
 * @param u_sel pass the codec selected by user to be searched in
 *  all elementary stream, we ignore the not to be selected stream
 *  if we find stream this is selected stream. since setting
 *  selected stream and not selected to same codec does not
 *  make ay sense.
 *
 * @param u_nsel pass the codec selected by user not to be parsed
 *               we give false value if f_sel is equal to n_sel
 *               and vice versa true if ...
 *
 * @param f_sel pass the codec name whom you are testing to be feasible
 *              to parse.
 */
#define IS_FEASIBLE(u_sel,u_nsel,f_sel) ( ( (u_sel) == CCX_CODEC_ANY && (u_nsel) != (f_sel) ) || (u_sel) == (f_sel) )
#define CCX_TXT_FORBIDDEN		    0 // Ignore teletext packets
#define CCX_TXT_AUTO_NOT_YET_FOUND	1
#define CCX_TXT_IN_USE			    2 // Positive auto-detected, or forced, etc

#define NB_LANGUAGE 100
extern const char *language[NB_LANGUAGE];

#define DEF_VAL_STARTCREDITSNOTBEFORE 	"0"
// To catch the theme after the teaser in TV shows
#define DEF_VAL_STARTCREDITSNOTAFTER	"5:00"
#define DEF_VAL_STARTCREDITSFORATLEAST	"2"
#define DEF_VAL_STARTCREDITSFORATMOST	"5"
#define DEF_VAL_ENDCREDITSFORATLEAST	"2"
#define DEF_VAL_ENDCREDITSFORATMOST	    "5"
#endif
