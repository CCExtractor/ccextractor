#ifndef CCX_DECODERS_STRUCTS_H
#define CCX_DECODERS_STRUCTS_H

#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_common_timing.h"
#include "ccx_common_structs.h"
#include "list.h"
#include "ccx_decoders_708.h"
// Define max width in characters/columns on the screen
#define CCX_DECODER_608_SCREEN_ROWS 15
#define CCX_DECODER_608_SCREEN_WIDTH 32
#define MAXBFRAMES 50
#define SORTBUF (2 * MAXBFRAMES + 1)

/* flag raised when end of display marker arrives in Dvb Subtitle */
#define SUB_EOD_MARKER (1 << 0)
struct cc_bitmap
{
	int x;
	int y;
	int w;
	int h;
	int nb_colors;
	unsigned char *data0;
	unsigned char *data1;
	int linesize0;
	int linesize1;
#ifdef ENABLE_OCR
	char *ocr_text;
#endif
};

enum ccx_eia608_format
{
	SFORMAT_CC_SCREEN,
	SFORMAT_CC_LINE,
	SFORMAT_XDS
};

enum cc_modes
{
	MODE_POPON = 0,
	MODE_ROLLUP_2 = 1,
	MODE_ROLLUP_3 = 2,
	MODE_ROLLUP_4 = 3,
	MODE_TEXT = 4,
	MODE_PAINTON = 5,
	// Fake modes to emulate stuff
	MODE_FAKE_ROLLUP_1 = 100
};

enum font_bits
{
	FONT_REGULAR = 0,
	FONT_ITALICS = 1,
	FONT_UNDERLINED = 2,
	FONT_UNDERLINED_ITALICS = 3
};

enum ccx_decoder_608_color_code
{
	COL_WHITE = 0,
	COL_GREEN = 1,
	COL_BLUE = 2,
	COL_CYAN = 3,
	COL_RED = 4,
	COL_YELLOW = 5,
	COL_MAGENTA = 6,
	COL_USERDEFINED = 7,
	COL_BLACK = 8,
	COL_TRANSPARENT = 9,

	// Must keep at end
	COL_MAX
};

/**
 * This structure have fields which need to be ignored according to format,
 * for example if format is SFORMAT_XDS then all fields other then
 * xds related (xds_str, xds_len and  cur_xds_packet_class) should be
 * ignored and not to be dereferenced.
 *
 * TODO use union inside struct for each kind of fields
 */
struct eia608_screen // A CC buffer
{
	/** format of data inside this structure */
	enum ccx_eia608_format format;
	unsigned char characters[CCX_DECODER_608_SCREEN_ROWS][CCX_DECODER_608_SCREEN_WIDTH + 1];
	enum ccx_decoder_608_color_code colors[CCX_DECODER_608_SCREEN_ROWS][CCX_DECODER_608_SCREEN_WIDTH + 1];
	enum font_bits fonts[CCX_DECODER_608_SCREEN_ROWS][CCX_DECODER_608_SCREEN_WIDTH + 1]; // Extra char at the end for a 0
	int row_used[CCX_DECODER_608_SCREEN_ROWS];					     // Any data in row?
	int empty;									     // Buffer completely empty?
	/** start time of this CC buffer */
	LLONG start_time;
	/** end time of this CC buffer */
	LLONG end_time;
	enum cc_modes mode;
	int channel;  // Currently selected channel
	int my_field; // Used for sanity checks
	/** XDS string */
	char *xds_str;
	/** length of XDS string */
	size_t xds_len;
	/** Class of XDS string */
	int cur_xds_packet_class;
};

struct ccx_decoders_common_settings_t
{
	LLONG subs_delay;					   // ms to delay (or advance) subs
	enum ccx_output_format output_format;			   // What kind of output format should be used?
	int fix_padding;					   // Replace 0000 with 8080 in HDTV (needed for some cards)
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	int cc_to_stdout;
	int extract; // Extract 1st, 2nd or both fields
	int fullbin; // Disable pruning of padding cc blocks
	int no_rollup;
	int noscte20;
	struct ccx_decoder_608_settings *settings_608; // Contains the settings for the 608 decoder.
	ccx_decoder_dtvcc_settings *settings_dtvcc;    // Same for cea 708 captions decoder (dtvcc)
	int cc_channel;				       // Channel we want to dump in srt mode
	unsigned send_to_srv;
	unsigned int hauppauge_mode; // If 1, use PID=1003, process specially and so on
	int program_number;
	int pid;
	enum ccx_code_type codec;
	int xds_write_to_file;
	void *private_data;
	int ocr_quantmode;
};

