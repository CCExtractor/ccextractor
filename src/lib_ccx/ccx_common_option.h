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

	/* subtitle codec type */
	enum ccx_code_type codec;
	enum ccx_code_type nocodec;

	unsigned ts_autoprogram; // Try to find a stream with captions automatically (no -pn needed)
	unsigned ts_allprogram;
	unsigned ts_cappids[128]; // PID for stream that holds caption information
	int nb_ts_cappid;
	unsigned ts_forced_cappid; // If 1, never mess with the selected PID
	int ts_forced_program;	   // Specific program to process in TS files, if ts_forced_program_selected==1
	unsigned ts_forced_program_selected;
	int ts_datastreamtype;	       // User WANTED stream type (i.e. use the stream that has this type)
	unsigned ts_forced_streamtype; // User selected (forced) stream type
};

struct encoder_cfg
{
	int extract;	      // Extract 1st, 2nd or both fields
	int dtvcc_extract;    // 1 or 0
	int gui_mode_reports; // If 1, output in stderr progress updates so the GUI can grab them
	char *output_filename;
	enum ccx_output_format write_format; // 0=Raw, 1=srt, 2=SMI
	int keep_output_closed;
	int force_flush; // Force flush on content write
	int append_mode; // Append mode for output files
	int ucla;	 // 1 if -UCLA used, 0 if not

	enum ccx_encoding_type encoding;
	enum ccx_output_date_format date_format;
	char millis_separator;
	int autodash;	     // Add dashes (-) before each speaker automatically?
	int trim_subs;	     // "    Remove spaces at sides?    "
	int sentence_cap;    // FIX CASE? = Fix case?
	int splitbysentence; // Split text into complete sentences and prorate time?
#ifdef WITH_LIBCURL
	char *curlposturl; // If out=curl, where do we send the data to?
#endif
	int filter_profanity; // Censors profane words from subtitles

	int with_semaphore; // Write a .sem file on file open and delete it on close?
	/* Credit stuff */
	char *start_credits_text;
	char *end_credits_text;
	struct ccx_boundary_time startcreditsnotbefore, startcreditsnotafter;	// Where to insert start credits, if possible
	struct ccx_boundary_time startcreditsforatleast, startcreditsforatmost; // How long to display them?
	struct ccx_boundary_time endcreditsforatleast, endcreditsforatmost;

	ccx_encoders_transcript_format transcript_settings; // Keeps the settings for generating transcript output files.
	unsigned int send_to_srv;
	int no_bom; // Set to 1 when no BOM (Byte Order Mark) should be used for files. Note, this might make files unreadable in windows!
	char *first_input_file;
	int multiple_files;
	int no_font_color;
	int no_type_setting;
	int cc_to_stdout;	// If this is set to 1, the stdout will be flushed when data was written to the screen during a process_608 call.
	int line_terminator_lf; // 0 = CRLF, 1=LF
	LLONG subs_delay;	// ms to delay (or advance) subs
	int program_number;
	unsigned char in_format;
	int nospupngocr; // 1 if we don't want to OCR bitmaps to add the text as comments in the XML file in spupng

	// MCC File
	int force_dropframe; // 1 if dropframe frame count should be used. defaults to no drop frame.

	// SCC output framerate
	int scc_framerate;	 // SCC output framerate: 0=29.97 (default), 1=24, 2=25, 3=30
	int scc_accurate_timing; // If 1, use bandwidth-aware timing for broadcast compliance (issue #1120)

	// text -> png (text render)
	char *render_font; // The font used to render text if needed (e.g. teletext->spupng)
	char *render_font_italics;

	// CEA-708
	int services_enabled[CCX_DTVCC_MAX_SERVICES];
	char **services_charsets;
	char *all_services_charset;
	int extract_only_708; // 1 if only 708 subs extraction is enabled
};

struct ccx_s_options // Options from user parameters
{
	int extract;   // Extract 1st, 2nd or both fields
	int no_rollup; // Disable roll-up emulation (no duplicate output in generated file)
	int noscte20;
	int webvtt_create_css;
	int cc_channel; // Channel we want to dump in srt mode
	int buffer_input;
	int nofontcolor;
	int nohtmlescape;
	int notypesetting;
	struct ccx_boundary_time extraction_start, extraction_end; // Segment we actually process
	int print_file_reports;

	ccx_decoder_608_settings settings_608;	   // Contains the settings for the 608 decoder.
	ccx_decoder_dtvcc_settings settings_dtvcc; // Same for 708 decoder
	int is_608_enabled;			   // Is 608 enabled by explicitly using flags(-1,-2,-12)
	int is_708_enabled;			   // Is 708 enabled by explicitly using flags(-svc)

