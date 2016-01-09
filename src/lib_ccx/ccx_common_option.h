#ifndef CCX_COMMON_OPTION_H
#define CCX_COMMON_OPTION_H

#include "ccx_common_timing.h"
#include "ccx_decoders_608.h"
#include "ccx_encoders_structs.h"
#include "list.h"

struct demuxer_cfg
{
	int m2ts; // Regular TS or M2TS
	enum ccx_stream_mode_enum auto_stream;
	char *out_elementarystream_filename;

	/* subtitle codec type */
	enum ccx_code_type codec;
	enum ccx_code_type nocodec;

	unsigned ts_autoprogram; // Try to find a stream with captions automatically (no -pn needed)
	unsigned ts_allprogram;
	unsigned ts_cappids[128]; // PID for stream that holds caption information
	int nb_ts_cappid;
	unsigned ts_forced_cappid ; // If 1, never mess with the selected PID
	int ts_forced_program; // Specific program to process in TS files, if ts_forced_program_selected==1
	unsigned ts_forced_program_selected;
	int ts_datastreamtype ; // User WANTED stream type (i.e. use the stream that has this type)
	unsigned ts_forced_streamtype; // User selected (forced) stream type
};

struct encoder_cfg
{
	int extract; // Extract 1st, 2nd or both fields
	int dtvcc_extract; // 1 or 0
	int gui_mode_reports; // If 1, output in stderr progress updates so the GUI can grab them
	char *output_filename;
	enum ccx_output_format write_format; // 0=Raw, 1=srt, 2=SMI
	
	enum ccx_encoding_type encoding;
	enum ccx_output_date_format date_format;
	char millis_separator;
	int autodash; // Add dashes (-) before each speaker automatically?
	int trim_subs; // "    Remove spaces at sides?    "
	int sentence_cap ; // FIX CASE? = Fix case?
	/* Credit stuff */
	char *start_credits_text;
	char *end_credits_text;
	struct ccx_boundary_time startcreditsnotbefore, startcreditsnotafter; // Where to insert start credits, if possible
	struct ccx_boundary_time startcreditsforatleast, startcreditsforatmost; // How long to display them?
	struct ccx_boundary_time endcreditsforatleast, endcreditsforatmost;

	ccx_encoders_transcript_format transcript_settings; // Keeps the settings for generating transcript output files.
	unsigned int send_to_srv;
	int no_bom; // Set to 1 when no BOM (Byte Order Mark) should be used for files. Note, this might make files unreadable in windows!
	char *first_input_file;
	int multiple_files;
	int no_font_color;
	int no_type_setting;
	int cc_to_stdout; // If this is set to 1, the stdout will be flushed when data was written to the screen during a process_608 call.
	int line_terminator_lf; // 0 = CRLF, 1=LF
	LLONG subs_delay; // ms to delay (or advance) subs
	int program_number;
	unsigned char in_format;

	//CEA-708
	int services_enabled[CCX_DTVCC_MAX_SERVICES];
	char** services_charsets;
	char* all_services_charset;
};
struct ccx_s_options // Options from user parameters
{
	int extract; // Extract 1st, 2nd or both fields
	int no_rollup;
	int noscte20;
	int cc_channel; // Channel we want to dump in srt mode
	int buffer_input;
	int nofontcolor;
	int nohtmlescape;
	int notypesetting;
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	int print_file_reports;


	ccx_decoder_608_settings settings_608; //  Contains the settings for the 608 decoder.
	ccx_decoder_dtvcc_settings settings_dtvcc; //Same for 708 decoder

	char millis_separator;
	int binary_concat; // Disabled by -ve or --videoedited
	int use_gop_as_pts; // Use GOP instead of PTS timing (0=do as needed, 1=always, -1=never)
	int fix_padding; // Replace 0000 with 8080 in HDTV (needed for some cards)
	int gui_mode_reports; // If 1, output in stderr progress updates so the GUI can grab them
	int no_progress_bar; // If 1, suppress the output of the progress to stdout
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
	unsigned int hauppauge_mode; // If 1, use PID=1003, process specially and so on
	int wtvconvertfix; // Fix broken Windows 7 conversion
	int wtvmpeg2;
	int auto_myth; // Use myth-tv mpeg code? 0=no, 1=yes, 2=auto
	/* MP4 related stuff */
	unsigned mp4vidtrack; // Process the video track even if a CC dedicated track exists.
	/* General settings */
	int usepicorder; // Force the use of pic_order_cnt_lsb in AVC/H.264 data streams
	int xmltv; // 1 = full output. 2 = live output. 3 = both
	int xmltvliveinterval; // interval in seconds between writting xmltv output files in live mode
	int xmltvoutputinterval; // interval in seconds between writting xmltv full file output
	int xmltvonlycurrent; // 0 off 1 on

	ccx_encoders_transcript_format transcript_settings; // Keeps the settings for generating transcript output files.
	enum ccx_output_date_format date_format;
	unsigned send_to_srv;
	enum ccx_output_format write_format; // 0=Raw, 1=srt, 2=SMI
	LLONG debug_mask; // dbg_print will use this mask to print or ignore different types
	LLONG debug_mask_on_debug; // If we're using temp_debug to enable/disable debug "live", this is the mask when temp_debug=1
	/* Networking */
	char *udpaddr;
	unsigned udpport; // Non-zero => Listen for UDP packets on this port, no files.
	char *tcpport;
	char *tcp_password;
	char *tcp_desc;
	char *srv_addr;
	char *srv_port;
	int noautotimeref; // Do NOT set time automatically?
	enum ccx_datasource input_source; // Files, stdin or network

	char *output_filename;

	char **inputfile; // List of files to process
	int num_input_files; // How many?
	struct demuxer_cfg demux_cfg;
	struct encoder_cfg enc_cfg;
	LLONG subs_delay; // ms to delay (or advance) subs
	int cc_to_stdout; // If this is set to 1, the stdout will be flushed when data was written to the screen during a process_608 call.
	int multiprogram;
	int out_interval;
};

extern struct ccx_s_options ccx_options;
void init_options (struct ccx_s_options *options);
#endif
