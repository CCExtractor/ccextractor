#ifndef CCX_CCEXTRACTOR_H
#define CCX_CCEXTRACTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h> 
#include <stdarg.h>
#include <errno.h>

// compatibility across platforms
#include "platform.h"

#define VERSION "0.70a1"

extern int cc_buffer_saved; // Do we have anything in the CC buffer already? 
extern int ccblocks_in_avc_total; // Total CC blocks found by the AVC code
extern int ccblocks_in_avc_lost; // CC blocks found by the AVC code lost due to overwrites (should be 0)

#include "608.h"
#include "708.h"
#include "bitstream.h"
#include "constants.h"

#define TS_PMT_MAP_SIZE 128

struct ccx_boundary_time
{
	int hh,mm,ss;
	LLONG time_in_ms;
	int set;
};

typedef struct {
	// TODO: add more options, and (perhaps) reduce other ccextractor options?
	int showStartTime, showEndTime; // Show start and/or end time.
	int showMode; // Show which mode if available (E.G.: POP, RU1, ...)	
	int showCC; // Show which CC channel has been captured.	
	int relativeTimestamp; // Timestamps relative to start of sample or in UTC?
	int xds; // Show XDS or not
	int useColors; // Add colors or no colors

} ccx_transcript_format;

extern ccx_transcript_format ccx_default_transcript_settings;

struct ccx_s_options // Options from user parameters
{
	int extract; // Extract 1st, 2nd or both fields
	int cc_channel; // Channel we want to dump in srt mode
	int buffer_input;
	int direct_rollup;
	int nofontcolor;
	int notypesetting;
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	int print_file_reports;

	/* subtitle codec type */
	enum cxx_code_type codec;
	enum cxx_code_type nocodec;
	/* Credit stuff */
	char *start_credits_text;
	char *end_credits_text;	
	struct ccx_boundary_time startcreditsnotbefore, startcreditsnotafter; // Where to insert start credits, if possible
	struct ccx_boundary_time startcreditsforatleast, startcreditsforatmost; // How long to display them?
	struct ccx_boundary_time endcreditsforatleast, endcreditsforatmost;
	int binary_concat; // Disabled by -ve or --videoedited
	int use_gop_as_pts; // Use GOP instead of PTS timing (0=do as needed, 1=always, -1=never)
	int fix_padding; // Replace 0000 with 8080 in HDTV (needed for some cards)
	int norollup; // If 1, write one line at a time
	int forced_ru; // 0=Disabled, 1, 2 or 3=max lines in roll-up mode
	int trim_subs; // "    Remove spaces at sides?    "
	int gui_mode_reports; // If 1, output in stderr progress updates so the GUI can grab them
	int no_progress_bar; // If 1, suppress the output of the progress to stdout
	int sentence_cap ; // FIX CASE? = Fix case?
	char *sentence_cap_file; // Extra words file?
	int live_stream; /* -1 -> Not a complete file but a live stream, without timeout
                       0 -> A regular file 
                      >0 -> Live stream with a timeout of this value in seconds */
	int messages_target; // 0 = nowhere (quiet), 1=stdout, 2=stderr
	/* Levenshtein's parameters, for string comparison */
	int levdistmincnt, levdistmaxpct; // Means 2 fails or less is "the same", 10% or less is also "the same"	
	int investigate_packets; // Look for captions in all packets when everything else fails
	int fullbin; // Disable pruning of padding cc blocks
	int nosync; // Disable syncing
	unsigned hauppauge_mode; // If 1, use PID=1003, process specially and so on	
	int wtvconvertfix; // Fix broken Windows 7 conversion
	int wtvmpeg2; 
	int auto_myth; // Use myth-tv mpeg code? 0=no, 1=yes, 2=auto
	/* MP4 related stuff */
	unsigned mp4vidtrack; // Process the video track even if a CC dedicated track exists.
	/* General settings */
	int usepicorder; // Force the use of pic_order_cnt_lsb in AVC/H.264 data streams
	int autodash; // Add dashes (-) before each speaker automatically?
	unsigned teletext_mode; // 0=Disabled, 1 = Not found, 2=Found
	ccx_transcript_format transcript_settings; // Keeps the settings for generating transcript output files.
	char millis_separator;
	LLONG screens_to_process; // How many screenfuls we want?
	enum ccx_encoding_type encoding;
	enum ccx_output_format write_format; // 0=Raw, 1=srt, 2=SMI
	enum ccx_output_date_format date_format;
	enum color_code cc608_default_color;
	char *output_filename;
	char *out_elementarystream_filename;
	LLONG debug_mask; // dbg_print will use this mask to print or ignore different types
	LLONG debug_mask_on_debug; // If we're using temp_debug to enable/disable debug "live", this is the mask when temp_debug=1
	unsigned ts_autoprogram ; // Try to find a stream with captions automatically (no -pn needed)
	unsigned ts_cappid ; // PID for stream that holds caption information
	unsigned ts_forced_cappid ; // If 1, never mess with the selected PID
	unsigned ts_forced_program; // Specific program to process in TS files, if ts_forced_program_selected==1
	unsigned ts_forced_program_selected; 
	int ts_datastreamtype ; // User WANTED stream type (i.e. use the stream that has this type)
	unsigned ts_forced_streamtype; // User selected (forced) stream type
	/* Networking */
	in_addr_t udpaddr;
	unsigned udpport; // Non-zero => Listen for UDP packets on this port, no files.
	int line_terminator_lf; // 0 = CRLF, 1=LF
	int noautotimeref; // Do NOT set time automatically?
	enum ccx_datasource input_source; // Files, stdin or network
	
};

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


