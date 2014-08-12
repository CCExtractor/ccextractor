#ifndef CCX_CCEXTRACTOR_H
#define CCX_CCEXTRACTOR_H

#define VERSION "0.72"

// Load common includes and constants for library usage
#include "ccx_common_common.h"

extern int cc_buffer_saved; // Do we have anything in the CC buffer already? 
extern int ccblocks_in_avc_total; // Total CC blocks found by the AVC code
extern int ccblocks_in_avc_lost; // CC blocks found by the AVC code lost due to overwrites (should be 0)

#include "ccx_encoders_common.h"
#include "ccx_decoders_608.h"
#include "ccx_decoders_708.h"
#include "bitstream.h"

#include "networking.h"

#define TS_PMT_MAP_SIZE 128

struct ccx_boundary_time
{
	int hh,mm,ss;
	LLONG time_in_ms;
	int set;
};

struct ccx_s_options // Options from user parameters
{
	int extract; // Extract 1st, 2nd or both fields
	int cc_channel; // Channel we want to dump in srt mode
	int buffer_input;	
	int nofontcolor;
	int notypesetting;
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	int print_file_reports;

	ccx_decoder_608_settings settings_608; //  Contains the settings for the 608 decoder.

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
	ccx_encoders_transcript_format transcript_settings; // Keeps the settings for generating transcript output files.
	char millis_separator;
	enum ccx_encoding_type encoding;
	enum ccx_output_format write_format; // 0=Raw, 1=srt, 2=SMI
	enum ccx_output_date_format date_format;
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
	char *udpaddr;
	unsigned udpport; // Non-zero => Listen for UDP packets on this port, no files.
	unsigned send_to_srv;
	char *tcpport;
	char *tcp_password;
	char *srv_addr;
	char *srv_port;
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

/* Report information */
#define SUB_STREAMS_CNT 10
struct file_report_t
{
	unsigned program_cnt;
	unsigned width;
	unsigned height;
	unsigned aspect_ratio;
	unsigned frame_rate;
	struct ccx_decoder_608_report_t data_from_608;
	unsigned services708[63];
	unsigned dvb_sub_pid[SUB_STREAMS_CNT]; 
	unsigned tlt_sub_pid[SUB_STREAMS_CNT];
	unsigned mp4_cc_track_cnt;
} file_report;

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

// general_loop.c
void position_sanity_check ();
int init_file_buffer( void );
LLONG ps_getmoredata( void );
LLONG general_getmoredata( void );
void raw_loop (void *enc_ctx);
LLONG process_raw (struct cc_subtitle *sub);
void general_loop(void *enc_ctx);
void processhex (char *filename);
void rcwt_loop(void *enc_ctx);

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
LLONG asf_getmoredata( void );

// wtv_functions.c
LLONG wtv_getmoredata( void );

// avc_functions.c
LLONG process_avc (unsigned char *avcbuf, LLONG avcbuflen, struct cc_subtitle *sub);
void init_avc(void);

// es_functions.c
LLONG process_m2v (unsigned char *data, LLONG length, struct cc_subtitle *sub);

extern unsigned top_field_first;

// es_userdata.c
int user_data(struct bitstream *ustream, int udtype, struct cc_subtitle *sub);

// bitstream.c - see bitstream.h

// file_functions.c
LLONG getfilesize (int in);
LLONG gettotalfilessize (void);
void prepare_for_new_file (void);
void close_input_file (void);
int switch_to_next_file (LLONG bytesinbuffer);
void return_to_buffer (unsigned char *buffer, unsigned int bytes);

// sequencing.c
void init_hdcc (void);
void store_hdcc(unsigned char *cc_data, int cc_count, int sequence_number, LLONG current_fts, struct cc_subtitle *sub);
void anchor_hdcc(int seq);
void process_hdcc (struct cc_subtitle *sub);
int do_cb (unsigned char *cc_block, struct cc_subtitle *sub);
// mp4.c
int processmp4 (char *file,void *enc_ctx);

// params_dump.c
void params_dump(void);
void print_file_report(void);

// output.c
void init_write (struct ccx_s_write *wb);
void writeraw (const unsigned char *data, int length, struct ccx_s_write *wb);
void writedata(const unsigned char *data, int length, ccx_decoder_608_context *context, struct cc_subtitle *sub);
void flushbuffer (struct ccx_s_write *wb, int closefile);
void printdata (const unsigned char *data1, int length1,const unsigned char *data2, int length2, struct cc_subtitle *sub);
void writercwtdata (const unsigned char *data);

// stream_functions.c
void detect_stream_type (void);
int detect_myth( void );
int read_video_pes_header (unsigned char *header, int *headerlength, int sbuflen);
int read_pts_pes(unsigned char*header, int len);

// ts_functions.c
void init_ts( void );
int ts_readpacket(void);
long ts_readstream(void);
LLONG ts_getmoredata( void );
int write_section(struct ts_payload *payload, unsigned char*buf, int size, int pos);
int parse_PMT (unsigned char *buf,int len, int pos);
int parse_PAT (void);

// myth.c
void myth_loop(void *enc_ctx);

// utility.c
void fatal(int exit_code, const char *fmt, ...);
void dvprint(const char *fmt, ...);
void mprint (const char *fmt, ...);
void dbg_print(LLONG mask, const char *fmt, ...);
void init_boundary_time (struct ccx_boundary_time *bt);
void sleep_secs (int secs);
void dump (LLONG mask, unsigned char *start, int l, unsigned long abs_start, unsigned clear_high_bit);
bool_t in_array(uint16_t *array, uint16_t length, uint16_t element) ;
int hex2int (char high, char low);
void timestamp_to_srttime(uint64_t timestamp, char *buffer);
void millis_to_date (uint64_t timestamp, char *buffer) ;
int levenshtein_dist (const uint64_t *s1, const uint64_t *s2, unsigned s1len, unsigned s2len);


unsigned encode_line (unsigned char *buffer, unsigned char *text);
void buffered_seek (int offset);
extern void build_parity_table(void);

void tlt_process_pes_packet(uint8_t *buffer, uint16_t size) ;
void telxcc_init(void);
void telxcc_close(void);

extern unsigned rollover_bits;
extern uint32_t global_timestamp, min_global_timestamp;
extern int global_timestamp_inited;

extern int saw_caption_block;


extern unsigned char *buffer;
extern LLONG past;
extern LLONG total_inputsize, total_past; // Only in binary concat mode

extern char **inputfile;
extern int current_file;
extern LLONG result; // Number of bytes read/skipped in last read operation


extern int strangeheader;

extern unsigned char startbytes[STARTBYTESLENGTH]; 
extern unsigned int startbytes_pos;
extern int startbytes_avail; // Needs to be able to hold -1 result.

extern unsigned char *pesheaderbuf;

extern unsigned total_pulldownfields;
extern unsigned total_pulldownframes;

extern unsigned char *filebuffer;
extern LLONG filebuffer_start; // Position of buffer start relative to file
extern int filebuffer_pos; // Position of pointer relative to buffer start
extern int bytesinbuffer; // Number of bytes we actually have on buffer

extern ccx_decoder_608_context context_cc608_field_1, context_cc608_field_2;

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

extern int last_reported_progress;
extern int cc_to_stdout;

extern unsigned hauppauge_warning_shown;
extern unsigned char *subline;
extern int saw_gop_header;
extern int max_gop_length;
extern int last_gop_length;
extern int frames_since_last_gop;
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
// Some moved to ccx_common_common.h
#define EXIT_OK                                 0
#define EXIT_NO_INPUT_FILES                     2
#define EXIT_TOO_MANY_INPUT_FILES               3
#define EXIT_INCOMPATIBLE_PARAMETERS            4
#define EXIT_FILE_CREATION_FAILED               5
#define EXIT_UNABLE_TO_DETERMINE_FILE_SIZE      6
#define EXIT_MALFORMED_PARAMETER                7
#define EXIT_READ_ERROR                         8
#define EXIT_NOT_CLASSIFIED                     300
#define EXIT_NOT_ENOUGH_MEMORY                  500
#define EXIT_ERROR_IN_CAPITALIZATION_FILE       501
#define EXIT_BUFFER_FULL                        502
#define EXIT_MISSING_ASF_HEADER                 1001
#define EXIT_MISSING_RCWT_HEADER                1002

extern int PIDs_seen[65536];
extern struct PMT_entry *PIDs_programs[65536];

// extern LLONG ts_start_of_xds; // Moved to 608.h, only referenced in 608.c & xds.c
//extern int timestamps_on_transcript;

extern unsigned teletext_mode;

#define MAX_TLT_PAGES 1000
extern short int seen_sub_page[MAX_TLT_PAGES];

extern struct ccx_s_teletext_config tlt_config;
extern uint32_t tlt_packet_counter;
extern uint32_t tlt_frames_produced;

#endif
