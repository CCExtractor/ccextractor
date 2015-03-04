#ifndef CCX_COMMON_OPTION_H
#define CCX_COMMON_OPTION_H

#include "ccx_common_timing.h"
#include "ccx_decoders_608.h"
#include "ccx_encoders_structs.h"
struct ccx_s_options // Options from user parameters
{
	int extract; // Extract 1st, 2nd or both fields
	int cc_channel; // Channel we want to dump in srt mode
	int buffer_input;
	int nofontcolor;
	int notypesetting;
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	int print_file_reports;

	int no_bom; // Set to 1 when no BOM (Byte Order Mark) should be used for files. Note, this might make files unreadable in windows!

	ccx_decoder_608_settings settings_608; //  Contains the settings for the 608 decoder.

	/* subtitle codec type */
	enum ccx_code_type codec;
	enum ccx_code_type nocodec;
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
	int xmltv; // 1 = full output. 2 = live output. 3 = both
	int xmltvliveinterval; // interval in seconds between writting xmltv output files in live mode
	int xmltvoutputinterval; // interval in seconds between writting xmltv full file output
	int xmltvonlycurrent; // 0 off 1 on
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
	char *tcp_desc;
	char *srv_addr;
	char *srv_port;
	int line_terminator_lf; // 0 = CRLF, 1=LF
	int noautotimeref; // Do NOT set time automatically?
	enum ccx_datasource input_source; // Files, stdin or network

	char **inputfile; // List of files to process
	int num_input_files; // How many?
	enum ccx_stream_mode_enum auto_stream;
	int m2ts; // Regular TS or M2TS
	LLONG subs_delay; // ms to delay (or advance) subs
	int cc_to_stdout; // If this is set to 1, the stdout will be flushed when data was written to the screen during a process_608 call.
	// Output structures
	struct ccx_s_write wbout1;
	struct ccx_s_write wbout2;
};

extern struct ccx_s_options ccx_options;
void init_options (struct ccx_s_options *options);
#endif
