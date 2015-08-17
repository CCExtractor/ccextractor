#ifndef CCX_CCEXTRACTOR_H
#define CCX_CCEXTRACTOR_H

#define VERSION "0.77"

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
#include "ccx_decoders_608.h"
#include "ccx_decoders_xds.h"
#include "ccx_decoders_708.h"
#include "bitstream.h"

#include "networking.h"
#include "avc_functions.h"

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
	struct dtvcc_report_t *data_from_708;
	unsigned mp4_cc_track_cnt;
};

// Stuff for telcc.c
struct ccx_s_teletext_config
{
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
	enum ccx_encoding_type encoding;
	int nofontcolor;
	char millis_separator;
};
struct lib_ccx_ctx
{

	// Stuff common to both loops
	unsigned char *pesheaderbuf;
	LLONG inputsize;
	LLONG total_inputsize;
	LLONG total_past; // Only in binary concat mode

	int last_reported_progress;


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

	int cc_to_stdout; // If 1, captions go to stdout instead of file


	LLONG subs_delay; // ms to delay (or advance) subs

	int startcredits_displayed;
	int end_credits_displayed;
	LLONG last_displayed_subs_ms; // When did the last subs end?
	LLONG screens_to_process; // How many screenfuls we want?
	char *basefilename; // Input filename without the extension

	const char *extension; // Output extension
	int current_file; // If current_file!=1, we are processing *inputfile[current_file]

	char **inputfile; // List of files to process
	int num_input_files; // How many?

	unsigned teletext_warning_shown; // Did we detect a possible PAL (with teletext subs) and told the user already?

	//struct EIT_buffer eit_buffer;
	struct EIT_buffer epg_buffers[0xfff+1];
	struct EIT_program eit_programs[TS_PMT_MAP_SIZE+1];
	int32_t eit_current_events[TS_PMT_MAP_SIZE+1];
	int16_t ATSC_source_pg_map[0xffff];
	int epg_last_output; 
	int epg_last_live_output; 
	struct file_report freport;

	unsigned int hauppauge_mode; // If 1, use PID=1003, process specially and so on
	int live_stream; /* -1 -> Not a complete file but a live stream, without timeout
                       0 -> A regular file
                      >0 -> Live stream with a timeout of this value in seconds */
	int binary_concat; // Disabled by -ve or --videoedited
	int multiprogram;
	enum ccx_output_format write_format; // 0=Raw, 1=srt, 2=SMI

	struct ccx_demuxer *demux_ctx;
	struct list_head enc_ctx_head;
};

#define buffered_skip(ctx, bytes) if (bytes<= ctx->bytesinbuffer - ctx->filebuffer_pos) { \
    ctx->filebuffer_pos+=bytes; \
    result=bytes; \
} else result=buffered_read_opt (ctx, NULL,bytes);

#define buffered_read(ctx, buffer,bytes) if (bytes<= ctx->bytesinbuffer - ctx->filebuffer_pos) { \
    if (buffer!=NULL) memcpy (buffer, ctx->filebuffer + ctx->filebuffer_pos, bytes); \
    ctx->filebuffer_pos+=bytes; \
    result=bytes; \
} else { result=buffered_read_opt (ctx, buffer,bytes); if (ccx_options.gui_mode_reports && ccx_options.input_source==CCX_DS_NETWORK) {net_activity_gui++; if (!(net_activity_gui%1000))activity_report_data_read();}}

#define buffered_read_4(buffer) if (4<=bytesinbuffer-filebuffer_pos) { \
    if (buffer) { buffer[0]=filebuffer[filebuffer_pos]; \
    buffer[1]=filebuffer[filebuffer_pos+1]; \
    buffer[2]=filebuffer[filebuffer_pos+2]; \
    buffer[3]=filebuffer[filebuffer_pos+3]; \
    filebuffer_pos+=4; \
    result=4; } \
} else result=buffered_read_opt (buffer,4);

