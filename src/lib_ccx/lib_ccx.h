#ifndef CCX_CCEXTRACTOR_H
#define CCX_CCEXTRACTOR_H

#define VERSION "0.76"

// Load common includes and constants for library usage
#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_common_common.h"
#include "ccx_common_char_encoding.h"
#include "ccx_common_structs.h"
#include "ccx_common_timing.h"
#include "ccx_common_option.h"

#include "ccx_encoders_common.h"
#include "ccx_decoders_608.h"
#include "ccx_decoders_xds.h"
#include "ccx_decoders_708.h"
#include "bitstream.h"

#include "networking.h"

extern int cc_buffer_saved; // Do we have anything in the CC buffer already?
extern int ccblocks_in_avc_total; // Total CC blocks found by the AVC code
extern int ccblocks_in_avc_lost; // CC blocks found by the AVC code lost due to overwrites (should be 0)

#define TS_PMT_MAP_SIZE 128

struct ts_payload
{
	unsigned char *start; // Payload start
	unsigned length;      // Payload length
	unsigned pesstart;    // PES or PSI start
	unsigned pid;         // Stream PID
	int counter;          // continuity counter
	int transport_error;  // 0 = packet OK, non-zero damaged
	unsigned char section_buf[1024];
	int section_index;
	int section_size;
};

struct PAT_entry
{
	unsigned program_number;
	unsigned PMT_PID;
	unsigned char *last_pmt_payload;
	unsigned last_pmt_length;
};

struct PMT_entry
{
	unsigned program_number;
	unsigned PMT_PID;
	unsigned elementary_PID;
	unsigned ccx_stream_type;
	unsigned printable_stream_type;
};

struct EIT_buffer
{
	uint32_t prev_ccounter;
	uint8_t *buffer;
	uint32_t buffer_length;
	uint32_t ccounter;
};

struct EPG_rating
{
	char country_code[4];
	uint8_t age;
};

struct EPG_event
{
	uint32_t id;
	char start_time_string[21]; //"YYYYMMDDHHMMSS +0000" = 20 chars
	char end_time_string[21];
	uint8_t running_status;
	uint8_t free_ca_mode;
	char ISO_639_language_code[4];
	char *event_name;
	char *text;
	char extended_ISO_639_language_code[4];
	char *extended_text;
	uint8_t has_simple;
	struct EPG_rating *ratings;
	uint32_t num_ratings;
	uint8_t *categories;
	uint32_t num_categories;
	uint16_t service_id;
	long long int count; //incremented by one each time the event is updated
	uint8_t live_output; //boolean flag, true if this event has been output
};

#define EPG_MAX_EVENTS 60*24*7
struct EIT_program
{
	uint32_t array_len;
	struct EPG_event epg_events[EPG_MAX_EVENTS];
};

/* Report information */
#define SUB_STREAMS_CNT 10
struct file_report
{
	unsigned program_cnt;
	unsigned width;
	unsigned height;
	unsigned aspect_ratio;
	unsigned frame_rate;
	struct ccx_decoder_608_report *data_from_608;
	struct ccx_decoder_708_report_t *data_from_708;
	unsigned dvb_sub_pid[SUB_STREAMS_CNT];
	unsigned tlt_sub_pid[SUB_STREAMS_CNT];
	unsigned mp4_cc_track_cnt;
};

// Stuff for telcc.c
struct ccx_s_teletext_config {
	uint8_t verbose : 1; // should telxcc be verbose?
	uint16_t page; // teletext page containing cc we want to filter
	uint16_t tid; // 13-bit packet ID for teletext stream
	double offset; // time offset in seconds
	uint8_t bom : 1; // print UTF-8 BOM characters at the beginning of output
	uint8_t nonempty : 1; // produce at least one (dummy) frame
	// uint8_t se_mode : 1; // search engine compatible mode => Uses CCExtractor's write_format
	// uint64_t utc_refvalue; // UTC referential value => Moved to ccx_decoders_common, so can be used for other decoders (608/xds) too
	uint16_t user_page; // Page selected by user, which MIGHT be different to 'page' depending on autodetection stuff
	ccx_encoders_transcript_format *transcript_settings; // Keeps the settings for generating transcript output files.
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
#define MAX_PID 65536
struct lib_ccx_ctx
{
	// TODO relates to fts_global
	uint32_t global_timestamp;
	uint32_t min_global_timestamp;
	int global_timestamp_inited;


