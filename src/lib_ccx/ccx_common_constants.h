#ifndef CCX_CONSTANTS_H
#define CCX_CONSTANTS_H

#include "stdio.h"

// ========== BOOLEAN DEFINITIONS FOR C COMPATIBILITY ==========
// PURPOSE: Define true/false for C (C++ has these built-in)
// NEEDED FOR: Boolean logic throughout CCExtractor codebase
#ifndef __cplusplus
#define false 0
#define true 1
#endif

// ========== EXTERNAL VARIABLE DECLARATIONS ==========
// PURPOSE: Declare arrays defined in other .c files for global access
// NEEDED FOR: Video format analysis, aspect ratio detection, frame type identification

// *** VIDEO FORMAT INFORMATION ***
extern const char *framerates_types[16];     // Frame rate descriptions ("23.976 fps", "29.97 fps", etc.)
extern const double framerates_values[16];   // Exact frame rate values (23.976, 29.97, etc.)
extern const char *aspect_ratio_types[16];   // Aspect ratio strings ("4:3", "16:9", etc.)
extern const char *pict_types[8];           // Picture type names ("I", "P", "B", "D" frames)
extern const char *slice_types[10];         // H.264 slice type descriptions
extern const char *cc_types[4];             // Closed caption types (Field 1/2, etc.)

// *** FILE FORMAT MAGIC BYTES ***
// NEEDED FOR: File format detection and validation
extern const unsigned char BROADCAST_HEADER[4];  // Broadcast file header signature
extern const unsigned char LITTLE_ENDIAN_BOM[2]; // Little endian byte order mark (FF FE)
extern const unsigned char UTF8_BOM[3];          // UTF-8 byte order mark (EF BB BF)
extern const unsigned char DVD_HEADER[8];        // DVD subtitle stream header
extern const unsigned char lc1[1];               // Line 21 caption data markers
extern const unsigned char lc2[1];               // (specific byte patterns for
extern const unsigned char lc3[2];               //  closed caption detection)
extern const unsigned char lc4[2];
extern const unsigned char lc5[1];
extern const unsigned char lc6[1];

// *** CCExtractor NATIVE FORMAT ***
extern unsigned char rcwt_header[11];            // Raw Captions With Time header

// ========== BUFFER SIZES AND LIMITS ==========
// PURPOSE: Define memory allocation sizes and processing limits
// NEEDED FOR: Safe memory management and preventing buffer overflows

#define ONEPASS 120                          // Safety buffer for lookahead parsing (120 bytes)
#define BUFSIZE (2048*1024+ONEPASS)          // Main processing buffer (2MB + safety)
#define MAX_CLOSED_CAPTION_DATA_PER_PICTURE 32  // Max CC bytes per video frame
#define EIA_708_BUFFER_LENGTH   2048         // CEA-708 digital caption buffer size
#define TS_PACKET_PAYLOAD_LENGTH     184     // MPEG-TS packet payload size (188 - 4 byte header)
#define SUBLINESIZE 2048                     // Maximum length of subtitle line (.srt format)
#define STARTBYTESLENGTH    (1024*1024)      // Initial file read size for format detection (1MB)
#define UTF8_MAX_BYTES 6                     // Maximum bytes in UTF-8 character encoding
#define XMLRPC_CHUNK_SIZE (64*1024)          // Network transfer chunk size (64KB)

