#ifndef CCX_COMMON_OPTION_H
#define CCX_COMMON_OPTION_H

#include "ccx_common_timing.h"
#include "ccx_decoders_608.h"
#include "ccx_encoders_structs.h"
#include "list.h"

struct demuxer_cfg
{
	int m2ts; // regular TS or M2TS
	enum ccx_stream_mode_enum auto_stream;
	char *out_elementarystream_filename;

	// subtitle codec type
	enum ccx_code_type codec;
	enum ccx_code_type nocodec;

	// try to find a stream with captions automatically (no -pn needed)
	unsigned ts_autoprogram;
	unsigned ts_allprogram;
	// PID for stream that holds caption information
	unsigned ts_cappids[128];
	int nb_ts_cappid;
	// if 1, never mess with the selected PID
	unsigned ts_forced_cappid ;
	// specific program to process in TS files, if ts_forced_program_selected == 1
	int ts_forced_program;
	unsigned ts_forced_program_selected;
	// user WANTED stream type (i.e. use the stream that has this type)
	int ts_datastreamtype ;
	// user selected (forced) stream type
	unsigned ts_forced_streamtype;
};

struct encoder_cfg
{
	// extract 1st, 2nd or both fields
	int extract;
	// 1 or 0
	int dtvcc_extract;
	// if 1, output in stderr progress updates so the GUI can grab them
	int gui_mode_reports;
	char *output_filename;
	// 0 = Raw, 1 = srt, 2 = SMI
	enum ccx_output_format write_format;
	int keep_output_closed;
	// force flush on content write
	int force_flush;
	// append mode for output files
	int append_mode;
	int ucla; // 1 if -UCLA used, 0 if not
	
	enum ccx_encoding_type encoding;
	enum ccx_output_date_format date_format;
	char millis_separator;
	// add dashes (-) before each speaker automatically?
	int autodash;
	// remove spaces at sides?
	int trim_subs;
	// FIX CASE? = Fix case?
	int sentence_cap ;
	// split text into complete sentences and prorate time?
	int splitbysentence;
	// if out=curl, where do we send the data to?
	char *curlposturl;

	// write a .sem file on file open and delete it on close?
	int with_semaphore;
	/* Credit stuff */
	char *start_credits_text;
	char *end_credits_text;
	// where to insert start credits, if possible
	struct ccx_boundary_time startcreditsnotbefore, startcreditsnotafter;
	// how long to display them?
	struct ccx_boundary_time startcreditsforatleast, startcreditsforatmost;
	struct ccx_boundary_time endcreditsforatleast, endcreditsforatmost;

	ccx_encoders_transcript_format transcript_settings; // keeps the settings for generating transcript output files.
	unsigned int send_to_srv;
	int no_bom; // set to 1 when no BOM (Byte Order Mark) should be used for files. Note, this might make files unreadable in windows!
	char *first_input_file;
	int multiple_files;
	int no_font_color;
	int no_type_setting;
	// if this is set to 1, the stdout will be flushed when data was written to the screen during a process_608 call
	int cc_to_stdout;
	// 0 = CRLF, 1 = LF
	int line_terminator_lf;
	// ms to delay (or advance) subs
	LLONG subs_delay;
	int program_number;
	unsigned char in_format;

	// CEA-708
	int services_enabled[CCX_DTVCC_MAX_SERVICES];
	char** services_charsets;
	char* all_services_charset;
};
// options from user parameters
struct ccx_s_options
{
	// extract 1st, 2nd or both fields
	int extract;
	int no_rollup;
	int noscte20;
	// channel we want to dump in srt mode
	int cc_channel;
	int buffer_input;
	int nofontcolor;
	int nohtmlescape;
	int notypesetting;
	// segment we actually process
	struct ccx_boundary_time extraction_start, extraction_end;
	int print_file_reports;

	//  contains the settings for the 608 decoder
	ccx_decoder_608_settings settings_608;
	// same for 708 decoder
	ccx_decoder_dtvcc_settings settings_dtvcc;