struct ccx_s_write
{
	int fh;
	char *filename;
	void* spupng_data;
};


struct gop_time_code
{
	int drop_frame_flag;
	int time_code_hours;
	int time_code_minutes;
	int marker_bit;
	int time_code_seconds;
	int time_code_pictures;
	int inited;
	LLONG ms;
};


/* Report information */
#define SUB_STREAMS_CNT 10
struct file_report_t
{
	unsigned program_cnt;
	unsigned width;
	unsigned height;
	unsigned aspect_ratio;
	unsigned frame_rate;
	unsigned xds : 1;
	unsigned cc_channels_608[4];
	unsigned services708[63];
	unsigned dvb_sub_pid[SUB_STREAMS_CNT]; 
	unsigned tlt_sub_pid[SUB_STREAMS_CNT];
	unsigned mp4_cc_track_cnt;
} file_report;


// Stuff for telcc.cpp
struct ccx_s_teletext_config {
	uint8_t verbose : 1; // should telxcc be verbose?
	uint16_t page; // teletext page containing cc we want to filter
	uint16_t tid; // 13-bit packet ID for teletext stream
	double offset; // time offset in seconds
	uint8_t bom : 1; // print UTF-8 BOM characters at the beginning of output
	uint8_t nonempty : 1; // produce at least one (dummy) frame
	// uint8_t se_mode : 1; // search engine compatible mode => Uses CCExtractor's write_format
	// uint64_t utc_refvalue; // UTC referential value => Moved to CCExtractor global, so can be used for 608 too
	uint16_t user_page; // Page selected by user, which MIGHT be different to 'page' depending on autodetection stuff
};

#define buffered_skip(bytes) if (bytes<=bytesinbuffer-filebuffer_pos) { \
    filebuffer_pos+=bytes; \
    result=bytes; \
} else result=buffered_read_opt (NULL,bytes);

#define buffered_read(buffer,bytes) if (bytes<=bytesinbuffer-filebuffer_pos) { \
    if (buffer!=NULL) memcpy (buffer,filebuffer+filebuffer_pos,bytes); \
    filebuffer_pos+=bytes; \
    result=bytes; \
} else { result=buffered_read_opt (buffer,bytes); if (ccx_options.gui_mode_reports && ccx_options.input_source==CCX_DS_NETWORK) {net_activity_gui++; if (!(net_activity_gui%1000))activity_report_data_read();}}

#define buffered_read_4(buffer) if (4<=bytesinbuffer-filebuffer_pos) { \
    if (buffer) { buffer[0]=filebuffer[filebuffer_pos]; \
    buffer[1]=filebuffer[filebuffer_pos+1]; \
    buffer[2]=filebuffer[filebuffer_pos+2]; \
    buffer[3]=filebuffer[filebuffer_pos+3]; \
    filebuffer_pos+=4; \
    result=4; } \
} else result=buffered_read_opt (buffer,4);