	// Stuff common to both loops
	unsigned char *buffer;
	LLONG past; /* Position in file, if in sync same as ftell()  */
	unsigned char *pesheaderbuf;
	LLONG inputsize;
	LLONG total_inputsize;
	LLONG total_past; // Only in binary concat mode

	int last_reported_progress;

	// Small buffer to help us with the initial sync
	unsigned char startbytes[STARTBYTESLENGTH];
	unsigned int startbytes_pos;
	int startbytes_avail;

	/* Stats */
	int stat_numuserheaders;
	int stat_dvdccheaders;
	int stat_scte20ccheaders;
	int stat_replay5000headers;
	int stat_replay4000headers;
	int stat_dishheaders;
	int stat_hdtv;
	int stat_divicom;
	unsigned total_pulldownfields;
	unsigned total_pulldownframes;
	int false_pict_header;

	/* GOP-based timing */
	int saw_gop_header;
	int frames_since_last_gop;


	/* Time info for timed-transcript */
	int max_gop_length; // (Maximum) length of a group of pictures
	int last_gop_length; // Length of the previous group of pictures

	// int hex_mode=HEX_NONE; // Are we processing an hex file?

	struct lib_cc_decode *dec_ctx;
	enum ccx_stream_mode_enum stream_mode;
	enum ccx_stream_mode_enum auto_stream;
	int m2ts;


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

	/* Hauppauge support */
	unsigned hauppauge_warning_shown; // Did we detect a possible Hauppauge capture and told the user already?
	unsigned teletext_warning_shown; // Did we detect a possible PAL (with teletext subs) and told the user already?

	// Output structures
	struct ccx_s_write wbout1;
	struct ccx_s_write wbout2;

	/* File handles */
	FILE *fh_out_elementarystream;
	int infd; // descriptor number to input.
	int PIDs_seen[MAX_PID];
	struct PMT_entry *PIDs_programs[MAX_PID];
	
	//struct EIT_buffer eit_buffer;
	struct EIT_buffer epg_buffers[0xfff+1];
	struct EIT_program eit_programs[TS_PMT_MAP_SIZE+1];
	int32_t eit_current_events[TS_PMT_MAP_SIZE+1];
	int16_t ATSC_source_pg_map[0xffff];
	int epg_last_output; 
	int epg_last_live_output; 
	struct file_report freport;

	long capbufsize;
	unsigned char *capbuf;
	long capbuflen; // Bytes read in capbuf

	unsigned hauppauge_mode; // If 1, use PID=1003, process specially and so on
	int live_stream; /* -1 -> Not a complete file but a live stream, without timeout
                       0 -> A regular file
                      >0 -> Live stream with a timeout of this value in seconds */
	int binary_concat; // Disabled by -ve or --videoedited
};
#ifdef DEBUG_TELEXCC
int main_telxcc (int argc, char *argv[]);
#endif

#define buffered_skip(ctx, bytes) if (bytes<=bytesinbuffer-filebuffer_pos) { \
    filebuffer_pos+=bytes; \
    result=bytes; \
} else result=buffered_read_opt (ctx, NULL,bytes);

#define buffered_read(ctx, buffer,bytes) if (bytes<=bytesinbuffer-filebuffer_pos) { \
    if (buffer!=NULL) memcpy (buffer,filebuffer+filebuffer_pos,bytes); \
    filebuffer_pos+=bytes; \
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

#define buffered_read_byte(ctx, buffer) if (bytesinbuffer-filebuffer_pos) { \
    if (buffer) { *buffer=filebuffer[filebuffer_pos]; \
    filebuffer_pos++; \
    result=1; } \
} else result=buffered_read_opt (ctx, buffer,1);

LLONG buffered_read_opt (struct lib_ccx_ctx *ctx, unsigned char *buffer, unsigned int bytes);

struct lib_ccx_ctx* init_libraries(struct ccx_s_options *opt);
void dinit_libraries( struct lib_ccx_ctx **ctx);

