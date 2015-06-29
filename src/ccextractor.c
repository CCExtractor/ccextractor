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

#ifdef ENABLE_SHARING
#include "ccx_share.h"
#endif //ENABLE_SHARING

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
	struct encoder_ctx enc_ctx[2];
	struct cc_subtitle dec_sub;
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

	dec_ctx = ctx->dec_ctx;



	int show_myth_banner = 0;
	
	memset (&cea708services[0],0,CCX_DECODERS_708_MAX_SERVICES*sizeof (int)); // Cannot (yet) be moved because it's needed in parse_parameters.
	memset (&dec_sub, 0,sizeof(dec_sub));


	params_dump(ctx);

	// default teletext page
	if (tlt_config.page > 0) {
		// dec to BCD, magazine pages numbers are in BCD (ETSI 300 706)
		tlt_config.page = ((tlt_config.page / 100) << 8) | (((tlt_config.page / 10) % 10) << 4) | (tlt_config.page % 10);
	}

	subline = (unsigned char *) malloc (SUBLINESIZE);

	if (ctx->pesheaderbuf==NULL ||
		subline==NULL || init_file_buffer() )
	{
		fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");		
	}

	if (ccx_options.send_to_srv)
	{
		connect_to_srv(ccx_options.srv_addr, ccx_options.srv_port, ccx_options.tcp_desc);
	}

	if (ccx_options.write_format!=CCX_OF_NULL)
	{
		/* # DVD format uses one raw file for both fields, while Broadcast requires 2 */
		if (ccx_options.write_format==CCX_OF_DVDRAW)
		{
			if (ctx->cc_to_stdout)
			{
				ctx->wbout1.fh=STDOUT_FILENO;
				mprint ("Sending captions to stdout.\n");
			}
			else
			{
				mprint ("Creating %s\n", ctx->wbout1.filename);
				if (detect_input_file_overwrite(ctx, ctx->wbout1.filename)) {
					fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED,
						  "Output filename is same as one of input filenames. Check output parameters.\n");
				}
			}
			if (init_encoder(enc_ctx, &ctx->wbout1, &ccx_options))
				fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
			set_encoder_subs_delay(enc_ctx, ctx->subs_delay);
			set_encoder_last_displayed_subs_ms(enc_ctx, ctx->last_displayed_subs_ms);
			set_encoder_startcredits_displayed(enc_ctx, ctx->startcredits_displayed);
		}
		else
		{
			if (ctx->cc_to_stdout && ccx_options.extract==12)
				fatal (EXIT_INCOMPATIBLE_PARAMETERS, "You can't extract both fields to stdout at the same time in broadcast mode.");
			
			if (ccx_options.write_format == CCX_OF_SPUPNG && ctx->cc_to_stdout)
				fatal (EXIT_INCOMPATIBLE_PARAMETERS, "You cannot use -out=spupng with -stdout.");

			if (ccx_options.extract!=2)
			{
				if (ctx->cc_to_stdout)
				{
					ctx->wbout1.fh=STDOUT_FILENO;
					mprint ("Sending captions to stdout.\n");
				}
				else if (!ccx_options.send_to_srv)
				{
					if (ctx->wbout1.filename[0]==0)
					{
						strcpy (ctx->wbout1.filename,ctx->basefilename);
						if (ccx_options.extract==12) // _1 only added if there's two files
							strcat (ctx->wbout1.filename,"_1");
						strcat (ctx->wbout1.filename,(const char *) ctx->extension);
					}
					mprint ("Creating %s\n", ctx->wbout1.filename);
					if (detect_input_file_overwrite(ctx, ctx->wbout1.filename)) {
						fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED,
							  "Output filename is same as one of input filenames. Check output parameters.\n");
					}
					ctx->wbout1.fh=open (ctx->wbout1.filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
					if (ctx->wbout1.fh==-1)
					{
						fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Failed (errno=%d)\n", errno);
					}
				}
				if (init_encoder(enc_ctx, &ctx->wbout1, &ccx_options))
					fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
				set_encoder_subs_delay(enc_ctx, ctx->subs_delay);
				set_encoder_last_displayed_subs_ms(enc_ctx, ctx->last_displayed_subs_ms);
				set_encoder_startcredits_displayed(enc_ctx, ctx->startcredits_displayed);
			}
			if (ccx_options.extract == 12 && ccx_options.write_format != CCX_OF_RAW)
				mprint (" and \n");
			if (ccx_options.extract!=1)
			{
				if (ctx->cc_to_stdout)
				{
					ctx->wbout1.fh=STDOUT_FILENO;
					mprint ("Sending captions to stdout.\n");
				}
				else if(ccx_options.write_format == CCX_OF_RAW
					&& ccx_options.extract == 12)
				{
					memcpy(&ctx->wbout2, &ctx->wbout1,sizeof(ctx->wbout1));
				}
				else if (!ccx_options.send_to_srv)
				{
					if (ctx->wbout2.filename[0]==0)
					{
						strcpy (ctx->wbout2.filename,ctx->basefilename);
						if (ccx_options.extract==12) // _ only added if there's two files
							strcat (ctx->wbout2.filename,"_2");
						strcat (ctx->wbout2.filename,(const char *) ctx->extension);
					}
					mprint ("Creating %s\n", ctx->wbout2.filename);
					if (detect_input_file_overwrite(ctx, ctx->wbout2.filename)) {
						fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED,
							  "Output filename is same as one of input filenames. Check output parameters.\n");
					}
					ctx->wbout2.fh=open (ctx->wbout2.filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
					if (ctx->wbout2.fh==-1)
					{
						fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Failed\n");
					}
					if(ccx_options.write_format == CCX_OF_RAW)
						dec_ctx->writedata (BROADCAST_HEADER,sizeof (BROADCAST_HEADER), dec_ctx->context_cc608_field_2, NULL);
				}

				if( init_encoder(enc_ctx+1, &ctx->wbout2, &ccx_options) )
					fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
				set_encoder_subs_delay(enc_ctx+1, ctx->subs_delay);
				set_encoder_last_displayed_subs_ms(enc_ctx+1, ctx->last_displayed_subs_ms);
				set_encoder_startcredits_displayed(enc_ctx+1, ctx->startcredits_displayed);
			}
		}
	}

	if (ccx_options.transcript_settings.xds)
	{
		if (ccx_options.write_format != CCX_OF_TRANSCRIPT)
		{
			ccx_options.transcript_settings.xds = 0;
			mprint ("Warning: -xds ignored, XDS can only be exported to transcripts at this time.\n");
		}
	}

	if (ccx_options.teletext_mode == CCX_TXT_IN_USE) // Here, it would mean it was forced by user
		telxcc_init(ctx);

	// Initialize HDTV caption buffer
	init_hdcc();

	if (ccx_options.line_terminator_lf)
		encoded_crlf_length = encode_line(encoded_crlf, (unsigned char *) "\n");
	else
		encoded_crlf_length = encode_line(encoded_crlf, (unsigned char *) "\r\n");

	encoded_br_length = encode_line(encoded_br, (unsigned char *) "<br>");
	

	time_t start, final;
	time(&start);

	dec_ctx->processed_enough=0;
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
#ifdef ENABLE_SHARING
	if (ccx_options.translate_enabled && ctx->num_input_files > 1) {
		mprint("[share] WARNING: simultaneous translation of several input files is not supported yet\n");
		ccx_options.translate_enabled = 0;
		ccx_options.sharing_enabled = 0;
	}
	if (ccx_options.translate_enabled) {
		mprint("[share] launching translate service\n");
		ccx_share_launch_translator(ccx_options.translate_langs, ccx_options.translate_key);
	}