#define buffered_read_byte(buffer) if (bytesinbuffer-filebuffer_pos) { \
    if (buffer) { *buffer=filebuffer[filebuffer_pos]; \
    filebuffer_pos++; \
    result=1; } \
} else result=buffered_read_opt (buffer,1);

extern LLONG buffered_read_opt (unsigned char *buffer, unsigned int bytes);

//params.c
void parse_parameters (int argc, char *argv[]);
void usage (void);
int atoi_hex (char *s);
int stringztoms (const char *s, struct ccx_boundary_time *bt);

// general_loop.cpp
void position_sanity_check ();
int init_file_buffer( void );
LLONG ps_getmoredata( void );
LLONG general_getmoredata( void );
void raw_loop (void);
LLONG process_raw (void);
void general_loop(void);
void processhex (char *filename);
void rcwt_loop( void );

#ifndef __cplusplus
#define false 0
#define true 1
#endif

// activity.cpp
void activity_header (void);
void activity_progress (int percentaje, int cur_min, int cur_sec);
void activity_report_version (void);
void activity_input_file_closed (void);
void activity_input_file_open (const char *filename);
void activity_message (const char *fmt, ...);
void  activity_video_info (int hor_size,int vert_size, 
    const char *aspect_ratio, const char *framerate);
void activity_program_number (unsigned program_number);
void activity_xds_program_name (const char *program_name);
void activity_xds_network_call_letters (const char *program_name);
void activity_xds_program_identification_number (unsigned minutes, unsigned hours, unsigned date, unsigned month);
void activity_xds_program_description (int line_num, const char *program_desc);
void activity_report_data_read (void);

extern LLONG result;
extern int end_of_file;
extern LLONG inbuf;
extern int ccx_bufferdatatype; // Can be RAW or PES

// asf_functions.cpp
LLONG asf_getmoredata( void );

// wtv_functions.cpp
LLONG wtv_getmoredata( void );

// avc_functions.cpp
LLONG process_avc (unsigned char *avcbuf, LLONG avcbuflen);
void init_avc(void);

// es_functions.cpp
LLONG process_m2v (unsigned char *data, LLONG length);

extern unsigned top_field_first;

// es_userdata.cpp
int user_data(struct bitstream *ustream, int udtype);

// bitstream.cpp - see bitstream.h

// 608.cpp
int write_cc_buffer(struct s_context_cc608 *context);
unsigned char *debug_608toASC (unsigned char *ccdata, int channel);


// cc_decoders_common.cpp
LLONG get_visible_start (void);
LLONG get_visible_end (void);

// file_functions.cpp
LLONG getfilesize (int in);
LLONG gettotalfilessize (void);
void prepare_for_new_file (void);
void close_input_file (void);
int switch_to_next_file (LLONG bytesinbuffer);
int init_sockets (void);
void return_to_buffer (unsigned char *buffer, unsigned int bytes);

// timing.cpp
void set_fts(void);
LLONG get_fts(void);
LLONG get_fts_max(void);
char *print_mstime( LLONG mstime );
char *print_mstime2buf( LLONG mstime , char *buf );
void print_debug_timing( void );
int gop_accepted(struct gop_time_code* g );
void calculate_ms_gop_time (struct gop_time_code *g);

// sequencing.cpp
void init_hdcc (void);
void store_hdcc(unsigned char *cc_data, int cc_count, int sequence_number, LLONG current_fts);
void anchor_hdcc(int seq);
void process_hdcc (void);
int do_cb (unsigned char *cc_block);

// mp4.cpp
int processmp4 (char *file);

// params_dump.cpp
void params_dump(void);
void print_file_report(void);

// output.cpp
void init_write (struct ccx_s_write *wb);
void writeraw (const unsigned char *data, int length, struct ccx_s_write *wb);
void writedata(const unsigned char *data, int length, struct s_context_cc608 *context);
void flushbuffer (struct ccx_s_write *wb, int closefile);
void printdata (const unsigned char *data1, int length1,const unsigned char *data2, int length2);
void writercwtdata (const unsigned char *data);

// stream_functions.cpp
void detect_stream_type (void);
int detect_myth( void );
int read_video_pes_header (unsigned char *header, int *headerlength, int sbuflen);
int read_pts_pes(unsigned char*header, int len);

