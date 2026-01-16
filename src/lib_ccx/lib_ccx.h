#ifndef CCX_CCEXTRACTOR_H
#define CCX_CCEXTRACTOR_H

#define VERSION "0.96.5"

// Load common includes and constants for library usage
#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_common_common.h"
#include "ccx_common_char_encoding.h"
#include "ccx_common_structs.h"
#include "ccx_common_timing.h"
#include "ccx_common_option.h"

#include "ccx_demuxer.h"
#include "ccx_encoders_common.h"
#include "ccx_decoders_common.h"
#include "cc_bitstream.h"

#include "networking.h"
#include "avc_functions.h"
#include "teletext.h"

#ifdef WITH_LIBCURL
#include <curl/curl.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

// #include "ccx_decoders_708.h"

/* Report information */
#define SUB_STREAMS_CNT 10

#define TELETEXT_CHUNK_LEN 1 + 8 + 44
struct file_report
{
	unsigned width;
	unsigned height;
	unsigned aspect_ratio;
	unsigned frame_rate;
	struct ccx_decoder_608_report *data_from_608;
	struct ccx_decoder_dtvcc_report *data_from_708;
	unsigned mp4_cc_track_cnt;
};

// Stuff for telxcc.c
#define MAX_TLT_PAGES_EXTRACT 8 // Maximum number of teletext pages to extract simultaneously

struct ccx_s_teletext_config
{
	uint8_t verbose : 1;  // should telxcc be verbose?
	uint16_t page;	      // teletext page containing cc we want to filter (legacy, first page)
	uint16_t tid;	      // 13-bit packet ID for teletext stream
	double offset;	      // time offset in seconds
	uint8_t bom : 1;      // print UTF-8 BOM characters at the beginning of output
	uint8_t nonempty : 1; // produce at least one (dummy) frame
	// uint8_t se_mode : 1;                                    // search engine compatible mode => Uses CCExtractor's write_format
	// uint64_t utc_refvalue;                                  // UTC referential value => Moved to ccx_decoders_common, so can be used for other decoders (608/xds) too
	uint16_t user_page; // Page selected by user (legacy, first page)
	// Multi-page teletext extraction (issue #665)
	uint16_t user_pages[MAX_TLT_PAGES_EXTRACT];		   // Pages selected by user for extraction
	int num_user_pages;					   // Number of pages to extract (0 = auto-detect single page)
	int extract_all_pages;					   // If 1, extract all detected subtitle pages
	int dolevdist;						   // 0=Don't attempt to correct errors
	int levdistmincnt, levdistmaxpct;			   // Means 2 fails or less is "the same", 10% or less is also "the same"
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	enum ccx_output_format write_format;			   // 0=Raw, 1=srt, 2=SMI
	int gui_mode_reports;					   // If 1, output in stderr progress updates so the GUI can grab them
	enum ccx_output_date_format date_format;
	int noautotimeref; // Do NOT set time automatically?
	unsigned send_to_srv;
	enum ccx_encoding_type encoding;
	int nofontcolor;
	int nohtmlescape;
	char millis_separator;
	int latrusmap;
	int forceg0latin; // Force G0 Latin charset, ignore Cyrillic designations (issue #1395)
};

struct ccx_s_mp4Cfg
{
	unsigned int mp4vidtrack : 1;
};

#define MAX_SUBTITLE_PIPELINES 64

/**
 * ccx_subtitle_pipeline - Encapsulates all components for a single subtitle output stream
 */
struct ccx_subtitle_pipeline
{
	int pid;
	int stream_type;
	char lang[4];
	char filename[1024]; // Using fixed size instead of PATH_MAX to avoid header issues
	struct ccx_s_write *writer;
	struct encoder_ctx *encoder;
	struct ccx_common_timing_ctx *timing;
	void *decoder;		       // Pointer to decoder context (e.g., ccx_decoders_dvb_context)
	struct lib_cc_decode *dec_ctx; // Full decoder context for DVB state management
	struct cc_subtitle sub;	       // Persistent cc_subtitle for DVB prev tracking
#ifdef ENABLE_OCR
	void *ocr_ctx; // Per-pipeline OCR context for thread safety
#endif
};