//params.c
void parse_parameters (struct ccx_s_options *opt, int argc, char *argv[]);
void usage (void);
int detect_input_file_overwrite(struct lib_ccx_ctx *ctx, const char *output_filename);
int atoi_hex (char *s);
int stringztoms (const char *s, struct ccx_boundary_time *bt);

// general_loop.c
void position_sanity_check (void);
int init_file_buffer( void );
LLONG ps_getmoredata(struct lib_ccx_ctx *ctx);
LLONG general_getmoredata(struct lib_ccx_ctx *ctx);
void raw_loop (struct lib_ccx_ctx *ctx, void *enc_ctx);
LLONG process_raw (struct lib_ccx_ctx *ctx, struct cc_subtitle *sub);
void general_loop(struct lib_ccx_ctx *ctx, void *enc_ctx);
void processhex (char *filename);
void rcwt_loop(struct lib_ccx_ctx *ctx, void *enc_ctx);

// activity.c
void activity_header (void);
void activity_progress (int percentaje, int cur_min, int cur_sec);
void activity_report_version (void);
void activity_input_file_closed (void);
void activity_input_file_open (const char *filename);
void activity_message (const char *fmt, ...);
void  activity_video_info (int hor_size,int vert_size,
    const char *aspect_ratio, const char *framerate);
void activity_program_number (unsigned program_number);
void activity_library_process(enum ccx_common_logging_gui message_type, ...);
void activity_report_data_read (void);

extern LLONG result;
extern int end_of_file;
extern LLONG inbuf;
extern int ccx_bufferdatatype; // Can be RAW or PES

// asf_functions.c
LLONG asf_getmoredata(struct lib_ccx_ctx *ctx);

// wtv_functions.c
LLONG wtv_getmoredata(struct lib_ccx_ctx *ctx);

// avc_functions.c
LLONG process_avc (struct lib_ccx_ctx *ctx, unsigned char *avcbuf, LLONG avcbuflen ,struct cc_subtitle *sub);
void init_avc(void);

// es_functions.c
LLONG process_m2v (struct lib_ccx_ctx *ctx, unsigned char *data, LLONG length,struct cc_subtitle *sub);

extern unsigned top_field_first;

// es_userdata.c
int user_data(struct lib_ccx_ctx *ctx, struct bitstream *ustream, int udtype, struct cc_subtitle *sub);

// bitstream.c - see bitstream.h

// file_functions.c
LLONG getfilesize (int in);
LLONG gettotalfilessize (struct lib_ccx_ctx *ctx);
void prepare_for_new_file (struct lib_ccx_ctx *ctx);
void close_input_file (struct lib_ccx_ctx *ctx);
int switch_to_next_file (struct lib_ccx_ctx *ctx, LLONG bytesinbuffer);
void return_to_buffer (unsigned char *buffer, unsigned int bytes);

// sequencing.c
void init_hdcc (void);
void store_hdcc(struct lib_ccx_ctx *ctx, unsigned char *cc_data, int cc_count, int sequence_number,
			LLONG current_fts_now,struct cc_subtitle *sub);
void anchor_hdcc(int seq);
void process_hdcc (struct lib_ccx_ctx *ctx, struct cc_subtitle *sub);

// params_dump.c
void params_dump(struct lib_ccx_ctx *ctx);
void print_file_report(struct lib_ccx_ctx *ctx);

// output.c
void init_write(struct ccx_s_write *wb, char *filename);
int writeraw (const unsigned char *data, int length, void *private_data, struct cc_subtitle *sub);
void flushbuffer (struct lib_ccx_ctx *ctx, struct ccx_s_write *wb, int closefile);
void writercwtdata (struct lib_cc_decode *ctx, const unsigned char *data);

// stream_functions.c
int isValidMP4Box(unsigned char *buffer, long position, long *nextBoxLocation, int *boxScore);
void detect_stream_type (struct lib_ccx_ctx *ctx);
int detect_myth( struct lib_ccx_ctx *ctx );
int read_video_pes_header (struct lib_ccx_ctx *ctx, unsigned char *nextheader, int *headerlength, int sbuflen);
int read_pts_pes(unsigned char*header, int len);