// ========== DEBUG MESSAGE CATEGORIES ==========
// PURPOSE: Categorize debug output for selective logging
// NEEDED FOR: Debugging specific components without flooding logs
enum ccx_debug_message_types
{
    /* Each debug message belongs to one category. Bitmaps allow multiple categories per message */
    CCX_DMT_PARSE = 1,               // Container parsing (TS, MP4, MKV structure)
    CCX_DMT_VIDES = 2,               // Video stream analysis (GOP, frame types)
    CCX_DMT_TIME = 4,                // Timing information (PTS, DTS, timestamps)
    CCX_DMT_VERBOSE = 8,             // Verbose output (detailed processing info)
    CCX_DMT_DECODER_608 = 0x10,      // CEA-608 closed caption decoder debug
    CCX_DMT_708 = 0x20,              // CEA-708 digital caption decoder debug
    CCX_DMT_DECODER_XDS = 0x40,      // XDS (eXtended Data Services) decoder debug
    CCX_DMT_CBRAW = 0x80,            // Raw caption blocks with timing
    CCX_DMT_GENERIC_NOTICES = 0x100, // Important notices (always shown)
    CCX_DMT_TELETEXT = 0x200,        // European teletext subtitle debug
    CCX_DMT_PAT = 0x400,             // Program Association Table parsing
    CCX_DMT_PMT = 0x800,             // Program Map Table parsing (codec detection)
    CCX_DMT_LEVENSHTEIN = 0x1000,    // Text similarity calculations
    CCX_DMT_DVB = 0x2000,            // DVB (European) subtitle debug
    CCX_DMT_DUMPDEF = 0x4000         // Dump corrupted/defective packets
#ifdef ENABLE_SHARING
    CCX_DMT_SHARE = 0x8000,          // Caption sharing service debug
#endif //ENABLE_SHARING
};