struct lib_ccx_ctx
{
	// Stuff common to both loops
	unsigned char *pesheaderbuf;
	LLONG inputsize;
	LLONG total_inputsize;
	LLONG total_past; // Only in binary concat mode

	int last_reported_progress;
	LLONG min_global_timestamp_offset; // Track minimum (global - min) for multi-program TS

	/* Stats */
	int stat_numuserheaders;
	int stat_dvdccheaders;
	int stat_scte20ccheaders;
	int stat_replay5000headers;
	int stat_replay4000headers;
	int stat_dishheaders;
	int stat_hdtv;
	int stat_divicom;
	int false_pict_header;

	// int hex_mode=HEX_NONE; // Are we processing an hex file?
	struct ccx_decoders_common_settings_t *dec_global_setting;
	struct list_head dec_ctx_head;

	int rawmode; // Broadcast or DVD
	// See -d from

	int cc_to_stdout;		// If 1, captions go to stdout instead of file
	int pes_header_to_stdout;	// If 1, PES Header data will be outputted to console
	int dvb_debug_traces_to_stdout; // If 1, DVB subtitle debug traces will be outputted to console
	int ignore_pts_jumps;		// If 1, the program will ignore PTS jumps. Sometimes this parameter is required for DVB subs with > 30s pause time

	LLONG subs_delay; // ms to delay (or advance) subs

	int startcredits_displayed;
	int end_credits_displayed;
	LLONG last_displayed_subs_ms; // When did the last subs end?
	LLONG screens_to_process;     // How many screenfuls we want?
	char *basefilename;	      // Input filename without the extension

	const char *extension; // Output extension
	int current_file;      // If current_file!=1, we are processing *inputfile[current_file]

	char **inputfile;    // List of files to process
	int num_input_files; // How many?

	unsigned teletext_warning_shown; // Did we detect a possible PAL (with teletext subs) and told the user already?

	int epg_inited;
	struct PSI_buffer *epg_buffers;
	struct EIT_program *eit_programs;
	int32_t *eit_current_events;
	int16_t *ATSC_source_pg_map;
	int epg_last_output;
	int epg_last_live_output;
	struct file_report freport;

	unsigned int hauppauge_mode; // If 1, use PID=1003, process specially and so on
	int live_stream;	     /* -1 -> Not a complete file but a live stream, without timeout
				  0 -> A regular file
				 >0 -> Live stream with a timeout of this value in seconds */
	int binary_concat;	     // Disabled by -ve or --videoedited
	int multiprogram;
	enum ccx_output_format write_format;

	struct ccx_demuxer *demux_ctx;
	struct list_head enc_ctx_head;
	struct ccx_s_mp4Cfg mp4_cfg;
	int out_interval;
	int segment_on_key_frames_only;
	int segment_counter;
	LLONG system_start_time;

	// Registration for multi-stream subtitle extraction
	struct ccx_subtitle_pipeline *pipelines[MAX_SUBTITLE_PIPELINES];
	int pipeline_count;
#ifdef _WIN32
	CRITICAL_SECTION pipeline_mutex;
#else
	pthread_mutex_t pipeline_mutex;
#endif
	int pipeline_mutex_initialized;
	void *dec_dvb_default; // Default decoder used in non-split mode
	void *shared_ocr_ctx;  // Shared OCR context to reduce memory usage
};

struct ccx_subtitle_pipeline *get_or_create_pipeline(struct lib_ccx_ctx *ctx, int pid, int stream_type, const char *lang);
void set_pipeline_pts(struct ccx_subtitle_pipeline *pipe, LLONG pts);

struct lib_ccx_ctx *init_libraries(struct ccx_s_options *opt);
void dinit_libraries(struct lib_ccx_ctx **ctx);

extern void ccxr_init_basic_logger();
extern void ccxr_update_logger_target();

// ccextractor.c
void print_end_msg(void);

// params.c
extern int ccxr_parse_parameters(int argc, char *argv[]);
int parse_parameters(struct ccx_s_options *opt, int argc, char *argv[]);
void print_usage(void);
int atoi_hex(char *s);
int stringztoms(const char *s, struct ccx_boundary_time *bt);

// general_loop.c
void position_sanity_check(struct ccx_demuxer *ctx);
int init_file_buffer(struct ccx_demuxer *ctx);
int ps_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata);
int general_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **data);
int raw_loop(struct lib_ccx_ctx *ctx);
size_t process_raw(struct lib_cc_decode *ctx, struct cc_subtitle *sub, unsigned char *buffer, size_t len);