// ts_functions.cpp
void init_ts( void );
int ts_readpacket(void);
long ts_readstream(void);
LLONG ts_getmoredata( void );
int write_section(struct ts_payload *payload, unsigned char*buf, int size, int pos);
int parse_PMT (unsigned char *buf,int len, int pos);
int parse_PAT (void);

// myth.cpp
void myth_loop(void);

// mp4_bridge2bento4.cpp
void mp4_loop (char *filename);

// xds.cpp
void process_xds_bytes (const unsigned char hi, int lo);
void do_end_of_xds (unsigned char expected_checksum);
void xds_init();

// ccextractor.cpp
LLONG calculate_gop_mstime (struct gop_time_code *g);
void set_fts(void);
char *print_mstime( LLONG mstime );
void print_debug_timing( void );
int switch_to_next_file (LLONG bytesinbuffer);

// utility.cpp
void fatal(int exit_code, const char *fmt, ...);
void dvprint(const char *fmt, ...);
void mprint (const char *fmt, ...);
void subsprintf (const char *fmt, ...);
void dbg_print(LLONG mask, const char *fmt, ...);
void fdprintf (int fd, const char *fmt, ...);
void init_boundary_time (struct ccx_boundary_time *bt);
void sleep_secs (int secs);
void dump (LLONG mask, unsigned char *start, int l, unsigned long abs_start, unsigned clear_high_bit);
bool_t in_array(uint16_t *array, uint16_t length, uint16_t element) ;
int hex2int (char high, char low);
void timestamp_to_srttime(uint64_t timestamp, char *buffer);
void millis_to_date (uint64_t timestamp, char *buffer) ;
int levenshtein_dist (const uint64_t *s1, const uint64_t *s2, unsigned s1len, unsigned s2len);

void init_context_cc608(struct s_context_cc608 *data, int field);
unsigned encode_line (unsigned char *buffer, unsigned char *text);
void buffered_seek (int offset);
void write_subtitle_file_header(struct ccx_s_write *out);
void write_subtitle_file_footer(struct ccx_s_write *out);
extern void build_parity_table(void);

void tlt_process_pes_packet(uint8_t *buffer, uint16_t size) ;
void telxcc_init(void);
void telxcc_close(void);
void mstotime(LLONG milli, unsigned *hours, unsigned *minutes,
	unsigned *seconds, unsigned *ms);

extern struct gop_time_code gop_time, first_gop_time, printed_gop;
extern int gop_rollover;
extern LLONG min_pts, sync_pts, current_pts;
extern unsigned rollover_bits;
extern uint32_t global_timestamp, min_global_timestamp;
extern int global_timestamp_inited;
extern LLONG fts_now; // Time stamp of current file (w/ fts_offset, w/o fts_global)
extern LLONG fts_offset; // Time before first sync_pts
extern LLONG fts_fc_offset; // Time before first GOP
extern LLONG fts_max; // Remember the maximum fts that we saw in current file
extern LLONG fts_global; // Duration of previous files (-ve mode)
// Count 608 (per field) and 708 blocks since last set_fts() call
extern int cb_field1, cb_field2, cb_708;
extern int saw_caption_block;


extern unsigned char *buffer;
extern LLONG past;
extern LLONG total_inputsize, total_past; // Only in binary concat mode

extern char **inputfile;
extern int current_file;
extern LLONG result; // Number of bytes read/skipped in last read operation


extern struct sockaddr_in servaddr, cliaddr;

extern int strangeheader;

extern unsigned char startbytes[STARTBYTESLENGTH]; 
extern unsigned int startbytes_pos;
extern int startbytes_avail; // Needs to be able to hold -1 result.

extern unsigned char *pesheaderbuf;
extern int pts_set; //0 = No, 1 = received, 2 = min_pts set
extern unsigned long long max_dif;

extern int MPEG_CLOCK_FREQ; // This is part of the standard

extern unsigned pts_big_change;
extern unsigned total_frames_count;
extern unsigned total_pulldownfields;
extern unsigned total_pulldownframes;

extern int CaptionGap;

extern unsigned char *filebuffer;
extern LLONG filebuffer_start; // Position of buffer start relative to file
extern int filebuffer_pos; // Position of pointer relative to buffer start
extern int bytesinbuffer; // Number of bytes we actually have on buffer

extern struct s_context_cc608 context_cc608_field_1, context_cc608_field_2;

extern const char *desc[256];

