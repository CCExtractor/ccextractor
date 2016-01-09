/* CCExtractor, carlos at ccextractor org
Credits: See CHANGES.TXT
License: GPL 2.0
*/
#include <stdio.h>
#include "lib_ccx.h"
#include "configuration.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "ffmpeg_intgr.h"
#include "ccx_common_option.h"
#include "ccx_mp4.h"

struct lib_ccx_ctx *signal_ctx;
void sigint_handler()
{
	if (ccx_options.print_file_reports)
		print_file_report(signal_ctx);

	exit(EXIT_SUCCESS);
}

struct ccx_s_options ccx_options;
int main(int argc, char *argv[])
{
#ifdef ENABLE_FFMPEG
	void *ffmpeg_ctx = NULL;
#endif
	struct lib_ccx_ctx *ctx;
	struct lib_cc_decode *dec_ctx = NULL;
	int ret = 0;
	enum ccx_stream_mode_enum stream_mode;

	init_options (&ccx_options);

	parse_configuration(&ccx_options);
	ret = parse_parameters (&ccx_options, argc, argv);
	if (ret == EXIT_NO_INPUT_FILES)
	{
		usage ();
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
	// Initialize libraries
	ctx = init_libraries(&ccx_options);
	if (!ctx && errno == ENOMEM)
		fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
	else if (!ctx && errno == EINVAL)
		fatal (CCX_COMMON_EXIT_BUG_BUG, "Invalid option to CCextractor Library\n");
	else if (!ctx && errno == EPERM)
		fatal (CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Unable to create Output File\n");
	else if (!ctx && errno == EACCES)
		fatal (CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Unable to create Output File\n");
	else if (!ctx)
		fatal (EXIT_NOT_CLASSIFIED, "Unable to create Library Context %d\n",errno);

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
		ctx->total_inputsize=gettotalfilessize(ctx);
		if (ctx->total_inputsize==-1)
			fatal (EXIT_UNABLE_TO_DETERMINE_FILE_SIZE, "Failed to determine total file size.\n");
	}

#ifndef _WIN32
	signal_ctx = ctx;
	m_signal(SIGINT, sigint_handler);
#endif

	while (switch_to_next_file(ctx, 0))
	{
		prepare_for_new_file(ctx);
#ifdef ENABLE_FFMPEG
		close_input_file(ctx);
		ffmpeg_ctx =  init_ffmpeg(ctx->inputfile[0]);
		if(ffmpeg_ctx)
		{
			do
			{
				int ret = 0;
				unsigned char *bptr = ctx->buffer;
				int len = ff_get_ccframe(ffmpeg_ctx, bptr, 1024);
                                int cc_count = 0;
				if(len == AVERROR(EAGAIN))
				{
					continue;
				}
				else if(len == AVERROR_EOF)
					break;
				else if(len == 0)
					continue;
				else if(len < 0 )
				{
					mprint("Error extracting Frame\n");
					break;

				}
                                else
                                    cc_count = len/3;
				ret = process_cc_data(dec_ctx, bptr, cc_count, &dec_ctx->dec_sub);
				if(ret >= 0 && dec_ctx->dec_sub.got_output)
				{
					encode_sub(enc_ctx, &dec_ctx->dec_sub);
					dec_ctx->dec_sub.got_output = 0;
				}
			}while(1);
			continue;
		}
		else
		{
			mprint ("\rFailed to initialized ffmpeg falling back to legacy\n");
		}
#endif
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
				if (!ccx_options.use_gop_as_pts) // If !0 then the user selected something
					ccx_options.use_gop_as_pts = 0; 
				mprint ("\rAnalyzing data in general mode\n");
				general_loop(ctx);
				break;
			case CCX_SM_MCPOODLESRAW:
				mprint ("\rAnalyzing data in McPoodle raw mode\n");
				raw_loop(ctx);
				break;
			case CCX_SM_RCWT:
				mprint ("\rAnalyzing data in CCExtractor's binary format\n");
				rcwt_loop(ctx);
				break;
			case CCX_SM_MYTH:
				mprint ("\rAnalyzing data in MythTV mode\n");
				show_myth_banner = 1;
				myth_loop(ctx);
				break;
			case CCX_SM_MP4:
				mprint ("\rAnalyzing data with GPAC (MP4 library)\n");
				close_input_file(ctx); // No need to have it open. GPAC will do it for us
				processmp4 (ctx, &ctx->mp4_cfg, ctx->inputfile[0]);
				if (ccx_options.print_file_reports)
					print_file_report(ctx);
				break;
#ifdef WTV_DEBUG
			case CCX_SM_HEX_DUMP:
				close_input_file(ctx); // processhex will open it in text mode
				processhex (ctx, ctx->inputfile[0]);
				break;
#endif
			case CCX_SM_AUTODETECT:
				fatal(CCX_COMMON_EXIT_BUG_BUG, "Cannot be reached!");
				break;
		}

#if 0
		if (ctx->total_pulldownframes)
			mprint ("incl. pulldown frames:  %s  (%u frames at %.2ffps)\n",
					print_mstime( (LLONG)(ctx->total_pulldownframes*1000/current_fps) ),
					ctx->total_pulldownframes, current_fps);
		if (pts_set >= 1 && min_pts != 0x01FFFFFFFFLL)
		{
			LLONG postsyncms = (LLONG) (ctx->frames_since_last_gop*1000/current_fps);
			mprint ("\nMin PTS:				%s\n",
					print_mstime( min_pts/(MPEG_CLOCK_FREQ/1000) - fts_offset));
			if (pts_big_change)
				mprint ("(Reference clock was reset at some point, Min PTS is approximated)\n");
			mprint ("Max PTS:				%s\n",
					print_mstime( sync_pts/(MPEG_CLOCK_FREQ/1000) + postsyncms));

			mprint ("Length:				 %s\n",
					print_mstime( sync_pts/(MPEG_CLOCK_FREQ/1000) + postsyncms
								  - min_pts/(MPEG_CLOCK_FREQ/1000) + fts_offset ));
		}
		// dvr-ms files have invalid GOPs
		if (gop_time.inited && first_gop_time.inited && stream_mode != CCX_SM_ASF)
		{
			mprint ("\nInitial GOP time:	   %s\n",
				print_mstime(first_gop_time.ms));
			mprint ("Final GOP time:		 %s%+3dF\n",
				print_mstime(gop_time.ms),
				ctx->frames_since_last_gop);
			mprint ("Diff. GOP length:	   %s%+3dF",
				print_mstime(gop_time.ms - first_gop_time.ms),
				ctx->frames_since_last_gop);
			mprint ("	(%s)\n",
				print_mstime(gop_time.ms - first_gop_time.ms
				+(LLONG) ((ctx->frames_since_last_gop)*1000/29.97)) );
		}

		if (ctx->false_pict_header)
			mprint ("\nNumber of likely false picture headers (discarded): %d\n",ctx->false_pict_header);

		if (ctx->stat_numuserheaders)
			mprint("\nTotal user data fields: %d\n", ctx->stat_numuserheaders);
		if (ctx->stat_dvdccheaders)
			mprint("DVD-type user data fields: %d\n", ctx->stat_dvdccheaders);
		if (ctx->stat_scte20ccheaders)
			mprint("SCTE-20 type user data fields: %d\n", ctx->stat_scte20ccheaders);
		if (ctx->stat_replay4000headers)
			mprint("ReplayTV 4000 user data fields: %d\n", ctx->stat_replay4000headers);
		if (ctx->stat_replay5000headers)
			mprint("ReplayTV 5000 user data fields: %d\n", ctx->stat_replay5000headers);
		if (ctx->stat_hdtv)
			mprint("HDTV type user data fields: %d\n", ctx->stat_hdtv);
		if (ctx->stat_dishheaders)
			mprint("Dish Network user data fields: %d\n", ctx->stat_dishheaders);
		if (ctx->stat_divicom)
		{
			mprint("CEA608/Divicom user data fields: %d\n", ctx->stat_divicom);

			mprint("\n\nNOTE! The CEA 608 / Divicom standard encoding for closed\n");
			mprint("caption is not well understood!\n\n");
			mprint("Please submit samples to the developers.\n\n\n");
		}
#endif

		list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
		{
			mprint("\n");
			dbg_print(CCX_DMT_DECODER_608, "\nTime stamps after last caption block was written:\n");
			dbg_print(CCX_DMT_DECODER_608, "GOP: %s	  \n", print_mstime(gop_time.ms) );
	
			dbg_print(CCX_DMT_DECODER_608, "GOP: %s (%+3dms incl.)\n",
				print_mstime((LLONG)(gop_time.ms
				-first_gop_time.ms
				+get_fts_max(dec_ctx->timing)-fts_at_gop_start)),
				(int)(get_fts_max(dec_ctx->timing)-fts_at_gop_start));
			// When padding is active the CC block time should be within
			// 1000/29.97 us of the differences.
			dbg_print(CCX_DMT_DECODER_608, "Max. FTS:	   %s  (without caption blocks since then)\n",
				print_mstime(get_fts_max(dec_ctx->timing)));

			if (dec_ctx->codec == CCX_CODEC_ATSC_CC)
			{
				mprint ("\nTotal frames time:	  %s  (%u frames at %.2ffps)\n",
				print_mstime( (LLONG)(total_frames_count*1000/current_fps) ),
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
		}

		if(is_decoder_processed_enough(ctx) == CCX_TRUE)
			break;
	} // file loop
	close_input_file(ctx);

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
	dbg_print(CCX_DMT_708, "[CEA-708] The 708 decoder was reset [%d] times.\n", ctx->freport.data_from_708->reset_count);

	if (is_decoder_processed_enough(ctx) == CCX_TRUE)
	{
		mprint ("\rNote: Processing was cancelled before all data was processed because\n");
		mprint ("\rone or more user-defined limits were reached.\n");
	} 
	mprint ("This is beta software. Report issues to carlos at ccextractor org...\n");
	if (show_myth_banner)
	{
		mprint ("NOTICE: Due to the major rework in 0.49, we needed to change part of the timing\n");
		mprint ("code in the MythTV's branch. Please report results to the address above. If\n");
		mprint ("something is broken it will be fixed. Thanks\n");		
	}
	dinit_libraries(&ctx);
	return EXIT_OK;
}