// Rust FFI: McPoodle DVD raw format processing (see src/rust/src/demuxer/dvdraw.rs)
unsigned int ccxr_process_dvdraw(struct lib_cc_decode *ctx, struct cc_subtitle *sub, const unsigned char *buffer, unsigned int len);
int ccxr_is_dvdraw_header(const unsigned char *buffer, unsigned int len);

// Rust FFI: SCC (Scenarist Closed Caption) format processing (see src/rust/src/demuxer/scc.rs)
unsigned int ccxr_process_scc(struct lib_cc_decode *ctx, struct cc_subtitle *sub, const unsigned char *buffer, unsigned int len, int framerate);
int ccxr_is_scc_file(const unsigned char *buffer, unsigned int len);

int general_loop(struct lib_ccx_ctx *ctx);
void process_hex(struct lib_ccx_ctx *ctx, char *filename);
int rcwt_loop(struct lib_ccx_ctx *ctx);

extern int end_of_file;

int ccx_mxf_getmoredata(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata);

// asf_functions.c
int asf_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata);

// wtv_functions.c
int wtv_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata);

// es_functions.c
size_t process_m2v(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, unsigned char *data, size_t length, struct cc_subtitle *sub);

extern unsigned top_field_first;

// es_userdata.c
int user_data(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct bitstream *ustream, int udtype, struct cc_subtitle *sub);

// bitstream.c - see bitstream.h

// file_functions.c
LLONG get_file_size(int in);
LLONG get_total_file_size(struct lib_ccx_ctx *ctx);
void prepare_for_new_file(struct lib_ccx_ctx *ctx);
void close_input_file(struct lib_ccx_ctx *ctx);
int switch_to_next_file(struct lib_ccx_ctx *ctx, LLONG bytesinbuffer);
void return_to_buffer(struct ccx_demuxer *ctx, unsigned char *buffer, unsigned int bytes);

// sequencing.c
void init_hdcc(struct lib_cc_decode *ctx);
void store_hdcc(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, unsigned char *cc_data, int cc_count, int sequence_number, LLONG current_fts_now, struct cc_subtitle *sub);
void anchor_hdcc(struct lib_cc_decode *ctx, int seq);
void process_hdcc(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, struct cc_subtitle *sub);

// params_dump.c
void params_dump(struct lib_ccx_ctx *ctx);
void print_file_report(struct lib_ccx_ctx *ctx);

// output.c
void dinit_write(struct ccx_s_write *wb);
int temporarily_open_output(struct ccx_s_write *wb);
int temporarily_close_output(struct ccx_s_write *wb);
int init_write(struct ccx_s_write *wb, char *filename, int with_semaphore);
int writeraw(const unsigned char *data, int length, void *private_data, struct cc_subtitle *sub);
void flushbuffer(struct lib_ccx_ctx *ctx, struct ccx_s_write *wb, int closefile);
void writercwtdata(struct lib_cc_decode *ctx, const unsigned char *data, struct cc_subtitle *sub);

// stream_functions.c
int isValidMP4Box(unsigned char *buffer, size_t position, size_t *nextBoxLocation, int *boxScore);
void detect_stream_type(struct ccx_demuxer *ctx);
int detect_myth(struct ccx_demuxer *ctx);
int read_video_pes_header(struct ccx_demuxer *ctx, struct demuxer_data *data, unsigned char *nextheader, int *headerlength, int sbuflen);

// ts_functions.c
void init_ts(struct ccx_demuxer *ctx);
int ts_readpacket(struct ccx_demuxer *ctx, struct ts_payload *payload);
int64_t ts_readstream(struct ccx_demuxer *ctx, struct demuxer_data **data);
int ts_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **data);
int write_section(struct ccx_demuxer *ctx, struct ts_payload *payload, unsigned char *buf, int size, struct program_info *pinfo);
void ts_buffer_psi_packet(struct ccx_demuxer *ctx);
int parse_PMT(struct ccx_demuxer *ctx, unsigned char *buf, int len, struct program_info *pinfo);
int parse_PAT(struct ccx_demuxer *ctx);
void parse_EPG_packet(struct lib_ccx_ctx *ctx);
void EPG_free(struct lib_ccx_ctx *ctx);
char *EPG_DVB_decode_string(uint8_t *in, size_t size);
void parse_SDT(struct ccx_demuxer *ctx);

