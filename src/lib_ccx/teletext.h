/*!
(c) 2011-2013 Forers, s. r. o.: telxcc
*/

#ifndef teletext_h_included
#define teletext_h_included

//#include <inttypes.h>

#define MAX_TLT_PAGES 1000

typedef struct {
	uint64_t show_timestamp; // show at timestamp (in ms)
	uint64_t hide_timestamp; // hide at timestamp (in ms)
	uint16_t text[25][40]; // 25 lines x 40 cols (1 screen/page) of wide chars
	uint8_t g2_char_present[25][40]; // 0- Supplementary G2 character set NOT used at this position 1-Supplementary G2 character set used at this position
	uint8_t tainted; // 1 = text variable contains any data
} teletext_page_t;

// application states -- flags for notices that should be printed only once
struct s_states {
	uint8_t programme_info_processed;
	uint8_t pts_initialized;
};

typedef enum
{
	TRANSMISSION_MODE_PARALLEL = 0,
	TRANSMISSION_MODE_SERIAL = 1
} transmission_mode_t;

typedef enum
{
	DATA_UNIT_EBU_TELETEXT_NONSUBTITLE = 0x02,
	DATA_UNIT_EBU_TELETEXT_SUBTITLE = 0x03,
	DATA_UNIT_EBU_TELETEXT_INVERTED = 0x0c,
	DATA_UNIT_VPS = 0xc3,
	DATA_UNIT_CLOSED_CAPTIONS = 0xc5
} data_unit_t;


struct TeletextCtx
{
	short int seen_sub_page[MAX_TLT_PAGES];
	uint8_t verbose : 1; // should telxcc be verbose?
	uint16_t page; // teletext page containing cc we want to filter
	uint16_t tid; // 13-bit packet ID for teletext stream
	double offset; // time offset in seconds
	uint8_t bom : 1; // print UTF-8 BOM characters at the beginning of output
	uint8_t nonempty : 1; // produce at least one (dummy) frame
						  // uint8_t se_mode : 1; // search engine compatible mode => Uses CCExtractor's write_format
						  // uint64_t utc_refvalue; // UTC referential value => Moved to ccx_decoders_common, so can be used for other decoders (608/xds) too
	uint16_t user_page; // Page selected by user, which MIGHT be different to 'page' depending on autodetection stuff
	int levdistmincnt, levdistmaxpct; // Means 2 fails or less is "the same", 10% or less is also "the same"
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	enum ccx_output_format write_format; // 0=Raw, 1=srt, 2=SMI
	int gui_mode_reports; // If 1, output in stderr progress updates so the GUI can grab them
	enum ccx_output_date_format date_format;
	int noautotimeref; // Do NOT set time automatically?
	unsigned send_to_srv;
	char millis_separator;
	uint32_t global_timestamp;

	// Current and previous page buffers. This is the output written to file when
	// the time comes.
	teletext_page_t page_buffer;
	char *page_buffer_prev;
	char *page_buffer_cur;
	unsigned page_buffer_cur_size;
	unsigned page_buffer_cur_used;
	unsigned page_buffer_prev_size;
	unsigned page_buffer_prev_used;
	// Current and previous page compare strings. This is plain text (no colors,
	// tags, etc) in UCS2 (fixed length), so we can compare easily.
	uint64_t *ucs2_buffer_prev;
	uint64_t *ucs2_buffer_cur;
	unsigned ucs2_buffer_cur_size;
	unsigned ucs2_buffer_cur_used;
	unsigned ucs2_buffer_prev_size;
	unsigned ucs2_buffer_prev_used;
	// Buffer timestamp
	uint64_t prev_hide_timestamp;
	uint64_t prev_show_timestamp;
	// subtitle type pages bitmap, 2048 bits = 2048 possible pages in teletext (excl. subpages)
	uint8_t cc_map[256];
	// last timestamp computed
	uint64_t last_timestamp;
	struct s_states states;
	// FYI, packet counter
	uint32_t tlt_packet_counter;
	// teletext transmission mode
	transmission_mode_t transmission_mode;
	// flag indicating if incoming data should be processed or ignored
	uint8_t receiving_data;

	uint8_t using_pts;
	int64_t delta;
	uint32_t t0;

	int sentence_cap;//Set to 1 if -sc is passed
	int new_sentence;
	int splitbysentence;
};

int tlt_print_seen_pages(struct lib_cc_decode *dec_ctx);
void telxcc_dump_prev_page(struct TeletextCtx *ctx, struct cc_subtitle *sub);
void set_tlt_delta(struct lib_cc_decode *dec_ctx, uint64_t pts);
#endif