	char millis_separator;
	// disabled by -ve or --videoedited
	int binary_concat;
	/**
	 * Use GOP instead of PTS timing
	 * 0 = do as needed
	 * 1 = always
	 * -1 = never
	 */
	int use_gop_as_pts;
	// replace 0000 with 8080 in HDTV (needed for some cards)
	int fix_padding;
	// if 1, output in stderr progress updates so the GUI can grab them
	int gui_mode_reports;
	// if 1, suppress the output of the progress to stdout
	int no_progress_bar;
	// extra words file?
	char *sentence_cap_file;
	int live_stream; /* -1 -> Not a complete file but a live stream, without timeout
                       	     0 -> A regular file
                            >0 -> Live stream with a timeout of this value in seconds */
	// 0 = nowhere (quiet), 1 = stdout, 2 = stderr
	int messages_target;
	/* Levenshtein's parameters, for string comparison */
	// means 2 fails or less is "the same", 10% or less is also "the same"
	int levdistmincnt, levdistmaxpct;
	// look for captions in all packets when everything else fails
	int investigate_packets;
	// disable pruning of padding CC blocks
	int fullbin;
	// disable syncing
	int nosync;
	// if 1, use PID=1003, process specially and so on
	unsigned int hauppauge_mode;
	// fix broken Windows 7 conversion
	int wtvconvertfix;
	int wtvmpeg2;
	// use myth-tv mpeg code? 0 = no, 1 = yes, 2 = auto
	int auto_myth;
	/* MP4 related stuff */
	unsigned mp4vidtrack; // process the video track even if a CC dedicated track exists.
	/* General settings */
	// force the use of pic_order_cnt_lsb in AVC/H.264 data streams
	int usepicorder;
	// 1 = full output. 2 = live output. 3 = both
	int xmltv;
	// interval in seconds between writing xmltv output files in live mode
	int xmltvliveinterval;
	// interval in seconds between writing xmltv full file output
	int xmltvoutputinterval;
	// 0 off 1 on
	int xmltvonlycurrent;
	int keep_output_closed;
	// force flush on content write
	int force_flush;
	// append mode for output files
	int append_mode;
	// 1 if UCLA used, 0 if not
	int ucla;
	// 1 if burned-in subtitles to be extracted
	int hardsubx;
	// 1 if Color to be detected for DVB
	int dvbcolor;
	// the name of the language stream for DVB
	char *dvblang;
	// the name of the .traineddata file to be loaded with tesseract
	char *ocrlang;

	/*HardsubX related stuff*/
	int hardsubx_ocr_mode;
	int hardsubx_subcolor;
	float hardsubx_min_sub_duration;
	int hardsubx_detect_italics;
	float hardsubx_conf_thresh;
	float hardsubx_hue;
	float hardsubx_lum_thresh;

	// keeps the settings for generating transcript output files
	ccx_encoders_transcript_format transcript_settings;
	enum ccx_output_date_format date_format;
	unsigned send_to_srv;
	// 0 = Raw, 1 = srt, 2 = SMI
	enum ccx_output_format write_format;
	int use_ass_instead_of_ssa;
	// dbg_print will use this mask to print or ignore different types
	LLONG debug_mask;
	// if we're using temp_debug to enable/disable debug "live", this is the mask when temp_debug = 1
	LLONG debug_mask_on_debug;
	/* Networking */
	char *udpaddr;
	// non-zero => Listen for UDP packets on this port, no files
	unsigned udpport;
	char *tcpport;
	char *tcp_password;
	char *tcp_desc;
	char *srv_addr;
	char *srv_port;
	// do NOT set time automatically?
	int noautotimeref;
	// files, stdin or network
	enum ccx_datasource input_source;

	char *output_filename;

	// list of files to process
	char **inputfile;
	// how many?
	int num_input_files;
	struct demuxer_cfg demux_cfg;
	struct encoder_cfg enc_cfg;
	// ms to delay (or advance) subs
	LLONG subs_delay;
	// if this is set to 1, the stdout will be flushed when data was written to the screen during a process_608 call
	int cc_to_stdout;
	int multiprogram;
	int out_interval;
#ifdef WITH_LIBCURL
	char *curlposturl;
#endif


#ifdef ENABLE_SHARING
	// CC sharing
	int sharing_enabled;
	char *sharing_url;
	// translating
	int translate_enabled;
	char *translate_langs;
	char *translate_key;
#endif
};

extern struct ccx_s_options ccx_options;
void init_options (struct ccx_s_options *options);
#endif