// ========== H.264/AVC NAL UNIT TYPES ==========
// PURPOSE: Identify different NAL (Network Abstraction Layer) unit types in H.264 streams
// NEEDED FOR: Parsing H.264 video to find closed captions in SEI NAL units
// SOURCE: ITU-T H.264 standard
enum ccx_avc_nal_types
{
    CCX_NAL_TYPE_UNSPECIFIED_0 = 0,                    // Unspecified
    CCX_NAL_TYPE_CODED_SLICE_NON_IDR_PICTURE_1 = 1,   // Regular video slice
    CCX_NAL_TYPE_CODED_SLICE_PARTITION_A = 2,          // Slice partition A
    CCX_NAL_TYPE_CODED_SLICE_PARTITION_B = 3,          // Slice partition B  
    CCX_NAL_TYPE_CODED_SLICE_PARTITION_C = 4,          // Slice partition C
    CCX_NAL_TYPE_CODED_SLICE_IDR_PICTURE = 5,          // I-frame (keyframe)
    CCX_NAL_TYPE_SEI = 6,                              // ← CRITICAL: SEI contains closed captions!
    CCX_NAL_TYPE_SEQUENCE_PARAMETER_SET_7 = 7,         // SPS (codec parameters)
    CCX_NAL_TYPE_PICTURE_PARAMETER_SET = 8,            // PPS (picture parameters)
    CCX_NAL_TYPE_ACCESS_UNIT_DELIMITER_9 = 9,          // Access unit delimiter
    CCX_NAL_TYPE_END_OF_SEQUENCE = 10,                 // End of sequence
    CCX_NAL_TYPE_END_OF_STREAM = 11,                   // End of stream
    CCX_NAL_TYPE_FILLER_DATA = 12,                     // Filler data
    CCX_NAL_TYPE_SEQUENCE_PARAMETER_SET_EXTENSION = 13, // SPS extension
    CCX_NAL_TYPE_PREFIX_NAL_UNIT = 14,                 // Prefix NAL unit
    CCX_NAL_TYPE_SUBSET_SEQUENCE_PARAMETER_SET = 15,   // Subset SPS
    CCX_NAL_TYPE_RESERVED_16 = 16,                     // Reserved types 16-23
    CCX_NAL_TYPE_RESERVED_17 = 18,
    CCX_NAL_TYPE_RESERVED_18 = 18,
    CCX_NAL_TYPE_CODED_SLICE_AUXILIARY_PICTURE = 19,   // Auxiliary picture
    CCX_NAL_TYPE_CODED_SLICE_EXTENSION = 20,           // Slice extension
    CCX_NAL_TYPE_RESERVED_21 = 21,                     // Reserved types 21-31
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
// ========== H.265/HEVC NAL UNIT TYPES ==========
// PURPOSE: Identify different NAL unit types in H.265/HEVC streams  
// NEEDED FOR: Parsing HEVC video to find closed captions in SEI NAL units
// SOURCE: ITU-T H.265 standard
// enum ccx_hevc_nal_types
// {
//     // *** CODED SLICE NAL UNITS (0-9) ***
//     CCX_HEVC_NAL_TRAIL_N = 0,                    // Coded slice - trailing picture, non-reference
//     CCX_HEVC_NAL_TRAIL_R = 1,                    // Coded slice - trailing picture, reference
//     CCX_HEVC_NAL_TSA_N = 2,                      // Coded slice - temporal sub-layer access, non-reference  
//     CCX_HEVC_NAL_TSA_R = 3,                      // Coded slice - temporal sub-layer access, reference
//     CCX_HEVC_NAL_STSA_N = 4,                     // Coded slice - step-wise temporal sub-layer access, non-reference
//     CCX_HEVC_NAL_STSA_R = 5,                     // Coded slice - step-wise temporal sub-layer access, reference
//     CCX_HEVC_NAL_RADL_N = 6,                     // Coded slice - random access decodable leading picture, non-reference
//     CCX_HEVC_NAL_RADL_R = 7,                     // Coded slice - random access decodable leading picture, reference  
//     CCX_HEVC_NAL_RASL_N = 8,                     // Coded slice - random access skipped leading picture, non-reference
//     CCX_HEVC_NAL_RASL_R = 9,                     // Coded slice - random access skipped leading picture, reference
    
//     // *** RESERVED NAL UNITS (10-15) ***
//     CCX_HEVC_NAL_RSV_VCL_N10 = 10,               // Reserved non-reference VCL NAL unit
//     CCX_HEVC_NAL_RSV_VCL_N12 = 12,               // Reserved non-reference VCL NAL unit  
//     CCX_HEVC_NAL_RSV_VCL_N14 = 14,               // Reserved non-reference VCL NAL unit
//     CCX_HEVC_NAL_RSV_VCL_R11 = 11,               // Reserved reference VCL NAL unit
//     CCX_HEVC_NAL_RSV_VCL_R13 = 13,               // Reserved reference VCL NAL unit
//     CCX_HEVC_NAL_RSV_VCL_R15 = 15,               // Reserved reference VCL NAL unit
    
//     // *** KEYFRAME NAL UNITS (16-21) ***
//     CCX_HEVC_NAL_BLA_W_LP = 16,                  // Broken link access with leading pictures
//     CCX_HEVC_NAL_BLA_W_RADL = 17,                // Broken link access with RADL pictures
//     CCX_HEVC_NAL_BLA_N_LP = 18,                  // Broken link access without leading pictures
//     CCX_HEVC_NAL_IDR_W_RADL = 19,                // Instantaneous decoder refresh with RADL pictures  
//     CCX_HEVC_NAL_IDR_N_LP = 20,                  // Instantaneous decoder refresh without leading pictures
//     CCX_HEVC_NAL_CRA_NUT = 21,                   // Clean random access
    
//     // *** RESERVED NAL UNITS (22-31) ***
//     CCX_HEVC_NAL_RSV_IRAP_VCL22 = 22,            // Reserved IRAP VCL NAL unit
//     CCX_HEVC_NAL_RSV_IRAP_VCL23 = 23,            // Reserved IRAP VCL NAL unit
//     // ... (24-31 reserved)
    
//     // *** PARAMETER SET NAL UNITS (32-34) *** 
//     CCX_HEVC_NAL_VPS = 32,                       // ← Video Parameter Set (HEVC-specific)
//     CCX_HEVC_NAL_SPS = 33,                       // ← Sequence Parameter Set  
//     CCX_HEVC_NAL_PPS = 34,                       // ← Picture Parameter Set
    
//     // *** OTHER NON-VCL NAL UNITS ***
//     CCX_HEVC_NAL_AUD = 35,                       // Access Unit Delimiter
//     CCX_HEVC_NAL_EOS = 36,                       // End of Sequence
//     CCX_HEVC_NAL_EOB = 37,                       // End of Bitstream  
//     CCX_HEVC_NAL_FD = 38,                        // Filler Data
    
//     // *** CRITICAL: SEI NAL UNITS (CONTAIN CLOSED CAPTIONS) ***
//     CCX_HEVC_NAL_SEI_PREFIX = 39,                // ← SEI Prefix - CONTAINS CLOSED CAPTIONS!
//     CCX_HEVC_NAL_SEI_SUFFIX = 40,                // ← SEI Suffix - CONTAINS CLOSED CAPTIONS!
    
//     // *** RESERVED NAL UNITS (41-47) ***
//     CCX_HEVC_NAL_RSV_NVCL41 = 41,                // Reserved non-VCL NAL unit
//     CCX_HEVC_NAL_RSV_NVCL42 = 42,                // Reserved non-VCL NAL unit  
//     CCX_HEVC_NAL_RSV_NVCL43 = 43,                // Reserved non-VCL NAL unit
//     CCX_HEVC_NAL_RSV_NVCL44 = 44,                // Reserved non-VCL NAL unit
//     CCX_HEVC_NAL_RSV_NVCL45 = 45,                // Reserved non-VCL NAL unit
//     CCX_HEVC_NAL_RSV_NVCL46 = 46,                // Reserved non-VCL NAL unit
//     CCX_HEVC_NAL_RSV_NVCL47 = 47,                // Reserved non-VCL NAL unit
    
//     // *** UNSPECIFIED NAL UNITS (48-63) ***
//     CCX_HEVC_NAL_UNSPEC48 = 48,                  // Unspecified NAL unit
//     CCX_HEVC_NAL_UNSPEC49 = 49,                  // Unspecified NAL unit
//     // ... (50-63 unspecified)
//     CCX_HEVC_NAL_UNSPEC63 = 63                   // Unspecified NAL unit
// };


// ========== *** MPEG TRANSPORT STREAM TYPES *** ==========
// PURPOSE: Identify video/audio codec types in MPEG Transport Streams
// NEEDED FOR: Video codec detection in PMT parsing - THIS IS WHERE H.264 IS DEFINED!
// SOURCE: ISO/IEC 13818-1 Table 2-29 (MPEG-TS standard)
enum ccx_stream_type
{
    CCX_STREAM_TYPE_UNKNOWNSTREAM = 0,          // Unknown or unrecognized stream