struct lib_cc_decode
{
	int cc_stats[4];
	int saw_caption_block;
	int processed_enough; // If 1, we have enough lines, time, etc.

	/* 608 contexts - note that this shouldn't be global, they should be
	per program */
	void *context_cc608_field_1;
	void *context_cc608_field_2;

	int no_rollup; // If 1, write one line at a time
	int noscte20;
	int fix_padding;					   // Replace 0000 with 8080 in HDTV (needed for some cards)
	enum ccx_output_format write_format;			   // 0 = Raw, 1 = srt, 2 = SMI
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	LLONG subs_delay;					   // ms to delay (or advance) subs
	int extract;						   // Extract 1st, 2nd or both fields
	int fullbin;						   // Disable pruning of padding cc blocks
	struct cc_subtitle dec_sub;
	enum ccx_bufferdata_type in_bufferdatatype;
	unsigned int hauppauge_mode; // If 1, use PID=1003, process specially and so on

	int frames_since_last_gop;
	/* GOP-based timing */
	int saw_gop_header;
	/* Time info for timed-transcript */
	int max_gop_length;  // (Maximum) length of a group of pictures
	int last_gop_length; // Length of the previous group of pictures
	unsigned total_pulldownfields;
	unsigned total_pulldownframes;
	int program_number;
	int pid;
	struct list_head list;
	struct ccx_common_timing_ctx *timing;
	enum ccx_code_type codec;
	// Set to true if data is buffered
	int has_ccdata_buffered;
	int is_alloc;

	struct avc_ctx *avc_ctx;
	void *private_data;

	/* General video information */
	unsigned int current_hor_size;
	unsigned int current_vert_size;
	unsigned int current_aspect_ratio;
	unsigned int current_frame_rate; // Assume standard fps, 29.97

	/* Required in es_function.c */
	int no_bitstream_error;
	int saw_seqgoppic;
	int in_pic_data;

	unsigned int current_progressive_sequence;
	unsigned int current_pulldownfields;

	int temporal_reference;
	enum ccx_frame_type picture_coding_type;
	unsigned num_key_frames;
	unsigned picture_structure;
	unsigned repeat_first_field;
	unsigned progressive_frame;
	unsigned pulldownfields;
	/* Required in es_function.c and es_userdata.c */
	unsigned top_field_first; // Needs to be global

	/* Stats. Modified in es_userdata.c*/
	int stat_numuserheaders;
	int stat_dvdccheaders;
	int stat_scte20ccheaders;
	int stat_replay5000headers;
	int stat_replay4000headers;
	int stat_dishheaders;
	int stat_hdtv;
	int stat_divicom;
	int false_pict_header;

	dtvcc_ctx *dtvcc;
	void *dtvcc_rust; // Persistent Rust CEA-708 decoder context
	int current_field;
	// Analyse/use the picture information
	int maxtref; // Use to remember the temporal reference number

	int cc_data_count[SORTBUF];
	// Store fts;
	LLONG cc_fts[SORTBUF];
	// Store HD CC packets
	unsigned char cc_data_pkts[SORTBUF][10 * 31 * 3 + 1]; // *10, because MP4 seems to have different limits

	// The sequence number of the current anchor frame.  All currently read
	// B-Frames belong to this I- or P-frame.
	int anchor_seq_number;
	struct ccx_decoders_xds_context *xds_ctx;
	struct ccx_decoder_vbi_ctx *vbi_decoder;

	int (*writedata)(const unsigned char *data, int length, void *private_data, struct cc_subtitle *sub);

	// dvb subtitle related
	int ocr_quantmode;
	struct lib_cc_decode *prev;
};

#endif
