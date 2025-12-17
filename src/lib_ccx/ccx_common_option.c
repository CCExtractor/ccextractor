#include "ccx_common_option.h"
#include "ccx_encoders_common.h"
#include "ccx_decoders_708.h"
#include "utility.h"

extern ccx_encoders_transcript_format ccx_encoders_default_transcript_settings;
/* Parameters */
void init_options(struct ccx_s_options *options)
{
#ifdef _WIN32
	options->buffer_input = 1; // In Windows buffering seems to help
#else
	options->buffer_input = 0; // In Linux, not so much.
#endif
	options->nofontcolor = 0;   // 1 = don't put <font color> tags
	options->notypesetting = 0; // 1 = Don't put <i>, <u>, etc typesetting tags
	options->no_rollup = 0;
	options->noscte20 = 0;

	options->settings_608.direct_rollup = 0;
	options->settings_608.no_rollup = 0;
	options->settings_608.force_rollup = 0;
	options->settings_608.screens_to_process = -1;
	options->settings_608.default_color = COL_TRANSPARENT; // Defaults to transparent/no-color.
	options->is_608_enabled = 0;
	options->is_708_enabled = 0;

	options->extract = 1;		   // Extract 1st field only (primary language)
	options->cc_channel = 1;	   // Channel we want to dump in srt mode
	options->binary_concat = 1;	   // Disabled by --videoedited
	options->use_gop_as_pts = 0;	   // Use GOP instead of PTS timing (0=do as needed, 1=always, -1=never)
	options->fix_padding = 0;	   // Replace 0000 with 8080 in HDTV (needed for some cards)
	options->gui_mode_reports = 0;	   // If 1, output in stderr progress updates so the GUI can grab them
	options->no_progress_bar = 0;	   // If 1, suppress the output of the progress to stdout
	options->enc_cfg.sentence_cap = 0; // FIX CASE? = Fix case?
	options->sentence_cap_file = NULL; // Extra words file?
	options->enc_cfg.filter_profanity = 0;
	options->filter_profanity_file = NULL;
	options->enc_cfg.splitbysentence = 0; // Split text into complete sentences and prorate time?
	options->enc_cfg.nospupngocr = 0;
	options->live_stream = 0;     // 0 -> A regular file
	options->messages_target = 1; // 1=stdout
	options->print_file_reports = 0;
	options->timestamp_map = 0; // Disable X-TIMESTAMP-MAP header by default

	/* Levenshtein's parameters, for string comparison */
	options->dolevdist = 1;		  // By default attempt to correct typos
	options->levdistmincnt = 2;	  // Means 2 fails or less is "the same"...
	options->levdistmaxpct = 10;	  // ...10% or less is also "the same"
	options->investigate_packets = 0; // Look for captions in all packets when everything else fails
	options->fullbin = 0;		  // Disable pruning of padding cc blocks
	options->nosync = 0;		  // Disable syncing
	options->hauppauge_mode = 0;	  // If 1, use PID=1003, process specially and so on
	options->wtvconvertfix = 0;	  // Fix broken Windows 7 conversion
	options->wtvmpeg2 = 0;
	options->auto_myth = 2; // 2=auto
	/* MP4 related stuff */
	options->mp4vidtrack = 0;      // Process the video track even if a CC dedicated track exists.
	options->extract_chapters = 0; // By default don't extract chapters.
	/* General stuff */
	options->usepicorder = 0;	  // Force the use of pic_order_cnt_lsb in AVC/H.264 data streams
	options->xmltv = 0;		  // 1 = full output. 2 = live output. 3 = both
	options->xmltvliveinterval = 10;  // interval in seconds between writing xmltv output files in live mode
	options->xmltvoutputinterval = 0; // interval in seconds between writing xmltv full file output
	options->xmltvonlycurrent = 0;	  // 0 off 1 on
	options->keep_output_closed = 0;  // By default just keep the file open.
	options->force_flush = 0;	  // Don't flush whenever content is written.
	options->append_mode = 0;	  // By default, files are overwritten.
	options->ucla = 0;		  // By default, -UCLA not used
	options->tickertext = 0;	  // By default, do not assume ticker style text
	options->hardsubx = 0;		  // By default, don't try to extract hard subtitles
	options->dvblang = NULL;	  // By default, autodetect DVB language
	options->ocrlang = NULL;	  // By default, autodetect .traineddata file
	options->ocr_oem = -1;		  // By default, OEM mode depends on the tesseract version
	options->psm = 3;		  // Default PSM mode (3 is the default tesseract as well)
	options->ocr_quantmode = 0;	  // No quantization (better OCR accuracy for DVB subtitles)
	options->mkvlang = NULL;	  // By default, all the languages are extracted
	options->ignore_pts_jumps = 1;
	options->analyze_video_stream = 0;

	/*HardsubX related stuff*/
	options->hardsubx_ocr_mode = 0;
	options->hardsubx_subcolor = 0;
	options->hardsubx_min_sub_duration = 0.5;
	options->hardsubx_detect_italics = 0;
	options->hardsubx_conf_thresh = 0.0;
	options->hardsubx_hue = 0.0;
	options->hardsubx_lum_thresh = 95.0;
	options->hardsubx_and_common = 0;

	options->transcript_settings = ccx_encoders_default_transcript_settings;
	options->millis_separator = ',';

	options->write_format = CCX_OF_SRT;
	options->date_format = ODF_NONE;
	options->output_filename = NULL;
	options->debug_mask = CCX_DMT_GENERIC_NOTICES;	// dbg_print will use this mask to print or ignore different types
	options->debug_mask_on_debug = CCX_DMT_VERBOSE; // If we're using temp_debug to enable/disable debug "live", this is the mask when temp_debug=1
	/* Networking */
	options->udpsrc = NULL;
	options->udpaddr = NULL;
	options->udpport = 0; // Non-zero => Listen for UDP packets on this port, no files.
	options->send_to_srv = 0;
	options->tcpport = NULL;
	options->tcp_password = NULL;
	options->tcp_desc = NULL;
	options->srv_addr = NULL;
	options->srv_port = NULL;
	options->noautotimeref = 0;	     // Do NOT set time automatically?
	options->input_source = CCX_DS_FILE; // Files, stdin or network
	options->multiprogram = 0;

	// [ADD THIS]
	options->split_dvb_subs = 0;

	options->out_interval = -1;
	options->segment_on_key_frames_only = 0;

	options->subs_delay = 0;

	/* Select subtitle codec */
	options->demux_cfg.codec = CCX_CODEC_ANY;
	options->demux_cfg.nocodec = CCX_CODEC_NONE;

	options->demux_cfg.auto_stream = CCX_SM_AUTODETECT;
	options->demux_cfg.m2ts = 0;
	options->demux_cfg.ts_autoprogram = 0; // Try to find a stream with captions automatically (no -pn needed)
	options->demux_cfg.ts_cappids[0] = 0;  // PID for stream that holds caption information
	options->demux_cfg.nb_ts_cappid = 0;
	options->demux_cfg.ts_forced_program = -1; // Specific program to process in TS files, if ts_forced_program_selected==1
	options->demux_cfg.ts_forced_program_selected = 0;
	options->demux_cfg.ts_datastreamtype = CCX_STREAM_TYPE_UNKNOWNSTREAM;	 // User WANTED stream type (i.e. use the stream that has this type)
	options->demux_cfg.ts_forced_streamtype = CCX_STREAM_TYPE_UNKNOWNSTREAM; // User selected (forced) stream type

	options->enc_cfg.autodash = 0;	// Add dashes (-) before each speaker automatically?
	options->enc_cfg.trim_subs = 0; // "	Remove spaces at sides?	"
	options->enc_cfg.in_format = 1;
	options->enc_cfg.line_terminator_lf = 0; // 0 = CRLF
	options->enc_cfg.start_credits_text = NULL;
	options->enc_cfg.end_credits_text = NULL;
	options->enc_cfg.encoding = CCX_ENC_UTF_8;
	options->enc_cfg.no_bom = 1;
	options->enc_cfg.services_charsets = NULL;
	options->enc_cfg.all_services_charset = NULL;
	options->enc_cfg.with_semaphore = 0;
	options->enc_cfg.force_dropframe = 0; // Assume No Drop Frame for MCC Encode.
	options->enc_cfg.extract_only_708 = 0;

	options->settings_dtvcc.enabled = 0;
	options->settings_dtvcc.active_services_count = 0;
	options->settings_dtvcc.print_file_reports = 1;
	options->settings_dtvcc.no_rollup = 0;
	options->settings_dtvcc.report = NULL;
	memset(
	    options->settings_dtvcc.services_enabled, 0,
	    CCX_DTVCC_MAX_SERVICES * sizeof(options->settings_dtvcc.services_enabled[0]));

#ifdef WITH_LIBCURL
	options->curlposturl = NULL;
#endif

	// Prepare time structures
	init_boundary_time(&options->extraction_start);
	init_boundary_time(&options->extraction_end);

	/* Credit stuff */
	init_boundary_time(&options->enc_cfg.startcreditsnotbefore);
	init_boundary_time(&options->enc_cfg.startcreditsnotafter);
	init_boundary_time(&options->enc_cfg.startcreditsforatleast);
	init_boundary_time(&options->enc_cfg.startcreditsforatmost);
	init_boundary_time(&options->enc_cfg.endcreditsforatleast);
	init_boundary_time(&options->enc_cfg.endcreditsforatmost);

	// Sensible default values for credits
	stringztoms(DEF_VAL_STARTCREDITSNOTBEFORE, &options->enc_cfg.startcreditsnotbefore);
	stringztoms(DEF_VAL_STARTCREDITSNOTAFTER, &options->enc_cfg.startcreditsnotafter);
	stringztoms(DEF_VAL_STARTCREDITSFORATLEAST, &options->enc_cfg.startcreditsforatleast);
	stringztoms(DEF_VAL_STARTCREDITSFORATMOST, &options->enc_cfg.startcreditsforatmost);
	stringztoms(DEF_VAL_ENDCREDITSFORATLEAST, &options->enc_cfg.endcreditsforatleast);
	stringztoms(DEF_VAL_ENDCREDITSFORATMOST, &options->enc_cfg.endcreditsforatmost);
}