    // *** STANDARD MPEG STREAM TYPES ***
    // These constants are defined by MPEG-TS standard
    // Reference: https://exiftool.org/TagNames/M2TS.html
    
    // *** VIDEO CODEC TYPES ***
    CCX_STREAM_TYPE_VIDEO_MPEG1            = 0x01,  // MPEG-1 Video (old standard)
    CCX_STREAM_TYPE_VIDEO_MPEG2            = 0x02,  // MPEG-2 Video (DVD, broadcast TV)
    CCX_STREAM_TYPE_VIDEO_MPEG4            = 0x10,  // MPEG-4 Part 2 Video (DivX, Xvid)
    CCX_STREAM_TYPE_VIDEO_H264             = 0x1b,  // ← H.264/AVC Video (HD TV, Blu-ray)
    CCX_STREAM_TYPE_VIDEO_HEVC             = 0x24,  // HEVC/H.265 Video (4K, modern streaming)
    
    // *** AUDIO CODEC TYPES ***
    CCX_STREAM_TYPE_AUDIO_MPEG1            = 0x03,  // MPEG-1 Audio (MP3)
    CCX_STREAM_TYPE_AUDIO_MPEG2            = 0x04,  // MPEG-2 Audio
    CCX_STREAM_TYPE_AUDIO_AAC              = 0x0f,  // AAC Audio (modern audio)
    CCX_STREAM_TYPE_AUDIO_AC3              = 0x81,  // AC-3/Dolby Digital Audio
    CCX_STREAM_TYPE_AUDIO_HDMV_DTS         = 0x82,  // DTS Audio (Blu-ray)
    CCX_STREAM_TYPE_AUDIO_DTS              = 0x8a,  // DTS Audio (general)
    
