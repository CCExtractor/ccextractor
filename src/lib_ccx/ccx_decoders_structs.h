#ifndef CCX_DECODERS_STRUCTS_H
#define CCX_DECODERS_STRUCTS_H

#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_common_timing.h"

// Define max width in characters/columns on the screen
#define CCX_DECODER_608_SCREEN_WIDTH  32

/* flag raised when end of display marker arrives in Dvb Subtitle */
#define SUB_EOD_MARKER (1 << 0 )
struct cc_bitmap
{
	int x;
	int y;
	int w;
	int h;
	int nb_colors;
	unsigned char *data[2];
	int linesize[2];
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

/**
* This structure have fields which need to be ignored according to format,
* for example if format is SFORMAT_XDS then all fields other then
* xds related (xds_str, xds_len and  cur_xds_packet_class) should be
* ignored and not to be derefrenced.
*
* TODO use union inside struct for each kind of fields
*/
typedef struct eia608_screen // A CC buffer
{
	/** format of data inside this structure */
	enum ccx_eia608_format format;
	unsigned char characters[15][33];
	unsigned char colors[15][33];
	unsigned char fonts[15][33]; // Extra char at the end for a 0
	int row_used[15]; // Any data in row?
	int empty; // Buffer completely empty?
	/** start time of this CC buffer */
	LLONG start_time;
	/** end time of this CC buffer */
	LLONG end_time;
	enum cc_modes mode;
	int channel; // Currently selected channel
	int my_field; // Used for sanity checks
	/** XDS string */
	char *xds_str;
	/** length of XDS string */
	size_t xds_len;
	/** Class of XDS string */
	int cur_xds_packet_class;
} eia608_screen;

struct ccx_decoders_common_settings_t
{
	LLONG subs_delay; // ms to delay (or advance) subs
	enum ccx_output_format output_format; // What kind of output format should be used?
	int fix_padding; // Replace 0000 with 8080 in HDTV (needed for some cards)
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	void *wbout1;
	int cc_to_stdout;
	int extract; // Extract 1st, 2nd or both fields
	int fullbin; // Disable pruning of padding cc blocks
	struct ccx_decoder_608_settings *settings_608; //  Contains the settings for the 608 decoder.
	int cc_channel; // Channel we want to dump in srt mode
	int trim_subs; // Remove spaces at sides?
	enum ccx_encoding_type encoding;
	unsigned send_to_srv;
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

	int fix_padding; // Replace 0000 with 8080 in HDTV (needed for some cards)
	enum ccx_output_format write_format; // 0=Raw, 1=srt, 2=SMI
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	void *wbout1;
	void *wbout2;
	LLONG subs_delay; // ms to delay (or advance) subs
	int extract; // Extract 1st, 2nd or both fields
	int fullbin; // Disable pruning of padding cc blocks

	int (*writedata)(const unsigned char *data, int length, void *private_data, struct cc_subtitle *sub);
};

#endif
