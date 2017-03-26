/* CCExtractor, originally by carlos at ccextractor.org, now a lot of people.
Credits: See CHANGES.TXT
License: GPL 2.0
*/

#ifdef ENABLE_OCR
#include <allheaders.h>
#endif
#include <stdio.h>
#include "lib_ccx.h"
#include "configuration.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "ccx_common_option.h"
#include "ccx_mp4.h"
#include "hardsubx.h"
#include "ccx_share.h"
#ifdef WITH_LIBCURL
CURL *curl;
CURLcode res;
#endif
#if defined(ENABLE_OCR) && defined(_WIN32)
#define LEPT_MSG_SEVERITY L_SEVERITY_NONE
#endif

struct lib_ccx_ctx *signal_ctx;

volatile int terminate_asap = 0;

void sigusr1_handler(int sig)
{
	mprint("Caught SIGUSR1. Filename Change Requested\n");
	change_filename_requested = 1;
}


void sigterm_handler(int sig)
{
	printf("Received SIGTERM, terminating as soon as possible.\n");
	terminate_asap = 1;
}


void sigint_handler(int sig)
{
	if (ccx_options.print_file_reports)
		print_file_report(signal_ctx);

	exit(EXIT_SUCCESS);
}


void print_end_msg()
{
	mprint("Issues? Open a ticket here\n");
	mprint("https://github.com/CCExtractor/ccextractor/issues\n");
}

/////////////////////////////////////////////////////////////////