    // *** DATA AND SUBTITLE STREAMS ***
    CCX_STREAM_TYPE_PRIVATE_TABLE_MPEG2    = 0x05,  // Private table sections
    CCX_STREAM_TYPE_PRIVATE_MPEG2          = 0x06,  // Private data (DVB subtitles, teletext)
    CCX_STREAM_TYPE_MHEG_PACKETS           = 0x07,  // MHEG packets (interactive TV)
    CCX_STREAM_TYPE_MPEG2_ANNEX_A_DSM_CC   = 0x08,  // DSM-CC data carousel
    CCX_STREAM_TYPE_ITU_T_H222_1           = 0x09,  // ITU-T H.222.1
    CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_A = 0x0A,  // Multi-protocol encapsulation
    CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_B = 0x0B,  // DSM-CC U-N messages
    CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_C = 0x0C,  // Stream descriptors
    CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_D = 0x0D,  // Sections with CRC
    
    // *** PRIVATE/USER-DEFINED STREAMS ***
    CCX_STREAM_TYPE_PRIVATE_USER_MPEG2     = 0x80,  // Private user streams (ATSC captions)
};

// ========== MPEG DESCRIPTOR TYPES ==========
// PURPOSE: Identify descriptor types in MPEG stream descriptors
// NEEDED FOR: Parsing PMT descriptors to find subtitle and language information
// SOURCE: ETSI EN 300 468 standard (European broadcasting standard)
enum ccx_mpeg_descriptor
{
    // *** STANDARD MPEG DESCRIPTORS ***
    CCX_MPEG_DSC_REGISTRATION            = 0x05,  // Registration descriptor
    CCX_MPEG_DSC_DATA_STREAM_ALIGNMENT   = 0x06,  // Data stream alignment
    CCX_MPEG_DSC_ISO639_LANGUAGE         = 0x0A,  // Language descriptor (3-letter codes)
    
    // *** TELETEXT AND SUBTITLE DESCRIPTORS ***
    CCX_MPEG_DSC_VBI_DATA_DESCRIPTOR     = 0x45,  // VBI data (teletext, WSS, etc.)
    CCX_MPEG_DSC_VBI_TELETEXT_DESCRIPTOR = 0x46,  // VBI teletext specifically
    CCX_MPEG_DSC_TELETEXT_DESCRIPTOR     = 0x56,  // DVB teletext descriptor
    CCX_MPEG_DSC_DVB_SUBTITLE            = 0x59,  // DVB bitmap subtitles
    