// ts_functions.c
void init_ts(struct lib_ccx_ctx *ctx);
void dinit_ts (struct lib_ccx_ctx *ctx);
int ts_readpacket(struct lib_ccx_ctx* ctx);
long ts_readstream(struct lib_ccx_ctx *ctx);
LLONG ts_getmoredata(struct lib_ccx_ctx *ctx);
int write_section(struct lib_ccx_ctx *ctx, struct ts_payload *payload, unsigned char*buf, int size, int pos);
int parse_PMT (struct lib_ccx_ctx *ctx, unsigned char *buf, int len, int pos);
int parse_PAT (struct lib_ccx_ctx *ctx);
void parse_EPG_packet (struct lib_ccx_ctx *ctx);
void EPG_free();

// myth.c
void myth_loop(struct lib_ccx_ctx *ctx, void *enc_ctx);

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


unsigned encode_line (unsigned char *buffer, unsigned char *text);
void buffered_seek (struct lib_ccx_ctx *ctx, int offset);
extern void build_parity_table(void);

void tlt_process_pes_packet(struct lib_ccx_ctx *ctx, uint8_t *buffer, uint16_t size);
void telxcc_init(struct lib_ccx_ctx *ctx);
void telxcc_close(struct lib_ccx_ctx *ctx);
void tlt_read_rcwt(struct lib_ccx_ctx *ctx);

extern unsigned rollover_bits;
extern int global_timestamp_inited;

extern int strangeheader;

extern unsigned char *filebuffer;
extern LLONG filebuffer_start; // Position of buffer start relative to file
extern int filebuffer_pos; // Position of pointer relative to buffer start
extern int bytesinbuffer; // Number of bytes we actually have on buffer

extern const char *desc[256];


extern long FILEBUFFERSIZE; // Uppercase because it used to be a define
extern unsigned long net_activity_gui;

/* General (ES stream) video information */
extern unsigned current_hor_size;
extern unsigned current_vert_size;
extern unsigned current_aspect_ratio;
extern unsigned current_frame_rate;

extern enum ccx_bufferdata_type bufferdatatype; // Can be CCX_BUFFERDATA_TYPE_RAW or CCX_BUFFERDATA_TYPE_PES

extern int firstcall;

#define MAXBFRAMES 50
#define SORTBUF (2*MAXBFRAMES+1)
extern int cc_data_count[SORTBUF];
extern unsigned char cc_data_pkts[SORTBUF][10*31*3+1];
extern int has_ccdata_buffered;

extern unsigned char *subline;


// From ts_functions
extern unsigned cap_stream_type;
extern struct ts_payload payload;
extern unsigned char tspacket[188];
extern struct PAT_entry pmt_array[TS_PMT_MAP_SIZE];
extern uint16_t pmt_array_length;
extern unsigned pmtpid;
extern unsigned TS_program_number;
extern unsigned char *last_pat_payload;
extern unsigned last_pat_length;
extern long capbuflen;


#define HAUPPAGE_CCPID	1003 // PID for CC's in some Hauppauge recordings

/* Exit codes. Take this seriously as the GUI depends on them.
   0 means OK as usual,
   <100 means display whatever was output to stderr as a warning
   >=100 means display whatever was output to stdout as an error
*/
// Some moved to ccx_common_common.h
#define EXIT_OK                                 0
#define EXIT_NO_INPUT_FILES                     2
#define EXIT_TOO_MANY_INPUT_FILES               3
#define EXIT_INCOMPATIBLE_PARAMETERS            4
#define EXIT_UNABLE_TO_DETERMINE_FILE_SIZE      6
#define EXIT_MALFORMED_PARAMETER                7
#define EXIT_READ_ERROR                         8
#define EXIT_NOT_CLASSIFIED                     300
#define EXIT_ERROR_IN_CAPITALIZATION_FILE       501
#define EXIT_BUFFER_FULL                        502
#define EXIT_MISSING_ASF_HEADER                 1001
#define EXIT_MISSING_RCWT_HEADER                1002

extern unsigned teletext_mode;

#define MAX_TLT_PAGES 1000
extern short int seen_sub_page[MAX_TLT_PAGES];

extern struct ccx_s_teletext_config tlt_config;
extern uint32_t tlt_packet_counter;
extern uint32_t tlt_frames_produced;

#endif