extern FILE *fh_out_elementarystream;
extern int infd;
extern int false_pict_header;

extern int stat_numuserheaders;
extern int stat_dvdccheaders;
extern int stat_scte20ccheaders;
extern int stat_replay5000headers;
extern int stat_replay4000headers;
extern int stat_dishheaders;
extern int stat_hdtv;
extern int stat_divicom;
extern enum ccx_stream_mode_enum stream_mode;
extern int cc_stats[4];
extern LLONG inputsize;

extern LLONG subs_delay; 
extern int startcredits_displayed, end_credits_displayed;
extern LLONG last_displayed_subs_ms; 
extern int processed_enough;
extern unsigned char usercolor_rgb[8];

extern const char *extension;
extern long FILEBUFFERSIZE; // Uppercase because it used to be a define
extern struct ccx_s_options ccx_options;
extern int temp_debug;
extern unsigned long net_activity_gui;

/* General (ES stream) video information */
extern unsigned current_hor_size;
extern unsigned current_vert_size;
extern unsigned current_aspect_ratio;
extern unsigned current_frame_rate;
extern double current_fps;

extern int end_of_file;
extern LLONG inbuf;
extern enum ccx_bufferdata_type bufferdatatype; // Can be CCX_BUFFERDATA_TYPE_RAW or CCX_BUFFERDATA_TYPE_PES

extern unsigned top_field_first;

extern int firstcall;
extern LLONG minimum_fts; // No screen should start before this FTS

#define MAXBFRAMES 50
#define SORTBUF (2*MAXBFRAMES+1)
extern int cc_data_count[SORTBUF];
extern unsigned char cc_data_pkts[SORTBUF][10*31*3+1];
extern int has_ccdata_buffered;
extern int current_field;

extern int last_reported_progress;
extern int cc_to_stdout;

extern unsigned hauppauge_warning_shown;
extern unsigned char *subline;
extern int saw_gop_header;
extern int max_gop_length;
extern int last_gop_length;
extern int frames_since_last_gop;
extern LLONG fts_at_gop_start;
extern int frames_since_ref_time;
extern enum ccx_stream_mode_enum auto_stream;
extern int num_input_files;
extern char *basefilename;
extern int do_cea708; // Process 708 data?
extern int cea708services[63]; // [] -> 1 for services to be processed
extern struct ccx_s_write wbout1, wbout2, *wbxdsout;

extern char **spell_lower;
extern char **spell_correct;
extern int spell_words;
extern int spell_capacity;

extern unsigned char encoded_crlf[16]; // We keep it encoded here so we don't have to do it many times
extern unsigned int encoded_crlf_length;
extern unsigned char encoded_br[16];
extern unsigned int encoded_br_length;


extern enum ccx_frame_type current_picture_coding_type; 
extern int current_tref; // Store temporal reference of current frame

extern int cc608_parity_table[256]; // From myth

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

#define EXIT_OK                                 0
#define EXIT_NO_INPUT_FILES                     2
#define EXIT_TOO_MANY_INPUT_FILES               3
#define EXIT_INCOMPATIBLE_PARAMETERS            4
#define EXIT_FILE_CREATION_FAILED               5
#define EXIT_UNABLE_TO_DETERMINE_FILE_SIZE      6
#define EXIT_MALFORMED_PARAMETER                7
#define EXIT_READ_ERROR                         8
#define EXIT_UNSUPPORTED						9
#define EXIT_NOT_CLASSIFIED                     300
#define EXIT_NOT_ENOUGH_MEMORY                  500
#define EXIT_ERROR_IN_CAPITALIZATION_FILE       501
#define EXIT_BUFFER_FULL                        502
#define EXIT_BUG_BUG                            1000
#define EXIT_MISSING_ASF_HEADER                 1001
#define EXIT_MISSING_RCWT_HEADER                1002

extern int PIDs_seen[65536];
extern struct PMT_entry *PIDs_programs[65536];

extern LLONG ts_start_of_xds; 
//extern int timestamps_on_transcript;

extern unsigned teletext_mode;

#define MAX_TLT_PAGES 1000
extern short int seen_sub_page[MAX_TLT_PAGES];

extern uint64_t utc_refvalue; // UTC referential value
extern struct ccx_s_teletext_config tlt_config;
extern uint32_t tlt_packet_counter;
extern uint32_t tlt_frames_produced;

#endif