    // *** USER-DEFINED DESCRIPTORS ***
    CCX_MPEG_DSC_CAPTION_SERVICE         = 0x86,  // ATSC caption service descriptor
    CCX_MPEG_DESC_DATA_COMP              = 0xfd   // ISDB data component descriptor
};

// ========== MESSAGE OUTPUT DESTINATIONS ==========
// PURPOSE: Control where messages are sent (quiet, stdout, stderr)
// NEEDED FOR: Message routing and verbosity control
enum
{
    CCX_MESSAGES_QUIET  = 0,  // No output
    CCX_MESSAGES_STDOUT = 1,  // Send to standard output
    CCX_MESSAGES_STDERR = 2   // Send to standard error
};

// ========== DATA SOURCE TYPES ==========
// PURPOSE: Identify where input data comes from
// NEEDED FOR: Configuring input handling (file vs network vs stdin)
enum ccx_datasource
{
    CCX_DS_FILE    = 0,  // Regular file input
    CCX_DS_STDIN   = 1,  // Standard input (pipe)
    CCX_DS_NETWORK = 2,  // Network stream (UDP)
    CCX_DS_TCP     = 3   // TCP network stream
};

// ========== OUTPUT FORMAT TYPES ==========
// PURPOSE: Define all supported subtitle output formats
// NEEDED FOR: Format conversion and output generation
enum ccx_output_format
{
    CCX_OF_RAW        = 0,   // Raw closed caption data
    CCX_OF_SRT        = 1,   // SubRip (.srt) format - most common
    CCX_OF_SAMI       = 2,   // SAMI (.smi) format - Microsoft
    CCX_OF_TRANSCRIPT = 3,   // Plain text transcript
    CCX_OF_RCWT       = 4,   // Raw Captions With Time (CCExtractor native)
    CCX_OF_NULL       = 5,   // No output (processing only)
    CCX_OF_SMPTETT    = 6,   // SMPTE-TT XML format
    CCX_OF_SPUPNG     = 7,   // DVD subtitle images (PNG)
    CCX_OF_DVDRAW     = 8,   // Raw DVD subtitle data
    CCX_OF_WEBVTT     = 9,   // WebVTT format (web standards)
    CCX_OF_SIMPLE_XML = 10,  // Simple XML format
    CCX_OF_G608       = 11,  // G608 format
    CCX_OF_CURL       = 12,  // CURL network output
    CCX_OF_SSA        = 13,  // SubStation Alpha format
    CCX_OF_MCC        = 14,  // MacCaption format (broadcast)
    CCX_OF_SCC        = 15,  // Scenarist Closed Caption format
    CCX_OF_CCD        = 16,  // CCD format
};

// ========== OUTPUT DATE/TIME FORMATS ==========
// PURPOSE: Control timestamp display format in subtitles
// NEEDED FOR: Different subtitle formats require different time representations
enum ccx_output_date_format
{
    ODF_NONE     = 0,  // No timestamps
    ODF_HHMMSS   = 1,  // Hours:Minutes:Seconds
    ODF_SECONDS  = 2,  // Total seconds
    ODF_DATE     = 3,  // Full date/time
    ODF_HHMMSSMS = 4   // Hours:Minutes:Seconds,Milliseconds (.srt style)
};

// ========== STREAM MODE TYPES ==========
// PURPOSE: Identify the container/wrapper format of input streams
// NEEDED FOR: Selecting appropriate demuxer and parser
enum ccx_stream_mode_enum
{
    CCX_SM_ELEMENTARY_OR_NOT_FOUND = 0,  // Raw elementary stream or unknown
    CCX_SM_TRANSPORT = 1,                // MPEG Transport Stream (.ts, .m2ts)
    CCX_SM_PROGRAM = 2,                  // MPEG Program Stream (.mpg, .vob)
    CCX_SM_ASF = 3,                      // Advanced Systems Format (.wmv, .asf)
    CCX_SM_MCPOODLESRAW = 4,             // McPoodle's raw format
    CCX_SM_RCWT = 5,                     // Raw Captions With Time
    CCX_SM_MYTH = 6,                     // MythTV recording format
    CCX_SM_MP4  = 7,                     // MP4/MOV container
#ifdef WTV_DEBUG
    CCX_SM_HEX_DUMP = 8,                 // Hexadecimal dump (debug)
#endif
    CCX_SM_WTV = 9,                      // Windows TV format
#ifdef ENABLE_FFMPEG
    CCX_SM_FFMPEG = 10,                  // FFmpeg-handled formats
#endif
    CCX_SM_GXF = 11,                     // General Exchange Format (broadcast)
    CCX_SM_MKV = 12,                     // Matroska Video (.mkv)
    CCX_SM_MXF = 13,                     // Material Exchange Format (broadcast)

