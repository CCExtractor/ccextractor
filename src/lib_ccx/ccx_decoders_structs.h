#ifndef CCX_DECODERS_STRUCTS_H
#define CCX_DECODERS_STRUCTS_H

#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_common_timing.h"
#include "ccx_common_structs.h"
#include "list.h"
#include "ccx_decoders_708.h"
// define max width in characters/columns on the screen
#define CCX_DECODER_608_SCREEN_WIDTH  32
#define MAXBFRAMES 50
#define SORTBUF (2*MAXBFRAMES+1)


// flag raised when end of display marker arrives in Dvb Subtitle
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
	// fake modes to emulate stuff
	MODE_FAKE_ROLLUP_1 = 100
};

/**
* This structure have fields which need to be ignored according to format,
* for example if format is SFORMAT_XDS then all fields other then
* xds related (xds_str, xds_len and  cur_xds_packet_class) should be
* ignored and not to be dereferenced.
*
* TODO use union inside struct for each kind of fields
*/
typedef struct eia608_screen // A CC buffer
{
	/** format of data inside this structure */
	enum ccx_eia608_format format;
	unsigned char characters[15][33];
	unsigned char colors[15][33];
	// extra char at the end for a 0
	unsigned char fonts[15][33];
	// any data in row?
	int row_used[15];
	// buffer completely empty?
	int empty;
	// start time of this CC buffer
	LLONG start_time;
	// end time of this CC buffer
	LLONG end_time;
	enum cc_modes mode;
	// currently selected channel
	int channel;
	// used for sanity checks
	int my_field;
	// XDS string
	char *xds_str;
	// length of XDS string
	size_t xds_len;
	// Class of XDS string
	int cur_xds_packet_class;
} eia608_screen;

struct ccx_decoders_common_settings_t
{
	// ms to delay (or advance) subs
	LLONG subs_delay;
	// what kind of output format should be used?
	enum ccx_output_format output_format;
	// replace 0000 with 8080 in HDTV (needed for some cards)
	int fix_padding;
	// segment we actually process
	struct ccx_boundary_time extraction_start, extraction_end;
	int cc_to_stdout;
	// extract 1st, 2nd or both fields
	int extract;
	// disable pruning of padding cc blocks
	int fullbin;
	int no_rollup;
	int noscte20;
	// contains the settings for the 608 decoder
	struct ccx_decoder_608_settings *settings_608;
	// same for cea 708 captions decoder (dtvcc)
	ccx_decoder_dtvcc_settings *settings_dtvcc;
	// channel we want to dump in srt mode
	int cc_channel;
	unsigned send_to_srv;
	// if 1, use PID=1003, process specially and so on
	unsigned int hauppauge_mode;
	int program_number;
	enum ccx_code_type codec;
	int xds_write_to_file;
	void *private_data;
};

struct lib_cc_decode
{
	int cc_stats[4];
	int saw_caption_block;
	// if 1, we have enough lines, time, etc
	int processed_enough;

	/**
	 * 608 contexts - note that this shouldn't be global, they should be
	 * per program
	 */
	void *context_cc608_field_1;
	void *context_cc608_field_2;

	// if 1, write one line at a time
	int no_rollup;
	int noscte20;
	// replace 0000 with 8080 in HDTV (needed for some cards)
	int fix_padding;
	// 0 = Raw, 1 = srt, 2 = SMI
	enum ccx_output_format write_format;
	// segment we actually process
	struct ccx_boundary_time extraction_start, extraction_end;
	// ms to delay (or advance) subs
	LLONG subs_delay;
	// extract 1st, 2nd or both fields
	int extract;
	// disable pruning of padding cc blocks
	int fullbin;
	struct cc_subtitle dec_sub;
	enum ccx_bufferdata_type in_bufferdatatype;
	// if 1, use PID=1003, process specially and so on
	unsigned int hauppauge_mode;

	int frames_since_last_gop;
	// GOP-based timing
	int saw_gop_header;
	// Time info for timed-transcript
	// (maximum) length of a group of pictures
	int max_gop_length;
	// Length of the previous group of pictures
	int last_gop_length;
	unsigned total_pulldownfields;
	unsigned total_pulldownframes;
	int program_number;
	struct list_head list;
	struct ccx_common_timing_ctx *timing;
	enum ccx_code_type codec;
	// set to true if data is buffered
	int has_ccdata_buffered;
	int is_alloc;

	struct avc_ctx *avc_ctx;
	void *private_data;

	/* General video information */
	unsigned int current_hor_size;
	unsigned int current_vert_size;
	unsigned int current_aspect_ratio;
	// assume standard fps, 29.97
	unsigned int current_frame_rate;

	// Required in es_function.c
	int no_bitstream_error;
	int saw_seqgoppic;
	int in_pic_data;

	unsigned int current_progressive_sequence;
	unsigned int current_pulldownfields ;

	int temporal_reference;
	enum ccx_frame_type picture_coding_type;
	unsigned picture_structure;
	unsigned repeat_first_field;
	unsigned progressive_frame;
	unsigned pulldownfields;
	// Required in es_function.c and es_userdata.c
	unsigned top_field_first; // needs to be global

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

	ccx_dtvcc_ctx *dtvcc;
	int current_field;

	// analyse/use the picture information
	int maxtref; // use to remember the temporal reference number

	int cc_data_count[SORTBUF];
	// store fts;
	LLONG cc_fts[SORTBUF];
	// store HD CC packets
	unsigned char cc_data_pkts[SORTBUF][10*31*3+1]; // *10, because MP4 seems to have different limits

	// the sequence number of the current anchor frame.  All currently read
	// B-Frames belong to this I- or P-frame.
	int anchor_seq_number;
	struct ccx_decoders_xds_context *xds_ctx;
	struct ccx_decoder_vbi_ctx *vbi_decoder;

	int (*writedata)(const unsigned char *data,
			 int length,
			 void *private_data,
			 struct cc_subtitle *sub);
};

#endif