struct ccx_s_options ccx_options;
int api_start(struct ccx_s_options api_options){
    struct lib_ccx_ctx *ctx;
	struct lib_cc_decode *dec_ctx = NULL;
	int ret = 0, tmp;
	enum ccx_stream_mode_enum stream_mode;

#if defined(ENABLE_OCR) && defined(_WIN32)
	setMsgSeverity(LEPT_MSG_SEVERITY);
#endif
#ifdef ENABLE_HARDSUBX
	if(api_options.hardsubx)
	{
		// Perform burned in subtitle extraction
		hardsubx(&api_options);
		return 0;
	}
#endif
	// Initialize CCExtractor libraries
	ctx = init_libraries(&api_options);
	if (!ctx && errno == ENOMEM)
		fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
	else if (!ctx && errno == EINVAL)
		fatal (CCX_COMMON_EXIT_BUG_BUG, "Invalid option to CCextractor Library\n");
	else if (!ctx && errno == EPERM)
		fatal (CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Unable to create output file: Operation not permitted.\n");
	else if (!ctx && errno == EACCES)
		fatal (CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Unable to create output file: Permission denied\n");
	else if (!ctx)
		fatal (EXIT_NOT_CLASSIFIED, "Unable to create Library Context %d\n",errno);

#ifdef WITH_LIBCURL
	curl_global_init(CURL_GLOBAL_ALL);

  	/* get a curl handle */
  	curl = curl_easy_init();
	if (!curl)
	{
		curl_global_cleanup(); // Must be done even on init fail
		fatal (EXIT_NOT_CLASSIFIED, "Unable to init curl.");
	}
#endif

	int show_myth_banner = 0;

	params_dump(ctx);

	// default teletext page
	if (tlt_config.page > 0) {
		// dec to BCD, magazine pages numbers are in BCD (ETSI 300 706)
		tlt_config.page = ((tlt_config.page / 100) << 8) | (((tlt_config.page / 10) % 10) << 4) | (tlt_config.page % 10);
	}

	if (api_options.transcript_settings.xds)
	{
		if (api_options.write_format != CCX_OF_TRANSCRIPT)
		{
			api_options.transcript_settings.xds = 0;
			mprint ("Warning: -xds ignored, XDS can only be exported to transcripts at this time.\n");
		}
	}


	time_t start, final;
	time(&start);

	if (api_options.binary_concat)
	{
		ctx->total_inputsize=get_total_file_size(ctx);
		if (ctx->total_inputsize < 0)
		{
			switch (ctx->total_inputsize)
			{
			case -1*ENOENT:
				fatal(EXIT_NO_INPUT_FILES, "Failed to open file: File does not exist.");
			case -1*EACCES:
				fatal(EXIT_READ_ERROR, "Failed to open file: Unable to access");
			case -1*EINVAL:
				fatal(EXIT_READ_ERROR, "Failed to open file: Invalid opening flag.");
			case -1*EMFILE:
				fatal(EXIT_TOO_MANY_INPUT_FILES, "Failed to open file: Too many files are open.");
			default:
				fatal(EXIT_READ_ERROR, "Failed to open file: Reason unknown");
			}
		}
	}

#ifndef _WIN32
	signal_ctx = ctx;
	m_signal(SIGINT, sigint_handler);
	m_signal(SIGTERM, sigterm_handler);
	m_signal(SIGUSR1, sigusr1_handler);
#endif
	terminate_asap = 0;

#ifdef ENABLE_SHARING
	if (api_options.translate_enabled && ctx->num_input_files > 1)
	{
		mprint("[share] WARNING: simultaneous translation of several input files is not supported yet\n");
		api_options.translate_enabled = 0;
	    api_options.sharing_enabled = 0;
	}
	if (api_options.translate_enabled)
	{
		mprint("[share] launching translate service\n");
		ccx_share_launch_translator(api_options.translate_langs, api_options.translate_key);
	}
#endif //ENABLE_SHARING
	ret = 0;
	while (switch_to_next_file(ctx, 0))
	{
		prepare_for_new_file(ctx);
#ifdef ENABLE_SHARING
		if (api_options.sharing_enabled)
			ccx_share_start(ctx->basefilename);
#endif //ENABLE_SHARING

		stream_mode = ctx->demux_ctx->get_stream_mode(ctx->demux_ctx);
		// Disable sync check for raw formats - they have the right timeline.
		// Also true for bin formats, but -nosync might have created a
		// broken timeline for debug purposes.
		// Disable too in MP4, specs doesn't say that there can't be a jump
		switch (stream_mode)
		{
			case CCX_SM_MCPOODLESRAW:
			case CCX_SM_RCWT:
			case CCX_SM_MP4:
#ifdef WTV_DEBUG
			case CCX_SM_HEX_DUMP:
#endif
				ccx_common_timing_settings.disable_sync_check = 1;
				break;
			default:
				break;
		}
		/* -----------------------------------------------------------------
		MAIN LOOP
		----------------------------------------------------------------- */
		switch (stream_mode)
		{
			case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
				if (!api_options.use_gop_as_pts) // If !0 then the user selected something
					api_options.use_gop_as_pts = 1; // Force GOP timing for ES
				ccx_common_timing_settings.is_elementary_stream = 1;
			case CCX_SM_TRANSPORT:
			case CCX_SM_PROGRAM:
			case CCX_SM_ASF:
			case CCX_SM_WTV:
			case CCX_SM_GXF:
#ifdef ENABLE_FFMPEG
			case CCX_SM_FFMPEG:
#endif
				if (!api_options.use_gop_as_pts) // If !0 then the user selected something
					api_options.use_gop_as_pts = 0;
				if (api_options.ignore_pts_jumps)
					ccx_common_timing_settings.disable_sync_check = 1;
				mprint ("\rAnalyzing data in general mode\n");
				tmp = general_loop(ctx);
				if (!ret) ret = tmp;
				break;
			case CCX_SM_MCPOODLESRAW:
				mprint ("\rAnalyzing data in McPoodle raw mode\n");
				tmp = raw_loop(ctx);
				if (!ret) ret = tmp;
				break;
			case CCX_SM_RCWT:
				mprint ("\rAnalyzing data in CCExtractor's binary format\n");
				tmp = rcwt_loop(ctx);
				if (!ret) ret = tmp;
				break;
			case CCX_SM_MYTH:
				mprint ("\rAnalyzing data in MythTV mode\n");
				show_myth_banner = 1;
				tmp = myth_loop(ctx);
				if (!ret) ret = tmp;
				break;
			case CCX_SM_MP4:
				mprint ("\rAnalyzing data with GPAC (MP4 library)\n");
				close_input_file(ctx); // No need to have it open. GPAC will do it for us
				if (ctx->current_file == -1) // We don't have a file to open, must be stdin, and GPAC is incompatible with stdin
				{
					fatal (EXIT_INCOMPATIBLE_PARAMETERS, "MP4 requires an actual file, it's not possible to read from a stream, including stdin.\n");
				}
				if(api_options.extract_chapters)
				{
					tmp = dumpchapters(ctx, &ctx->mp4_cfg, ctx->inputfile[ctx->current_file]);
				}
				else
				{
					tmp = processmp4(ctx, &ctx->mp4_cfg, ctx->inputfile[ctx->current_file]);
				}
				if (api_options.print_file_reports)
					print_file_report(ctx);
				if (!ret) ret = tmp;
				break;
            case CCX_SM_MKV:
                mprint ("\rAnalyzing data in Matroska mode\n");
				tmp = matroska_loop(ctx);
				if (!ret) ret = tmp;
                break;
#ifdef WTV_DEBUG
			case CCX_SM_HEX_DUMP:
				close_input_file(ctx); // process_hex will open it in text mode
				process_hex (ctx, ctx->inputfile[0]);
				break;
#endif
			case CCX_SM_AUTODETECT:
				fatal(CCX_COMMON_EXIT_BUG_BUG, "Cannot be reached!");
				break;
		}

		list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
		{
			mprint("\n");
			dbg_print(CCX_DMT_DECODER_608, "\nTime stamps after last caption block was written:\n");
			dbg_print(CCX_DMT_DECODER_608, "GOP: %s	  \n", print_mstime_static(gop_time.ms) );

			dbg_print(CCX_DMT_DECODER_608, "GOP: %s (%+3dms incl.)\n",
				print_mstime_static((LLONG)(gop_time.ms
				-first_gop_time.ms
				+get_fts_max(dec_ctx->timing)-fts_at_gop_start)),
				(int)(get_fts_max(dec_ctx->timing)-fts_at_gop_start));
			// When padding is active the CC block time should be within
			// 1000/29.97 us of the differences.
			dbg_print(CCX_DMT_DECODER_608, "Max. FTS:	   %s  (without caption blocks since then)\n",
				print_mstime_static(get_fts_max(dec_ctx->timing)));

			if (dec_ctx->codec == CCX_CODEC_ATSC_CC)
			{
				mprint ("\nTotal frames time:	  %s  (%u frames at %.2ffps)\n",
				print_mstime_static( (LLONG)(total_frames_count*1000/current_fps) ),
				total_frames_count, current_fps);
			}

			if (ctx->stat_hdtv)
			{
				mprint ("\rCC type 0: %d (%s)\n", dec_ctx->cc_stats[0], cc_types[0]);
				mprint ("CC type 1: %d (%s)\n", dec_ctx->cc_stats[1], cc_types[1]);
				mprint ("CC type 2: %d (%s)\n", dec_ctx->cc_stats[2], cc_types[2]);
				mprint ("CC type 3: %d (%s)\n", dec_ctx->cc_stats[3], cc_types[3]);
			}
			// Add one frame as fts_max marks the beginning of the last frame,
			// but we need the end.
			dec_ctx->timing->fts_global += dec_ctx->timing->fts_max + (LLONG) (1000.0/current_fps);
			// CFS: At least in Hauppage mode, cb_field can be responsible for ALL the
			// timing (cb_fields having a huge number and fts_now and fts_global being 0 all
			// the time), so we need to take that into account in fts_global before resetting
			// counters.
			if (cb_field1!=0)
				dec_ctx->timing->fts_global += cb_field1*1001/3;
			else if (cb_field2!=0)
				dec_ctx->timing->fts_global += cb_field2*1001/3;
			else
				dec_ctx->timing->fts_global += cb_708*1001/3;
			// Reset counters - This is needed if some captions are still buffered
			// and need to be written after the last file is processed.
			cb_field1 = 0; cb_field2 = 0; cb_708 = 0;
			dec_ctx->timing->fts_now = 0;
			dec_ctx->timing->fts_max = 0;

#ifdef ENABLE_SHARING
			if (api_options.sharing_enabled)
			{
				ccx_share_stream_done(ctx->basefilename);
				ccx_share_stop();
			}
#endif //ENABLE_SHARING

			if (dec_ctx->total_pulldownframes)
				mprint ("incl. pulldown frames:  %s  (%u frames at %.2ffps)\n",
						print_mstime_static( (LLONG)(dec_ctx->total_pulldownframes*1000/current_fps) ),
						dec_ctx->total_pulldownframes, current_fps);
			if (dec_ctx->timing->pts_set >= 1 && dec_ctx->timing->min_pts != 0x01FFFFFFFFLL)
			{
				LLONG postsyncms = (LLONG) (dec_ctx->frames_since_last_gop*1000/current_fps);
				mprint ("\nMin PTS:				%s\n",
						print_mstime_static( dec_ctx->timing->min_pts/(MPEG_CLOCK_FREQ/1000) - dec_ctx->timing->fts_offset));
				if (pts_big_change)
					mprint ("(Reference clock was reset at some point, Min PTS is approximated)\n");
				mprint ("Max PTS:				%s\n",
						print_mstime_static( dec_ctx->timing->sync_pts/(MPEG_CLOCK_FREQ/1000) + postsyncms));

				mprint ("Length:				 %s\n",
						print_mstime_static( dec_ctx->timing->sync_pts/(MPEG_CLOCK_FREQ/1000) + postsyncms
									  - dec_ctx->timing->min_pts/(MPEG_CLOCK_FREQ/1000) + dec_ctx->timing->fts_offset ));
			}


			// dvr-ms files have invalid GOPs
			if (gop_time.inited && first_gop_time.inited && stream_mode != CCX_SM_ASF)
			{
				mprint ("\nInitial GOP time:	   %s\n",
					print_mstime_static(first_gop_time.ms));
				mprint ("Final GOP time:		 %s%+3dF\n",
					print_mstime_static(gop_time.ms),
					dec_ctx->frames_since_last_gop);
				mprint ("Diff. GOP length:	   %s%+3dF",
					print_mstime_static(gop_time.ms - first_gop_time.ms),
					dec_ctx->frames_since_last_gop);
				mprint ("	(%s)\n\n",
					print_mstime_static(gop_time.ms - first_gop_time.ms
					+(LLONG) ((dec_ctx->frames_since_last_gop)*1000/29.97)) );
			}

			if (dec_ctx->false_pict_header)
				mprint ("\nNumber of likely false picture headers (discarded): %d\n",dec_ctx->false_pict_header);

			if (dec_ctx->stat_numuserheaders)
				mprint("\nTotal user data fields: %d\n", dec_ctx->stat_numuserheaders);
			if (dec_ctx->stat_dvdccheaders)
				mprint("DVD-type user data fields: %d\n", dec_ctx->stat_dvdccheaders);
			if (dec_ctx->stat_scte20ccheaders)
				mprint("SCTE-20 type user data fields: %d\n", dec_ctx->stat_scte20ccheaders);
			if (dec_ctx->stat_replay4000headers)
				mprint("ReplayTV 4000 user data fields: %d\n", dec_ctx->stat_replay4000headers);
			if (dec_ctx->stat_replay5000headers)
				mprint("ReplayTV 5000 user data fields: %d\n", dec_ctx->stat_replay5000headers);
			if (dec_ctx->stat_hdtv)
				mprint("HDTV type user data fields: %d\n", dec_ctx->stat_hdtv);
			if (dec_ctx->stat_dishheaders)
				mprint("Dish Network user data fields: %d\n", dec_ctx->stat_dishheaders);
			if (dec_ctx->stat_divicom)
			{
				mprint("CEA608/Divicom user data fields: %d\n", dec_ctx->stat_divicom);

				mprint("\n\nNOTE! The CEA 608 / Divicom standard encoding for closed\n");
				mprint("caption is not well understood!\n\n");
				mprint("Please submit samples to the developers.\n\n\n");
			}

		}

		if(is_decoder_processed_enough(ctx) == CCX_TRUE)
			break;
	} // file loop
	close_input_file(ctx);
	free((void *) ctx->extension);

	prepare_for_new_file (ctx); // To reset counters used by handle_end_of_data()


	time (&final);

	long proc_time=(long) (final-start);
	mprint ("\rDone, processing time = %ld seconds\n", proc_time);
#if 0
	if (proc_time>0)
	{
		LLONG ratio=(get_fts_max()/10)/proc_time;
		unsigned s1=(unsigned) (ratio/100);
		unsigned s2=(unsigned) (ratio%100);
		mprint ("Performance (real length/process time) = %u.%02u\n",
			s1, s2);
	}
#endif

	if (is_decoder_processed_enough(ctx) == CCX_TRUE)
	{
		mprint ("\rNote: Processing was canceled before all data was processed because\n");
		mprint ("\rone or more user-defined limits were reached.\n");
	}

#ifdef CURL
	if (curl)
		curl_easy_cleanup(curl);
  	curl_global_cleanup();
#endif
	dinit_libraries(&ctx);

	if (!ret)
		mprint("\nNo captions were found in input.\n");

	print_end_msg();

	if (show_myth_banner)
	{
		mprint("NOTICE: Due to the major rework in 0.49, we needed to change part of the timing\n");
		mprint("code in the MythTV's branch. Please report results to the address above. If\n");
		mprint("something is broken it will be fixed. Thanks\n");
	}
	return ret ? EXIT_OK : EXIT_NO_CAPTIONS;
}
struct ccx_s_options* api_init_options(){
    init_options(&ccx_options);
    return &ccx_options;
}
void checking_configuration_file(struct ccx_s_options api_options){
    parse_configuration(&api_options);
}
   

int compile_params(struct ccx_s_options *api_options,int argc){
	int ret = parse_parameters (api_options, argc, api_options->myarguments);
    if (ret == EXIT_NO_INPUT_FILES)
	{
		print_usage ();
		fatal (EXIT_NO_INPUT_FILES, "(This help screen was shown because there were no input files)\n");
	}
	else if (ret == EXIT_WITH_HELP)
	{
		return EXIT_OK;
	}
	else if (ret != EXIT_OK)
	{
		exit(ret);
	}
    return EXIT_OK;
}


void api_add_param(struct ccx_s_options* api_options,char* arg){
      api_options->myarguments = realloc(api_options->myarguments, (api_options->argument_count+1) * sizeof *api_options->myarguments);
      api_options->myarguments[api_options->argument_count] = malloc(strlen(arg)+1);
      strcpy(api_options->myarguments[api_options->argument_count], arg);
      api_options->argument_count++;
}
int main(int argc, char* argv[]){
    int ret,i,start_ret;
    struct ccx_s_options* api_options = api_init_options();
    checking_configuration_file(*api_options);
    for(i = 0; i < argc; i++) {
        api_add_param(api_options,argv[i]);
        }
    int compile_ret = compile_params(api_options,argc);
    printf("********************* %d *********************\n",api_options->argument_count);
    start_ret = api_start(*api_options);
    printf("Done compiling and extraction of subs");
    printf("********************* %d *********************\n",api_options->argument_count);

 return 1;
}


//////////////////////////////////////////////////////////////////
//struct ccx_s_options ccx_options;
int defined_main(int argc, char *argv[])
{
	struct lib_ccx_ctx *ctx;
	struct lib_cc_decode *dec_ctx = NULL;
	int ret = 0, tmp;
	enum ccx_stream_mode_enum stream_mode;

	init_options (&ccx_options);

	parse_configuration(&ccx_options);
	ret = parse_parameters (&ccx_options, argc, argv);
	if (ret == EXIT_NO_INPUT_FILES)
	{
		print_usage ();
		fatal (EXIT_NO_INPUT_FILES, "(This help screen was shown because there were no input files)\n");
	}
	else if (ret == EXIT_WITH_HELP)
	{
		return EXIT_OK;
	}
	else if (ret != EXIT_OK)
	{
		exit(ret);
	}
#if defined(ENABLE_OCR) && defined(_WIN32)
	setMsgSeverity(LEPT_MSG_SEVERITY);
#endif
#ifdef ENABLE_HARDSUBX
	if(ccx_options.hardsubx)
	{
		// Perform burned in subtitle extraction
		hardsubx(&ccx_options);
		return 0;
	}
#endif
	// Initialize CCExtractor libraries
	ctx = init_libraries(&ccx_options);
	if (!ctx && errno == ENOMEM)
		fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
	else if (!ctx && errno == EINVAL)
		fatal (CCX_COMMON_EXIT_BUG_BUG, "Invalid option to CCextractor Library\n");
	else if (!ctx && errno == EPERM)
		fatal (CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Unable to create output file: Operation not permitted.\n");
	else if (!ctx && errno == EACCES)
		fatal (CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Unable to create output file: Permission denied\n");
	else if (!ctx)
		fatal (EXIT_NOT_CLASSIFIED, "Unable to create Library Context %d\n",errno);

#ifdef WITH_LIBCURL
	curl_global_init(CURL_GLOBAL_ALL);

  	/* get a curl handle */
  	curl = curl_easy_init();
	if (!curl)
	{
		curl_global_cleanup(); // Must be done even on init fail
		fatal (EXIT_NOT_CLASSIFIED, "Unable to init curl.");
	}
#endif

	int show_myth_banner = 0;

	params_dump(ctx);

	// default teletext page
	if (tlt_config.page > 0) {
		// dec to BCD, magazine pages numbers are in BCD (ETSI 300 706)
		tlt_config.page = ((tlt_config.page / 100) << 8) | (((tlt_config.page / 10) % 10) << 4) | (tlt_config.page % 10);
	}

	if (ccx_options.transcript_settings.xds)
	{
		if (ccx_options.write_format != CCX_OF_TRANSCRIPT)
		{
			ccx_options.transcript_settings.xds = 0;
			mprint ("Warning: -xds ignored, XDS can only be exported to transcripts at this time.\n");
		}
	}


	time_t start, final;
	time(&start);

	if (ccx_options.binary_concat)
	{
		ctx->total_inputsize=get_total_file_size(ctx);
		if (ctx->total_inputsize < 0)
		{
			switch (ctx->total_inputsize)
			{
			case -1*ENOENT:
				fatal(EXIT_NO_INPUT_FILES, "Failed to open file: File does not exist.");
			case -1*EACCES:
				fatal(EXIT_READ_ERROR, "Failed to open file: Unable to access");
			case -1*EINVAL:
				fatal(EXIT_READ_ERROR, "Failed to open file: Invalid opening flag.");
			case -1*EMFILE:
				fatal(EXIT_TOO_MANY_INPUT_FILES, "Failed to open file: Too many files are open.");
			default:
				fatal(EXIT_READ_ERROR, "Failed to open file: Reason unknown");
			}
		}
	}

#ifndef _WIN32
	signal_ctx = ctx;
	m_signal(SIGINT, sigint_handler);
	m_signal(SIGTERM, sigterm_handler);
	m_signal(SIGUSR1, sigusr1_handler);
#endif
	terminate_asap = 0;

#ifdef ENABLE_SHARING
	if (ccx_options.translate_enabled && ctx->num_input_files > 1)
	{
		mprint("[share] WARNING: simultaneous translation of several input files is not supported yet\n");
		ccx_options.translate_enabled = 0;
		ccx_options.sharing_enabled = 0;
	}
	if (ccx_options.translate_enabled)
	{
		mprint("[share] launching translate service\n");
		ccx_share_launch_translator(ccx_options.translate_langs, ccx_options.translate_key);
	}
#endif //ENABLE_SHARING
	ret = 0;
	while (switch_to_next_file(ctx, 0))
	{
		prepare_for_new_file(ctx);
#ifdef ENABLE_SHARING
		if (ccx_options.sharing_enabled)
			ccx_share_start(ctx->basefilename);
#endif //ENABLE_SHARING

		stream_mode = ctx->demux_ctx->get_stream_mode(ctx->demux_ctx);
		// Disable sync check for raw formats - they have the right timeline.
		// Also true for bin formats, but -nosync might have created a
		// broken timeline for debug purposes.
		// Disable too in MP4, specs doesn't say that there can't be a jump
		switch (stream_mode)
		{
			case CCX_SM_MCPOODLESRAW:
			case CCX_SM_RCWT:
			case CCX_SM_MP4:
#ifdef WTV_DEBUG
			case CCX_SM_HEX_DUMP:
#endif
				ccx_common_timing_settings.disable_sync_check = 1;
				break;
			default:
				break;
		}
		/* -----------------------------------------------------------------
		MAIN LOOP
		----------------------------------------------------------------- */
		switch (stream_mode)
		{
			case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
				if (!ccx_options.use_gop_as_pts) // If !0 then the user selected something
					ccx_options.use_gop_as_pts = 1; // Force GOP timing for ES
				ccx_common_timing_settings.is_elementary_stream = 1;
			case CCX_SM_TRANSPORT:
			case CCX_SM_PROGRAM:
			case CCX_SM_ASF:
			case CCX_SM_WTV:
			case CCX_SM_GXF:
#ifdef ENABLE_FFMPEG
			case CCX_SM_FFMPEG:
#endif
				if (!ccx_options.use_gop_as_pts) // If !0 then the user selected something
					ccx_options.use_gop_as_pts = 0;
				if (ccx_options.ignore_pts_jumps)
					ccx_common_timing_settings.disable_sync_check = 1;
				mprint ("\rAnalyzing data in general mode\n");
				tmp = general_loop(ctx);
				if (!ret) ret = tmp;
				break;
			case CCX_SM_MCPOODLESRAW:
				mprint ("\rAnalyzing data in McPoodle raw mode\n");
				tmp = raw_loop(ctx);
				if (!ret) ret = tmp;
				break;
			case CCX_SM_RCWT:
				mprint ("\rAnalyzing data in CCExtractor's binary format\n");
				tmp = rcwt_loop(ctx);
				if (!ret) ret = tmp;
				break;
			case CCX_SM_MYTH:
				mprint ("\rAnalyzing data in MythTV mode\n");
				show_myth_banner = 1;
				tmp = myth_loop(ctx);
				if (!ret) ret = tmp;
				break;
			case CCX_SM_MP4:
				mprint ("\rAnalyzing data with GPAC (MP4 library)\n");
				close_input_file(ctx); // No need to have it open. GPAC will do it for us
				if (ctx->current_file == -1) // We don't have a file to open, must be stdin, and GPAC is incompatible with stdin
				{
					fatal (EXIT_INCOMPATIBLE_PARAMETERS, "MP4 requires an actual file, it's not possible to read from a stream, including stdin.\n");
				}
				if(ccx_options.extract_chapters)
				{
					tmp = dumpchapters(ctx, &ctx->mp4_cfg, ctx->inputfile[ctx->current_file]);
				}
				else
				{
					tmp = processmp4(ctx, &ctx->mp4_cfg, ctx->inputfile[ctx->current_file]);
				}
				if (ccx_options.print_file_reports)
					print_file_report(ctx);
				if (!ret) ret = tmp;
				break;
            case CCX_SM_MKV:
                mprint ("\rAnalyzing data in Matroska mode\n");
				tmp = matroska_loop(ctx);
				if (!ret) ret = tmp;
                break;
#ifdef WTV_DEBUG
			case CCX_SM_HEX_DUMP:
				close_input_file(ctx); // process_hex will open it in text mode
				process_hex (ctx, ctx->inputfile[0]);
				break;
#endif
			case CCX_SM_AUTODETECT:
				fatal(CCX_COMMON_EXIT_BUG_BUG, "Cannot be reached!");
				break;
		}

		list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
		{
			mprint("\n");
			dbg_print(CCX_DMT_DECODER_608, "\nTime stamps after last caption block was written:\n");
			dbg_print(CCX_DMT_DECODER_608, "GOP: %s	  \n", print_mstime_static(gop_time.ms) );

			dbg_print(CCX_DMT_DECODER_608, "GOP: %s (%+3dms incl.)\n",
				print_mstime_static((LLONG)(gop_time.ms
				-first_gop_time.ms
				+get_fts_max(dec_ctx->timing)-fts_at_gop_start)),
				(int)(get_fts_max(dec_ctx->timing)-fts_at_gop_start));
			// When padding is active the CC block time should be within
			// 1000/29.97 us of the differences.
			dbg_print(CCX_DMT_DECODER_608, "Max. FTS:	   %s  (without caption blocks since then)\n",
				print_mstime_static(get_fts_max(dec_ctx->timing)));

			if (dec_ctx->codec == CCX_CODEC_ATSC_CC)
			{
				mprint ("\nTotal frames time:	  %s  (%u frames at %.2ffps)\n",
				print_mstime_static( (LLONG)(total_frames_count*1000/current_fps) ),
				total_frames_count, current_fps);
			}

			if (ctx->stat_hdtv)
			{
				mprint ("\rCC type 0: %d (%s)\n", dec_ctx->cc_stats[0], cc_types[0]);
				mprint ("CC type 1: %d (%s)\n", dec_ctx->cc_stats[1], cc_types[1]);
				mprint ("CC type 2: %d (%s)\n", dec_ctx->cc_stats[2], cc_types[2]);
				mprint ("CC type 3: %d (%s)\n", dec_ctx->cc_stats[3], cc_types[3]);
			}
			// Add one frame as fts_max marks the beginning of the last frame,
			// but we need the end.
			dec_ctx->timing->fts_global += dec_ctx->timing->fts_max + (LLONG) (1000.0/current_fps);
			// CFS: At least in Hauppage mode, cb_field can be responsible for ALL the
			// timing (cb_fields having a huge number and fts_now and fts_global being 0 all
			// the time), so we need to take that into account in fts_global before resetting
			// counters.
			if (cb_field1!=0)
				dec_ctx->timing->fts_global += cb_field1*1001/3;
			else if (cb_field2!=0)
				dec_ctx->timing->fts_global += cb_field2*1001/3;
			else
				dec_ctx->timing->fts_global += cb_708*1001/3;
			// Reset counters - This is needed if some captions are still buffered
			// and need to be written after the last file is processed.
			cb_field1 = 0; cb_field2 = 0; cb_708 = 0;
			dec_ctx->timing->fts_now = 0;
			dec_ctx->timing->fts_max = 0;

#ifdef ENABLE_SHARING
			if (ccx_options.sharing_enabled)
			{
				ccx_share_stream_done(ctx->basefilename);
				ccx_share_stop();
			}
#endif //ENABLE_SHARING

			if (dec_ctx->total_pulldownframes)
				mprint ("incl. pulldown frames:  %s  (%u frames at %.2ffps)\n",
						print_mstime_static( (LLONG)(dec_ctx->total_pulldownframes*1000/current_fps) ),
						dec_ctx->total_pulldownframes, current_fps);
			if (dec_ctx->timing->pts_set >= 1 && dec_ctx->timing->min_pts != 0x01FFFFFFFFLL)
			{
				LLONG postsyncms = (LLONG) (dec_ctx->frames_since_last_gop*1000/current_fps);
				mprint ("\nMin PTS:				%s\n",
						print_mstime_static( dec_ctx->timing->min_pts/(MPEG_CLOCK_FREQ/1000) - dec_ctx->timing->fts_offset));
				if (pts_big_change)
					mprint ("(Reference clock was reset at some point, Min PTS is approximated)\n");
				mprint ("Max PTS:				%s\n",
						print_mstime_static( dec_ctx->timing->sync_pts/(MPEG_CLOCK_FREQ/1000) + postsyncms));

				mprint ("Length:				 %s\n",
						print_mstime_static( dec_ctx->timing->sync_pts/(MPEG_CLOCK_FREQ/1000) + postsyncms
									  - dec_ctx->timing->min_pts/(MPEG_CLOCK_FREQ/1000) + dec_ctx->timing->fts_offset ));
			}


			// dvr-ms files have invalid GOPs
			if (gop_time.inited && first_gop_time.inited && stream_mode != CCX_SM_ASF)
			{
				mprint ("\nInitial GOP time:	   %s\n",
					print_mstime_static(first_gop_time.ms));
				mprint ("Final GOP time:		 %s%+3dF\n",
					print_mstime_static(gop_time.ms),
					dec_ctx->frames_since_last_gop);
				mprint ("Diff. GOP length:	   %s%+3dF",
					print_mstime_static(gop_time.ms - first_gop_time.ms),
					dec_ctx->frames_since_last_gop);
				mprint ("	(%s)\n\n",
					print_mstime_static(gop_time.ms - first_gop_time.ms
					+(LLONG) ((dec_ctx->frames_since_last_gop)*1000/29.97)) );
			}

			if (dec_ctx->false_pict_header)
				mprint ("\nNumber of likely false picture headers (discarded): %d\n",dec_ctx->false_pict_header);

			if (dec_ctx->stat_numuserheaders)
				mprint("\nTotal user data fields: %d\n", dec_ctx->stat_numuserheaders);
			if (dec_ctx->stat_dvdccheaders)
				mprint("DVD-type user data fields: %d\n", dec_ctx->stat_dvdccheaders);
			if (dec_ctx->stat_scte20ccheaders)
				mprint("SCTE-20 type user data fields: %d\n", dec_ctx->stat_scte20ccheaders);
			if (dec_ctx->stat_replay4000headers)
				mprint("ReplayTV 4000 user data fields: %d\n", dec_ctx->stat_replay4000headers);
			if (dec_ctx->stat_replay5000headers)
				mprint("ReplayTV 5000 user data fields: %d\n", dec_ctx->stat_replay5000headers);
			if (dec_ctx->stat_hdtv)
				mprint("HDTV type user data fields: %d\n", dec_ctx->stat_hdtv);
			if (dec_ctx->stat_dishheaders)
				mprint("Dish Network user data fields: %d\n", dec_ctx->stat_dishheaders);
			if (dec_ctx->stat_divicom)
			{
				mprint("CEA608/Divicom user data fields: %d\n", dec_ctx->stat_divicom);

				mprint("\n\nNOTE! The CEA 608 / Divicom standard encoding for closed\n");
				mprint("caption is not well understood!\n\n");
				mprint("Please submit samples to the developers.\n\n\n");
			}

		}

		if(is_decoder_processed_enough(ctx) == CCX_TRUE)
			break;
	} // file loop
	close_input_file(ctx);
	free((void *) ctx->extension);

	prepare_for_new_file (ctx); // To reset counters used by handle_end_of_data()


	time (&final);

	long proc_time=(long) (final-start);
	mprint ("\rDone, processing time = %ld seconds\n", proc_time);
#if 0
	if (proc_time>0)
	{
		LLONG ratio=(get_fts_max()/10)/proc_time;
		unsigned s1=(unsigned) (ratio/100);
		unsigned s2=(unsigned) (ratio%100);
		mprint ("Performance (real length/process time) = %u.%02u\n",
			s1, s2);
	}
#endif

	if (is_decoder_processed_enough(ctx) == CCX_TRUE)
	{
		mprint ("\rNote: Processing was canceled before all data was processed because\n");
		mprint ("\rone or more user-defined limits were reached.\n");
	}

#ifdef CURL
	if (curl)
		curl_easy_cleanup(curl);
  	curl_global_cleanup();
#endif
	dinit_libraries(&ctx);

	if (!ret)
		mprint("\nNo captions were found in input.\n");

	print_end_msg();

	if (show_myth_banner)
	{
		mprint("NOTICE: Due to the major rework in 0.49, we needed to change part of the timing\n");
		mprint("code in the MythTV's branch. Please report results to the address above. If\n");
		mprint("something is broken it will be fixed. Thanks\n");
	}
	return ret ? EXIT_OK : EXIT_NO_CAPTIONS;
}