    CCX_SM_AUTODETECT = 16               // Auto-detect format
};

// ========== CHARACTER ENCODING TYPES ==========
// PURPOSE: Handle different text encodings in subtitles
// NEEDED FOR: International character support and encoding conversion
enum ccx_encoding_type
{
    CCX_ENC_UNICODE = 0,  // Unicode (UTF-16)
    CCX_ENC_LATIN_1 = 1,  // Latin-1/ISO-8859-1 (Western Europe)
    CCX_ENC_UTF_8   = 2,  // UTF-8 (universal)
    CCX_ENC_ASCII   = 3   // ASCII (7-bit)
};

// ========== BUFFER DATA TYPES ==========
// PURPOSE: Identify the type of data in processing buffers
// NEEDED FOR: Routing data to appropriate decoders
enum ccx_bufferdata_type
{
    CCX_UNKNOWN = 0,           // Unknown data type
    CCX_PES = 1,               // MPEG-2 PES packets (general video)
    CCX_RAW = 2,               // Raw caption data
    CCX_H264 = 3,              // H.264 video data (needs NAL parsing)
    CCX_HAUPPAGE = 4,          // Hauppauge capture card format
    CCX_TELETEXT = 5,          // European teletext subtitles
    CCX_PRIVATE_MPEG2_CC = 6,  // MPEG-2 private stream closed captions
    CCX_DVB_SUBTITLE = 7,      // DVB bitmap subtitles
    CCX_ISDB_SUBTITLE = 8,     // ISDB subtitles (Japanese)
    CCX_RAW_TYPE = 9,          // Raw 3-byte CC data format
    CCX_DVD_SUBTITLE = 10,      // DVD subtitle data
    CCX_HEVC = 11,          // HEVC/H.265 video data (needs HEVC NAL parsing)
};

// ========== VIDEO FRAME TYPES ==========
// PURPOSE: Identify different types of video frames
// NEEDED FOR: Video analysis and timing calculations
enum ccx_frame_type
{
    CCX_FRAME_TYPE_RESET_OR_UNKNOWN = 0,  // Unknown or reset frame
    CCX_FRAME_TYPE_I_FRAME = 1,           // I-frame (keyframe, complete picture)
    CCX_FRAME_TYPE_P_FRAME = 2,           // P-frame (predicted, references previous)
    CCX_FRAME_TYPE_B_FRAME = 3,           // B-frame (bidirectional, references both directions)
    CCX_FRAME_TYPE_D_FRAME = 4            // D-frame (DC-only, low quality)
};

// ========== BOOLEAN TYPE DEFINITION ==========
// PURPOSE: Define a proper boolean type with undefined state
// NEEDED FOR: Three-state logic (true/false/unknown)
typedef enum {
    NO  = 0,        // False
    YES = 1,        // True
    UNDEFINED = 0xff // Unknown/uninitialized state
} bool_t;

// ========== CODEC TYPES ==========
// PURPOSE: Identify which subtitle/caption codec to use
// NEEDED FOR: Decoder selection and codec-specific processing
enum ccx_code_type
{
    CCX_CODEC_ANY      = 0,  // Accept any codec (auto-detect)
    CCX_CODEC_TELETEXT = 1,  // European teletext subtitles
    CCX_CODEC_DVB      = 2,  // DVB bitmap subtitles
    CCX_CODEC_ISDB_CC  = 3,  // ISDB closed captions (Japanese)
    CCX_CODEC_ATSC_CC  = 4,  // ATSC closed captions (North American)
    CCX_CODEC_NONE     = 5   // No codec (skip processing)
};

// ========== CDP (CAPTION DISTRIBUTION PACKET) SECTION TYPES ==========
// PURPOSE: Identify sections within CDP packets for CEA-708 captions
// NEEDED FOR: Parsing SMPTE ST 334 Caption Distribution Packets
// SOURCE: SMPTE ST 334 standard (broadcast caption distribution)
enum cdp_section_type
{
    CDP_SECTION_DATA     = 0x72,  // Caption data section
    CDP_SECTION_SVC_INFO = 0x73,  // Service information section
    CDP_SECTION_FOOTER   = 0x74   // Footer section
};

// ========== TELETEXT VALIDATION MACRO ==========
// PURPOSE: Check if a descriptor tag indicates teletext content
// NEEDED FOR: Teletext stream detection in PMT parsing
/*
 * This Macro checks whether descriptor tag is valid for teletext
 * codec or not.
 *
 * @param desc descriptor tag given for each stream
 * @return 1 if descriptor tag is valid for teletext, 0 otherwise
 */
#define IS_VALID_TELETEXT_DESC(desc) ( ((desc) == CCX_MPEG_DSC_VBI_DATA_DESCRIPTOR )|| \
		( (desc) == CCX_MPEG_DSC_VBI_TELETEXT_DESCRIPTOR ) || \
		( (desc) == CCX_MPEG_DSC_TELETEXT_DESCRIPTOR ) )


