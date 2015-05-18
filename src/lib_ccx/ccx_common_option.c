#include "ccx_common_option.h"
#include "ccx_encoders_common.h"
#include "utility.h"

/* Parameters */
void init_options (struct ccx_s_options *options)
{
#ifdef _WIN32
	options->buffer_input = 1; // In Windows buffering seems to help
	options->no_bom = 0;
#else
	options->buffer_input = 0; // In linux, not so much.
	options->no_bom = 1;
#endif
	options->nofontcolor=0; // 1 = don't put <font color> tags
	options->notypesetting=0; // 1 = Don't put <i>, <u>, etc typesetting tags

	options->no_bom = 0; // Use BOM by default.

	options->settings_608.direct_rollup = 0;
	options->settings_608.no_rollup = 0;
	options->settings_608.force_rollup = 0;
	options->settings_608.screens_to_process = -1;
	options->settings_608.default_color = COL_TRANSPARENT; // Defaults to transparant/no-color.

	/* Select subtitle codec */
	options->codec = CCX_CODEC_ANY;
	options->nocodec = CCX_CODEC_NONE;

	/* Credit stuff */
	options->start_credits_text=NULL;
	options->end_credits_text=NULL;
	options->extract = 1; // Extract 1st field only (primary language)
	options->cc_channel = 1; // Channel we want to dump in srt mode
	options->binary_concat=1; // Disabled by -ve or --videoedited
	options->use_gop_as_pts = 0; // Use GOP instead of PTS timing (0=do as needed, 1=always, -1=never)
	options->fix_padding = 0; // Replace 0000 with 8080 in HDTV (needed for some cards)
	options->trim_subs=0; // "	Remove spaces at sides?	"
	options->gui_mode_reports=0; // If 1, output in stderr progress updates so the GUI can grab them
	options->no_progress_bar=0; // If 1, suppress the output of the progress to stdout
	options->sentence_cap =0 ; // FIX CASE? = Fix case?
	options->sentence_cap_file=NULL; // Extra words file?
	options->live_stream=0; // 0 -> A regular file
	options->messages_target=1; // 1=stdout
	options->print_file_reports=0;
	/* Levenshtein's parameters, for string comparison */
	options->levdistmincnt=2; // Means 2 fails or less is "the same"...
	options->levdistmaxpct=10; // ...10% or less is also "the same"
	options->investigate_packets = 0; // Look for captions in all packets when everything else fails
	options->fullbin=0; // Disable pruning of padding cc blocks
	options->nosync=0; // Disable syncing
	options->hauppauge_mode=0; // If 1, use PID=1003, process specially and so on
	options->wtvconvertfix = 0; // Fix broken Windows 7 conversion
	options->wtvmpeg2 = 0;
	options->auto_myth = 2; // 2=auto
	/* MP4 related stuff */
	options->mp4vidtrack=0; // Process the video track even if a CC dedicated track exists.
	/* General stuff */
	options->usepicorder = 0; // Force the use of pic_order_cnt_lsb in AVC/H.264 data streams
	options->autodash=0; // Add dashes (-) before each speaker automatically?
	options->xmltv=0; // 1 = full output. 2 = live output. 3 = both
	options->xmltvliveinterval=10; // interval in seconds between writting xmltv output files in live mode
	options->xmltvoutputinterval=0; // interval in seconds between writting xmltv full file output
	options->xmltvonlycurrent=0; // 0 off 1 on
	options->teletext_mode=CCX_TXT_AUTO_NOT_YET_FOUND; // 0=Disabled, 1 = Not found, 2=Found

	options->transcript_settings = ccx_encoders_default_transcript_settings;
	options->millis_separator=',';

	options->encoding = CCX_ENC_UTF_8;
	options->write_format=CCX_OF_SRT; // 0=Raw, 1=srt, 2=SMI
	options->date_format=ODF_NONE;
	options->output_filename=NULL;
	options->out_elementarystream_filename=NULL;
	options->debug_mask=CCX_DMT_GENERIC_NOTICES; // dbg_print will use this mask to print or ignore different types
	options->debug_mask_on_debug=CCX_DMT_VERBOSE; // If we're using temp_debug to enable/disable debug "live", this is the mask when temp_debug=1
	options->ts_autoprogram =0; // Try to find a stream with captions automatically (no -pn needed)
	options->ts_cappid = 0; // PID for stream that holds caption information
	options->ts_forced_cappid = 0; // If 1, never mess with the selected PID
	options->ts_forced_program=0; // Specific program to process in TS files, if ts_forced_program_selected==1
	options->ts_forced_program_selected=0;
	options->ts_datastreamtype = -1; // User WANTED stream type (i.e. use the stream that has this type)
	options->ts_forced_streamtype=CCX_STREAM_TYPE_UNKNOWNSTREAM; // User selected (forced) stream type
	/* Networking */
	options->udpaddr = NULL;
	options->udpport=0; // Non-zero => Listen for UDP packets on this port, no files.
	options->send_to_srv = 0;
	options->tcpport = NULL;
	options->tcp_password = NULL;
	options->tcp_desc = NULL;
	options->srv_addr = NULL;
	options->srv_port = NULL;
	options->line_terminator_lf=0; // 0 = CRLF
	options->noautotimeref=0; // Do NOT set time automatically?
	options->input_source=CCX_DS_FILE; // Files, stdin or network
	options->auto_stream = CCX_SM_AUTODETECT;
	options->m2ts = 0;

	// Prepare time structures
	init_boundary_time (&options->extraction_start);
	init_boundary_time (&options->extraction_end);
	init_boundary_time (&options->startcreditsnotbefore);
	init_boundary_time (&options->startcreditsnotafter);
	init_boundary_time (&options->startcreditsforatleast);
	init_boundary_time (&options->startcreditsforatmost);
	init_boundary_time (&options->endcreditsforatleast);
	init_boundary_time (&options->endcreditsforatmost);

}