#define buffered_read_byte(ctx, buffer) if (ctx->bytesinbuffer-ctx->filebuffer_pos) { \
    if (buffer) { *buffer=ctx->filebuffer[ctx->filebuffer_pos]; \
    ctx->filebuffer_pos++; \
    result=1; } \
} else result=buffered_read_opt (ctx, buffer,1);

LLONG buffered_read_opt (struct ccx_demuxer *ctx, unsigned char *buffer, unsigned int bytes);

struct lib_ccx_ctx* init_libraries(struct ccx_s_options *opt);
void dinit_libraries( struct lib_ccx_ctx **ctx);

//params.c
int parse_parameters (struct ccx_s_options *opt, int argc, char *argv[]);
void usage (void);
int detect_input_file_overwrite(struct lib_ccx_ctx *ctx, const char *output_filename);
int atoi_hex (char *s);
int stringztoms (const char *s, struct ccx_boundary_time *bt);

// general_loop.c
void position_sanity_check (int in);
int init_file_buffer(struct ccx_demuxer *ctx);
int ps_getmoredata(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata);
int general_getmoredata(struct lib_ccx_ctx *ctx, struct demuxer_data **data);
void raw_loop (struct lib_ccx_ctx *ctx);
LLONG process_raw (struct lib_cc_decode *ctx, struct cc_subtitle *sub, unsigned char *buffer, int len);
void general_loop(struct lib_ccx_ctx *ctx);
void processhex (char *filename);
void rcwt_loop(struct lib_ccx_ctx *ctx);

extern LLONG result;
extern int end_of_file;

// asf_functions.c
int asf_getmoredata(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata);

// wtv_functions.c
int wtv_getmoredata(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata);

// es_functions.c
LLONG process_m2v (struct lib_cc_decode *ctx, unsigned char *data, LLONG length,struct cc_subtitle *sub);

extern unsigned top_field_first;

// es_userdata.c
int user_data(struct lib_cc_decode *ctx, struct bitstream *ustream, int udtype, struct cc_subtitle *sub);

// bitstream.c - see bitstream.h

// file_functions.c
LLONG getfilesize (int in);
LLONG gettotalfilessize (struct lib_ccx_ctx *ctx);
void prepare_for_new_file (struct lib_ccx_ctx *ctx);
void close_input_file (struct lib_ccx_ctx *ctx);
int switch_to_next_file (struct lib_ccx_ctx *ctx, LLONG bytesinbuffer);
void return_to_buffer (struct ccx_demuxer *ctx, unsigned char *buffer, unsigned int bytes);

// sequencing.c
void init_hdcc (struct lib_cc_decode *ctx);
void store_hdcc(struct lib_cc_decode *ctx, unsigned char *cc_data, int cc_count, int sequence_number, LLONG current_fts_now, struct cc_subtitle *sub);
void anchor_hdcc(int seq);
void process_hdcc (struct lib_cc_decode *ctx, struct cc_subtitle *sub);

// params_dump.c
void params_dump(struct lib_ccx_ctx *ctx);
void print_file_report(struct lib_ccx_ctx *ctx);

// output.c
void dinit_write(struct ccx_s_write *wb);
int init_write (struct ccx_s_write *wb,char *filename);
int writeraw (const unsigned char *data, int length, void *private_data, struct cc_subtitle *sub);
void flushbuffer (struct lib_ccx_ctx *ctx, struct ccx_s_write *wb, int closefile);
void writercwtdata (struct lib_cc_decode *ctx, const unsigned char *data, struct cc_subtitle *sub);

// stream_functions.c
int isValidMP4Box(unsigned char *buffer, long position, long *nextBoxLocation, int *boxScore);
void detect_stream_type (struct ccx_demuxer *ctx);
int detect_myth( struct ccx_demuxer *ctx );
int read_video_pes_header (struct ccx_demuxer *ctx, struct demuxer_data *data, unsigned char *nextheader, int *headerlength, int sbuflen);