// ts_info.c
int get_video_stream(struct ccx_demuxer *ctx);

// myth.c
int myth_loop(struct lib_ccx_ctx *ctx);

// matroska.c
int matroska_loop(struct lib_ccx_ctx *ctx);

// utility.c
#if defined(__GNUC__)
__attribute__((noreturn))
#elif defined(_MSC_VER)
__declspec(noreturn)
#endif
void
fatal(int exit_code, const char *fmt, ...);
void mprint(const char *fmt, ...);
void sleep_secs(int secs);
void dump(LLONG mask, unsigned char *start, int l, unsigned long abs_start, unsigned clear_high_bit);
bool_t in_array(uint16_t *array, uint16_t length, uint16_t element);
int hex_to_int(char high, char low);
int hex_string_to_int(char *string, int len);
void timestamp_to_srttime(uint64_t timestamp, char *buffer);
int levenshtein_dist(const uint64_t *s1, const uint64_t *s2, unsigned s1len, unsigned s2len);
void millis_to_date(uint64_t timestamp, char *buffer, enum ccx_output_date_format date_format, char millis_separator);
void signal_handler(int sig_type);
struct encoder_ctx *change_filename(struct encoder_ctx *);
#ifndef _WIN32
void m_signal(int sig, void (*func)(int));
#endif

void buffered_seek(struct ccx_demuxer *ctx, int offset);
extern void build_parity_table(void);

int tlt_process_pes_packet(struct lib_cc_decode *dec_ctx, uint8_t *buffer, uint16_t size, struct cc_subtitle *sub, int sentence_cap);
void *telxcc_init(void);
void telxcc_close(void **ctx, struct cc_subtitle *sub);
void tlt_read_rcwt(void *codec, unsigned char *buf, struct cc_subtitle *sub);
void telxcc_configure(void *codec, struct ccx_s_teletext_config *cfg);
void telxcc_update_gt(void *codec, uint32_t global_timestamp);

extern int strangeheader;

extern const char *desc[256];

extern int64_t FILEBUFFERSIZE; // Uppercase because it used to be a define

extern int firstcall;

// From ts_functions
// extern struct ts_payload payload;
extern unsigned char tspacket[188];
extern unsigned char *last_pat_payload;
extern unsigned last_pat_length;
extern volatile int terminate_asap;

#define HAUPPAGE_CCPID 1003 // PID for CC's in some Hauppauge recordings

extern unsigned teletext_mode;

#define MAX_TLT_PAGES 1000

extern struct ccx_s_teletext_config tlt_config;

int is_decoder_processed_enough(struct lib_ccx_ctx *ctx);
struct lib_cc_decode *update_decoder_list_cinfo(struct lib_ccx_ctx *ctx, struct cap_info *cinfo);
struct lib_cc_decode *update_decoder_list(struct lib_ccx_ctx *ctx);

struct encoder_ctx *update_encoder_list_cinfo(struct lib_ccx_ctx *ctx, struct cap_info *cinfo);
struct encoder_ctx *update_encoder_list(struct lib_ccx_ctx *ctx);
struct encoder_ctx *get_encoder_by_pn(struct lib_ccx_ctx *ctx, int pn);
int process_non_multiprogram_general_loop(struct lib_ccx_ctx *ctx,
					  struct demuxer_data **datalist,
					  struct demuxer_data **data_node,
					  struct lib_cc_decode **dec_ctx,
					  struct encoder_ctx **enc_ctx,
					  uint64_t *min_pts,
					  int ret,
					  int *caps);
void segment_output_file(struct lib_ccx_ctx *ctx, struct lib_cc_decode *dec_ctx);
int decode_vbi(struct lib_cc_decode *dec_ctx, uint8_t field, unsigned char *buffer, size_t len, struct cc_subtitle *sub);

#ifndef DISABLE_RUST
// Rust FFI function to set encoder on persistent CEA-708 decoder
void ccxr_dtvcc_set_encoder(void *dtvcc_rust, struct encoder_ctx *encoder);
#endif

#endif