// ========== CODEC FEASIBILITY MACRO ==========
// PURPOSE: Determine if a codec should be processed based on user selection
// NEEDED FOR: Codec filtering when user specifies include/exclude preferences
/*
 * This macro determines whether to process a specific subtitle codec type
 *
 * @param u_sel   codec selected by user to be processed (CCX_CODEC_ANY = process all)
 * @param u_nsel  codec selected by user NOT to be processed 
 * @param f_sel   codec being tested for feasibility
 * 
 * @return 1 if codec should be processed, 0 if it should be skipped
 *
 * Logic: Process if (user wants any codec AND this codec is not excluded) 
 *        OR user specifically requested this codec
 */
#define IS_FEASIBLE(u_sel,u_nsel,f_sel) ( ( (u_sel) == CCX_CODEC_ANY && (u_nsel) != (f_sel) ) || (u_sel) == (f_sel) )

// ========== TELETEXT PROCESSING STATES ==========
// PURPOSE: Track teletext processing state machine
// NEEDED FOR: Teletext auto-detection and processing control
#define CCX_TXT_FORBIDDEN           0 // Ignore teletext packets (user disabled)
#define CCX_TXT_AUTO_NOT_YET_FOUND  1 // Auto-detection mode, not found yet
#define CCX_TXT_IN_USE              2 // Teletext active (auto-detected or forced)

// ========== LANGUAGE SUPPORT ==========
// PURPOSE: Support international language detection and processing
// NEEDED FOR: Multi-language subtitle handling
#define NB_LANGUAGE 100                    // Maximum number of supported languages
extern const char *language[NB_LANGUAGE]; // Language name array

// ========== CREDITS DETECTION DEFAULTS ==========
// PURPOSE: Default timing values for automatic credits detection
// NEEDED FOR: Identifying opening and closing credits in TV shows/movies
#define DEF_VAL_STARTCREDITSNOTBEFORE   "0"      // Don't look for opening credits before this time
#define DEF_VAL_STARTCREDITSNOTAFTER    "5:00"   // Don't look for opening credits after 5 minutes
#define DEF_VAL_STARTCREDITSFORATLEAST  "2"      // Opening credits must last at least 2 seconds
#define DEF_VAL_STARTCREDITSFORATMOST   "5"      // Opening credits must not last more than 5 seconds
#define DEF_VAL_ENDCREDITSFORATLEAST    "2"      // Closing credits must last at least 2 seconds
#define DEF_VAL_ENDCREDITSFORATMOST     "5"      // Closing credits must not last more than 5 seconds

#endif // CCX_CONSTANTS_H