// ts_functions.c
void init_ts(struct ccx_demuxer *ctx);
int ts_readpacket(struct ccx_demuxer* ctx, struct ts_payload *payload);
long ts_readstream(struct ccx_demuxer *ctx, struct demuxer_data **data);
LLONG ts_getmoredata(struct ccx_demuxer *ctx, struct demuxer_data **data);
int write_section(struct ccx_demuxer *ctx, struct ts_payload *payload, unsigned char*buf, int size,  struct program_info *pinfo);
int parse_PMT (struct ccx_demuxer *ctx, struct ts_payload *payload, unsigned char *buf, int len,  struct program_info *pinfo);
int parse_PAT (struct ccx_demuxer *ctx, struct ts_payload *payload);
void parse_EPG_packet (struct lib_ccx_ctx *ctx);
void EPG_free(struct lib_ccx_ctx *ctx);

// myth.c
void myth_loop(struct lib_ccx_ctx *ctx);

LLONG process_avc (struct lib_cc_decode *ctx, unsigned char *avcbuf, LLONG avcbuflen ,struct cc_subtitle *sub);
// utility.c
void fatal(int exit_code, const char *fmt, ...);
void mprint (const char *fmt, ...);
void sleep_secs (int secs);
void dump (LLONG mask, unsigned char *start, int l, unsigned long abs_start, unsigned clear_high_bit);
bool_t in_array(uint16_t *array, uint16_t length, uint16_t element) ;
int hex2int (char high, char low);
void timestamp_to_srttime(uint64_t timestamp, char *buffer);
void timestamp_to_smptetttime(uint64_t timestamp, char *buffer);
int levenshtein_dist (const uint64_t *s1, const uint64_t *s2, unsigned s1len, unsigned s2len);
void millis_to_date (uint64_t timestamp, char *buffer, enum ccx_output_date_format date_format, char millis_separator);
#ifndef _WIN32
void m_signal(int sig, void (*func)(int));
#endif


void buffered_seek (struct ccx_demuxer *ctx, int offset);
extern void build_parity_table(void);

int tlt_process_pes_packet(struct lib_cc_decode *dec_ctx, uint8_t *buffer, uint16_t size, struct cc_subtitle *sub);
void* telxcc_init(void);
void telxcc_close(void **ctx, struct cc_subtitle *sub);
void tlt_read_rcwt(void *codec, unsigned char *buf, struct cc_subtitle *sub);
void telxcc_configure (void *codec, struct ccx_s_teletext_config *cfg);
void telxcc_update_gt(void *codec, uint32_t global_timestamp);

extern unsigned rollover_bits;

extern int strangeheader;

extern const char *desc[256];


extern long FILEBUFFERSIZE; // Uppercase because it used to be a define
extern unsigned long net_activity_gui;

extern int firstcall;

#define MAXBFRAMES 50
#define SORTBUF (2*MAXBFRAMES+1)
extern int cc_data_count[SORTBUF];
extern unsigned char cc_data_pkts[SORTBUF][10*31*3+1];

// From ts_functions
//extern struct ts_payload payload;
extern unsigned char tspacket[188];
extern unsigned char *last_pat_payload;
extern unsigned last_pat_length;


#define HAUPPAGE_CCPID	1003 // PID for CC's in some Hauppauge recordings

extern unsigned teletext_mode;

#define MAX_TLT_PAGES 1000

extern struct ccx_s_teletext_config tlt_config;

int is_decoder_processed_enough(struct lib_ccx_ctx *ctx);
struct lib_cc_decode *update_decoder_list_cinfo(struct lib_ccx_ctx *ctx, struct cap_info* cinfo);
struct lib_cc_decode *update_decoder_list(struct lib_ccx_ctx *ctx);

struct encoder_ctx *update_encoder_list_cinfo(struct lib_ccx_ctx *ctx, struct cap_info* cinfo);
struct encoder_ctx * update_encoder_list(struct lib_ccx_ctx *ctx);
#endif