	char millis_separator;
	int binary_concat;	     // Disabled by -ve or --videoedited
	int use_gop_as_pts;	     // Use GOP instead of PTS timing (0=do as needed, 1=always, -1=never)
	int fix_padding;	     // Replace 0000 with 8080 in HDTV (needed for some cards)
	int gui_mode_reports;	     // If 1, output in stderr progress updates so the GUI can grab them
	int no_progress_bar;	     // If 1, suppress the output of the progress to stdout
	char *sentence_cap_file;     // Extra capitalization word file
	int live_stream;	     /* -1 -> Not a complete file but a live stream, without timeout
				     0 -> A regular file
				    >0 -> Live stream with a timeout of this value in seconds */
	char *filter_profanity_file; // Extra profanity word file
	int messages_target;	     // 0 = nowhere (quiet), 1=stdout, 2=stderr
	int timestamp_map;	     // If 1, add WebVTT X-TIMESTAMP-MAP header
	/* Levenshtein's parameters, for string comparison */
	int dolevdist;			  // 0 => don't attempt to correct typos with this algorithm
	int levdistmincnt, levdistmaxpct; // Means 2 fails or less is "the same", 10% or less is also "the same"
	int investigate_packets;	  // Look for captions in all packets when everything else fails
	int fullbin;			  // Disable pruning of padding cc blocks
	int nosync;			  // Disable syncing
	unsigned int hauppauge_mode;	  // If 1, use PID=1003, process specially and so on
	int wtvconvertfix;		  // Fix broken Windows 7 conversion
	int wtvmpeg2;
	int auto_myth; // Use myth-tv mpeg code? 0=no, 1=yes, 2=auto
	/* MP4 related stuff */
	unsigned mp4vidtrack; // Process the video track even if a CC dedicated track exists.
	int extract_chapters; // If 1, extracts chapters (if present), from MP4 files.
	/* General settings */
	int usepicorder;	 // Force the use of pic_order_cnt_lsb in AVC/H.264 data streams
	int xmltv;		 // 1 = full output. 2 = live output. 3 = both
	int xmltvliveinterval;	 // interval in seconds between writing xmltv output files in live mode
	int xmltvoutputinterval; // interval in seconds between writing xmltv full file output
	int xmltvonlycurrent;	 // 0 off 1 on
	int keep_output_closed;
	int force_flush;	  // Force flush on content write
	int append_mode;	  // Append mode for output files
	int ucla;		  // 1 if UCLA used, 0 if not
	int tickertext;		  // 1 if ticker text style burned in subs, 0 if not
	int hardsubx;		  // 1 if burned-in subtitles to be extracted
	int hardsubx_and_common;  // 1 if both burned-in and not burned in need to be extracted
	char *dvblang;		  // The name of the language stream for DVB
	const char *ocrlang;	  // The name of the .traineddata file to be loaded with tesseract
	int ocr_oem;		  // The Tesseract OEM mode, could be 0 (default), 1 or 2
	int psm;		  // The Tesseract PSM mode, could be between 0 and 13. 3 is tesseract default
	int ocr_quantmode;	  // How to quantize the bitmap before passing to to tesseract (0=no quantization at all, 1=CCExtractor's internal)
	int ocr_line_split;	  // If 1, split images into lines before OCR (uses PSM 7 for better accuracy)
	int ocr_blacklist;	  // If 1, use character blacklist to prevent common OCR errors (default: enabled)
	char *mkvlang;		  // The name of the language stream for MKV
	int analyze_video_stream; // If 1, the video stream will be processed even if we're using a different one for subtitles.

	/*HardsubX related stuff*/
	int hardsubx_ocr_mode;
	int hardsubx_subcolor;
	float hardsubx_min_sub_duration;
	int hardsubx_detect_italics;
	float hardsubx_conf_thresh;
	float hardsubx_hue;
	float hardsubx_lum_thresh;

	ccx_encoders_transcript_format transcript_settings; // Keeps the settings for generating transcript output files.
	enum ccx_output_date_format date_format;
	unsigned send_to_srv;
	enum ccx_output_format write_format; // 0=Raw, 1=srt, 2=SMI
	int write_format_rewritten;
	int use_ass_instead_of_ssa;
	int use_webvtt_styling;
	LLONG debug_mask;	   // dbg_print will use this mask to print or ignore different types
	LLONG debug_mask_on_debug; // If we're using temp_debug to enable/disable debug "live", this is the mask when temp_debug=1
	/* Networking */
	char *udpsrc;
	char *udpaddr;
	unsigned udpport; // Non-zero => Listen for UDP packets on this port, no files.
	char *tcpport;
	char *tcp_password;
	char *tcp_desc;
	char *srv_addr;
	char *srv_port;
	int noautotimeref;		  // Do NOT set time automatically?
	enum ccx_datasource input_source; // Files, stdin or network

	char *output_filename;

	char **inputfile;    // List of files to process
	int num_input_files; // How many?
	struct demuxer_cfg demux_cfg;
	struct encoder_cfg enc_cfg;
	LLONG subs_delay;	  // ms to delay (or advance) subs
	int cc_to_stdout;	  // If this is set to 1, the stdout will be flushed when data was written to the screen during a process_608 call.
	int pes_header_to_stdout; // If this is set to 1, the PES Header will be printed to console (debugging purposes)
	int ignore_pts_jumps;	  // If 1, the program will ignore PTS jumps. Sometimes this parameter is required for DVB subs with > 30s pause time
	int multiprogram;
	int out_interval;
	int segment_on_key_frames_only;
	int split_dvb_subs; // Enable per-stream DVB subtitle extraction (0=disabled, 1=enabled)
	int no_dvb_dedup;   // Disable DVB subtitle deduplication (0=dedup enabled, 1=dedup disabled)
	int scc_framerate;  // SCC input framerate: 0=29.97 (default), 1=24, 2=25, 3=30
#ifdef WITH_LIBCURL
	char *curlposturl;
#endif
};

extern struct ccx_s_options ccx_options;
void init_options(struct ccx_s_options *options);
#endif