#endif //ENABLE_SHARING
	while (switch_to_next_file(ctx, 0) && !dec_ctx->processed_enough)
	{
		prepare_for_new_file(ctx);
#ifdef ENABLE_SHARING
		if (ccx_options.sharing_enabled) {
			ccx_share_start(ctx->basefilename);
		}
#endif //ENABLE_SHARING
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
				ret = process_cc_data(dec_ctx, bptr, cc_count, &dec_sub);
				if(ret >= 0 && dec_sub.got_output)
				{
					encode_sub(enc_ctx, &dec_sub);
					dec_sub.got_output = 0;
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
			struct ccx_s_mp4Cfg mp4_cfg = {ccx_options.mp4vidtrack};
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
				general_loop(ctx, &enc_ctx);
				break;
			case CCX_SM_MCPOODLESRAW:
				mprint ("\rAnalyzing data in McPoodle raw mode\n");
				raw_loop(ctx, &enc_ctx);
				break;
			case CCX_SM_RCWT:
				mprint ("\rAnalyzing data in CCExtractor's binary format\n");
				rcwt_loop(ctx, &enc_ctx);
				break;
			case CCX_SM_MYTH:
				mprint ("\rAnalyzing data in MythTV mode\n");
				show_myth_banner = 1;
				myth_loop(ctx, &enc_ctx);
				break;
			case CCX_SM_MP4:
				mprint ("\rAnalyzing data with GPAC (MP4 library)\n");
				close_input_file(ctx); // No need to have it open. GPAC will do it for us
				processmp4 (ctx, &mp4_cfg, ctx->inputfile[0],&enc_ctx);
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

		mprint("\n");
		dbg_print(CCX_DMT_DECODER_608, "\nTime stamps after last caption block was written:\n");
		dbg_print(CCX_DMT_DECODER_608, "Last time stamps:  PTS: %s (%+2dF)		",
			   print_mstime( (LLONG) (sync_pts/(MPEG_CLOCK_FREQ/1000)
								   +frames_since_ref_time*1000.0/current_fps) ),
			   frames_since_ref_time);
		dbg_print(CCX_DMT_DECODER_608, "GOP: %s	  \n", print_mstime(gop_time.ms) );

		// Blocks since last PTS/GOP time stamp.
		dbg_print(CCX_DMT_DECODER_608, "Calc. difference:  PTS: %s (%+3lldms incl.)  ",
			print_mstime( (LLONG) ((sync_pts-min_pts)/(MPEG_CLOCK_FREQ/1000)
			+ fts_offset + frames_since_ref_time*1000.0/current_fps)),
			fts_offset + (LLONG) (frames_since_ref_time*1000.0/current_fps) );
		dbg_print(CCX_DMT_DECODER_608, "GOP: %s (%+3dms incl.)\n",
			print_mstime((LLONG)(gop_time.ms
			-first_gop_time.ms
			+get_fts_max()-fts_at_gop_start)),
			(int)(get_fts_max()-fts_at_gop_start));
		// When padding is active the CC block time should be within
		// 1000/29.97 us of the differences.
		dbg_print(CCX_DMT_DECODER_608, "Max. FTS:	   %s  (without caption blocks since then)\n",
			print_mstime(get_fts_max()));

		if (ctx->stat_hdtv)
		{
			mprint ("\rCC type 0: %d (%s)\n", dec_ctx->cc_stats[0], cc_types[0]);
			mprint ("CC type 1: %d (%s)\n", dec_ctx->cc_stats[1], cc_types[1]);
			mprint ("CC type 2: %d (%s)\n", dec_ctx->cc_stats[2], cc_types[2]);
			mprint ("CC type 3: %d (%s)\n", dec_ctx->cc_stats[3], cc_types[3]);
		}
		mprint ("\nTotal frames time:	  %s  (%u frames at %.2ffps)\n",
			print_mstime( (LLONG)(total_frames_count*1000/current_fps) ),
			total_frames_count, current_fps);
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

		// Add one frame as fts_max marks the beginning of the last frame,
		// but we need the end.
		fts_global += fts_max + (LLONG) (1000.0/current_fps);
		// CFS: At least in Hauppage mode, cb_field can be responsible for ALL the 
		// timing (cb_fields having a huge number and fts_now and fts_global being 0 all
		// the time), so we need to take that into account in fts_global before resetting
		// counters.
		if (cb_field1!=0)
			fts_global += cb_field1*1001/3;
		else
			fts_global += cb_field2*1001/3;
		// Reset counters - This is needed if some captions are still buffered
		// and need to be written after the last file is processed.		
		cb_field1 = 0; cb_field2 = 0; cb_708 = 0;
		fts_now = 0;
		fts_max = 0;
#ifdef ENABLE_SHARING
		if (ccx_options.sharing_enabled) {
			ccx_share_stream_done();
			ccx_share_stop();
		}
#endif //ENABLE_SHARING
	} // file loop
	close_input_file(ctx);

	ccx_demuxer_delete(&ctx->demux_ctx);
	flushbuffer (ctx, &ctx->wbout1, false);
	flushbuffer (ctx, &ctx->wbout2, false);

	prepare_for_new_file (ctx); // To reset counters used by handle_end_of_data()

	telxcc_close(ctx);
	if (ccx_options.extract != 2)
	{
		if (ccx_options.write_format==CCX_OF_SMPTETT || ccx_options.write_format==CCX_OF_SAMI || 
			ccx_options.write_format==CCX_OF_SRT || ccx_options.write_format==CCX_OF_TRANSCRIPT
			|| ccx_options.write_format==CCX_OF_SPUPNG )
		{
			handle_end_of_data(dec_ctx->context_cc608_field_1, &dec_sub);
			if (dec_sub.got_output)
			{
				encode_sub(enc_ctx,&dec_sub);
				dec_sub.got_output = 0;
			}
		}
		else if(ccx_options.write_format==CCX_OF_RCWT)
		{
			// Write last header and data
			writercwtdata (dec_ctx, NULL, &dec_sub);
			if (dec_sub.got_output)
			{
				encode_sub(enc_ctx,&dec_sub);
				dec_sub.got_output = 0;
			}
		}
		dinit_encoder(enc_ctx);
		flushbuffer (ctx, &ctx->wbout1,true);
	}
	if (ccx_options.extract != 1)
	{
		if (ccx_options.write_format == CCX_OF_SMPTETT || ccx_options.write_format == CCX_OF_SAMI ||
			ccx_options.write_format == CCX_OF_SRT || ccx_options.write_format == CCX_OF_TRANSCRIPT
			|| ccx_options.write_format == CCX_OF_SPUPNG )
		{
			handle_end_of_data(dec_ctx->context_cc608_field_2, &dec_sub);
			if (dec_sub.got_output)
			{
				encode_sub(enc_ctx,&dec_sub);
				dec_sub.got_output = 0;
			}
		}
		dinit_encoder(enc_ctx+1);
		flushbuffer (ctx, &ctx->wbout2,true);
	}
	time (&final);

	long proc_time=(long) (final-start);
	mprint ("\rDone, processing time = %ld seconds\n", proc_time);
	if (proc_time>0)
	{
		LLONG ratio=(get_fts_max()/10)/proc_time;
		unsigned s1=(unsigned) (ratio/100);
		unsigned s2=(unsigned) (ratio%100);	
		mprint ("Performance (real length/process time) = %u.%02u\n", 
			s1, s2);
	}
	dbg_print(CCX_DMT_708, "The 708 decoder was reset [%d] times.\n",resets_708);
	if (ccx_options.teletext_mode == CCX_TXT_IN_USE)
		mprint ( "Teletext decoder: %"PRIu32" packets processed, %"PRIu32" SRT frames written.\n", tlt_packet_counter, tlt_frames_produced);

	if (dec_ctx->processed_enough)
	{
		mprint ("\rNote: Processing was cancelled before all data was processed because\n");
		mprint ("\rone or more user-defined limits were reached.\n");
	} 
	if (ccblocks_in_avc_lost>0)
	{
		mprint ("Total caption blocks received: %d\n", ccblocks_in_avc_total);
		mprint ("Total caption blocks lost: %d\n", ccblocks_in_avc_lost);
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
