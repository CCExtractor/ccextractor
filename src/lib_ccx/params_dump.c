#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "teletext.h"

#include "ccx_decoders_708.h"

void params_dump(struct lib_ccx_ctx *ctx)
{
	// Display parsed parameters
	mprint("Input: ");
	switch (ccx_options.input_source)
	{
		case CCX_DS_FILE:
			dbg_print(CCX_DMT_VERBOSE, "Files (%d): ", ctx->num_input_files);
			for (int i = 0; i < ctx->num_input_files; i++)
				mprint("%s%s", ctx->inputfile[i], i == (ctx->num_input_files - 1) ? "" : ", ");
			break;
		case CCX_DS_STDIN:
			mprint("stdin");
			break;
		case CCX_DS_NETWORK:
			if (ccx_options.udpsrc != NULL)
				mprint("Network, %s@%s:%d", ccx_options.udpsrc, ccx_options.udpaddr, ccx_options.udpport);
			else if (ccx_options.udpaddr == NULL)
				mprint("Network, UDP/%u", ccx_options.udpport);
			else
			{
				mprint("Network, %s:%d", ccx_options.udpaddr, ccx_options.udpport);
			}
			break;
		case CCX_DS_TCP:
			mprint("Network, TCP/%s", ccx_options.tcpport);
			break;
	}
	mprint("\n");
	mprint("[Extract: %d] ", ccx_options.extract);
	mprint("[Stream mode: ");

	ctx->demux_ctx->print_cfg(ctx->demux_ctx);
	mprint("]\n");
	mprint("[Program : ");
	if (ccx_options.demux_cfg.ts_forced_program_selected != 0)
		mprint("%u ]", ccx_options.demux_cfg.ts_forced_program);
	else
		mprint("Auto ]");
	mprint(" [Hauppage mode: %s]", ccx_options.hauppauge_mode ? "Yes" : "No");

	mprint(" [Use MythTV code: ");
	switch (ccx_options.auto_myth)
	{
		case 0:
			mprint("Disabled");
			break;
		case 1:
			mprint("Forced - Overrides stream mode setting");
			break;
		case 2:
			mprint("Auto");
			break;
	}
	mprint("]\n");

	if (ccx_options.settings_dtvcc.enabled)
	{
		mprint("[CEA-708: %d decoders active]\n", ccx_options.settings_dtvcc.active_services_count);
		if (ccx_options.settings_dtvcc.active_services_count == CCX_DTVCC_MAX_SERVICES)
		{
			char *charset = ccx_options.enc_cfg.all_services_charset;
			mprint("[CEA-708: using charset \"%s\" for all services]\n", charset ? charset : "none");
		}
		else
		{
			for (int i = 0; i < CCX_DTVCC_MAX_SERVICES; i++)
			{
				if (ccx_options.settings_dtvcc.services_enabled[i])
					mprint("[CEA-708: using charset \"%s\" for service %d]\n",
					       ccx_options.enc_cfg.services_charsets[i] ? ccx_options.enc_cfg.services_charsets[i] : "none",
					       i + 1);
			}
		}
	}

	if (ccx_options.wtvconvertfix)
		mprint(" [Windows 7 wtv to dvr-ms conversion fix: Enabled]\n");

	if (ccx_options.wtvmpeg2)
		mprint(" [WTV use MPEG2 stream: Enabled]\n");

	mprint("[Timing mode: ");
	switch (ccx_options.use_gop_as_pts)
	{
		case 1:
			mprint("GOP (forced)");
			break;
		case -1:
			mprint("PTS (forced)");
			break;
		case 0:
			mprint("Auto");
			break;
	}
	mprint("] ");
	mprint("[Debug: %s] ", (ccx_options.debug_mask & CCX_DMT_VERBOSE) ? "Yes" : "No");
	mprint("[Buffer input: %s]\n", ccx_options.buffer_input ? "Yes" : "No");
	mprint("[Use pic_order_cnt_lsb for H.264: %s] ", ccx_options.usepicorder ? "Yes" : "No");
	mprint("[Print CC decoder traces: %s]\n", (ccx_options.debug_mask & CCX_DMT_DECODER_608) ? "Yes" : "No");
	mprint("[Target format: %s] ", ctx->extension);
	mprint("[Encoding: ");
	switch (ccx_options.enc_cfg.encoding)
	{
		case CCX_ENC_UNICODE:
			mprint("Unicode");
			break;
		case CCX_ENC_UTF_8:
			mprint("UTF-8");
			break;
		case CCX_ENC_LATIN_1:
			mprint("Latin-1");
			break;
		case CCX_ENC_ASCII:
			mprint("ASCII");
			break;
	}
	mprint("] ");
	mprint("[Delay: %lld] ", ctx->subs_delay);

	mprint("[Trim lines: %s]\n", ccx_options.enc_cfg.trim_subs ? "Yes" : "No");
	mprint("[Add font color data: %s] ", ccx_options.nofontcolor ? "No" : "Yes");
	mprint("[Add font typesetting: %s]\n", ccx_options.notypesetting ? "No" : "Yes");
	mprint("[Convert case: ");
	if (ccx_options.sentence_cap_file != NULL)
		mprint("Yes, using %s", ccx_options.sentence_cap_file);
	else
	{
		mprint("%s", ccx_options.enc_cfg.sentence_cap ? "Yes, but only built-in words" : "No");
	}
	mprint("]");

	mprint("[Filter profanity: ");
	if (ccx_options.filter_profanity_file != NULL)
	{
		mprint("Yes, using %s", ccx_options.filter_profanity_file);
	}
	else
	{
		mprint(ccx_options.enc_cfg.filter_profanity ? "Yes, but only filtering builtin words" : "No");
	}
	mprint("]");

	mprint(" [Video-edit join: %s]", ccx_options.binary_concat ? "No" : "Yes");
	mprint("\n[Extraction start time: ");
	if (ccx_options.extraction_start.set == 0)
		mprint("not set (from start)");
	else
		mprint("%02d:%02d:%02d", ccx_options.extraction_start.hh,
		       ccx_options.extraction_start.mm,
		       ccx_options.extraction_start.ss);
	mprint("]\n");
	mprint("[Extraction end time: ");
	if (ccx_options.extraction_end.set == 0)
		mprint("not set (to end)");
	else
		mprint("%02d:%02d:%02d", ccx_options.extraction_end.hh,
		       ccx_options.extraction_end.mm,
		       ccx_options.extraction_end.ss);
	mprint("]\n");
	mprint("[Live stream: ");
	if (ccx_options.live_stream == 0)
		mprint("No");
	else
	{
		if (ccx_options.live_stream < 1)
			mprint("Yes, no timeout");
		else
			mprint("Yes, timeout: %d seconds", ccx_options.live_stream);
	}
	mprint("] [Clock frequency: %d]\n", MPEG_CLOCK_FREQ);
	mprint("[Teletext page: ");
	if (tlt_config.page)
		mprint("%d]\n", tlt_config.page);
	else
		mprint("Autodetect]\n");
	mprint("[Start credits text: %s]\n",
	       ccx_options.enc_cfg.start_credits_text ? ccx_options.enc_cfg.start_credits_text : "None");
	if (ccx_options.enc_cfg.start_credits_text)
	{
		mprint("Start credits time: Insert between [%ld] and [%ld] seconds\n",
		       (long)(ccx_options.enc_cfg.startcreditsnotbefore.time_in_ms / 1000),
		       (long)(ccx_options.enc_cfg.startcreditsnotafter.time_in_ms / 1000));
		mprint("                    Display for at least [%ld] and at most [%ld] seconds\n",
		       (long)(ccx_options.enc_cfg.startcreditsforatleast.time_in_ms / 1000),
		       (long)(ccx_options.enc_cfg.startcreditsforatmost.time_in_ms / 1000));
	}
	if (ccx_options.enc_cfg.end_credits_text)
	{
		mprint("End credits text: [%s]\n",
		       ccx_options.enc_cfg.end_credits_text ? ccx_options.enc_cfg.end_credits_text : "None");
		mprint("                    Display for at least [%ld] and at most [%ld] seconds\n",
		       (long)(ccx_options.enc_cfg.endcreditsforatleast.time_in_ms / 1000),
		       (long)(ccx_options.enc_cfg.endcreditsforatmost.time_in_ms / 1000));
	}

	mprint("[Quantisation-mode: ");
	switch (ccx_options.ocr_quantmode)
	{
		case 0:
			// when no quantisation
			mprint("None]\n");
			break;
		case 1:
			// default mode, CCExtractor's internal function
			mprint("CCExtractor's internal function]\n");
			break;
		case 2:
			// reduced color palette quantisation
			mprint("Reduced color palette]\n");
			break;
	}
}

#define Y_N(cond) ((cond) ? "Yes" : "No")

void print_cc_report(struct lib_ccx_ctx *ctx, struct cap_info *info)
{
	struct lib_cc_decode *dec_ctx = NULL;
	dec_ctx = update_decoder_list_cinfo(ctx, info);
	printf("EIA-608: %s\n", Y_N(dec_ctx->cc_stats[0] > 0 || dec_ctx->cc_stats[1] > 0));

	if (dec_ctx->cc_stats[0] > 0 || dec_ctx->cc_stats[1] > 0)
	{
		printf("XDS: %s\n", Y_N(ctx->freport.data_from_608->xds));

		printf("CC1: %s\n", Y_N(ctx->freport.data_from_608->cc_channels[0]));
		printf("CC2: %s\n", Y_N(ctx->freport.data_from_608->cc_channels[1]));
		printf("CC3: %s\n", Y_N(ctx->freport.data_from_608->cc_channels[2]));
		printf("CC4: %s\n", Y_N(ctx->freport.data_from_608->cc_channels[3]));
	}
	printf("CEA-708: %s\n", Y_N(dec_ctx->cc_stats[2] > 0 || dec_ctx->cc_stats[3] > 0));

	if (dec_ctx->cc_stats[2] > 0 || dec_ctx->cc_stats[3] > 0)
	{
		printf("Services: ");
		for (int i = 0; i < CCX_DTVCC_MAX_SERVICES; i++)
		{
			if (ctx->freport.data_from_708->services[i] == 0)
				continue;
			printf("%d ", i);
		}
		printf("\n");

		printf("Primary Language Present: %s\n", Y_N(ctx->freport.data_from_708->services[1]));

		printf("Secondary Language Present: %s\n", Y_N(ctx->freport.data_from_708->services[2]));
	}
}

void print_file_report(struct lib_ccx_ctx *ctx)
{
	struct lib_cc_decode *dec_ctx = NULL;
	struct ccx_demuxer *demux_ctx = ctx->demux_ctx;

	printf("File: ");
	switch (ccx_options.input_source)
	{
		case CCX_DS_FILE:
			if (ctx->current_file < 0)
			{
				printf("file is not opened yet\n");
				return;
			}

			printf("%s\n", ctx->inputfile[ctx->current_file]);
			break;
		case CCX_DS_STDIN:
			printf("stdin\n");
			break;
		case CCX_DS_TCP:
		case CCX_DS_NETWORK:
			printf("network\n");
			break;
	}

	struct cap_info *program;
	printf("Stream Mode: ");
	switch (demux_ctx->stream_mode)
	{
		case CCX_SM_TRANSPORT:
			printf("Transport Stream\n");

			printf("Program Count: %d\n", demux_ctx->freport.program_cnt);

			printf("Program Numbers: ");

			for (int i = 0; i < demux_ctx->nb_program; i++)
				printf("%u ", demux_ctx->pinfo[i].program_number);

			printf("\n");

			for (int i = 0; i < 65536; i++)
			{
				if (demux_ctx->PIDs_programs[i] == 0)
					continue;

				printf("PID: %u, Program: %u, ", i, demux_ctx->PIDs_programs[i]->program_number);
				int j;
				for (j = 0; j < SUB_STREAMS_CNT; j++)
				{
					if (demux_ctx->freport.dvb_sub_pid[j] == i)
					{
						printf("DVB Subtitles\n");
						break;
					}
					if (demux_ctx->freport.tlt_sub_pid[j] == i)
					{
						printf("Teletext Subtitles\n");
						break;
					}
				}
				if (j == SUB_STREAMS_CNT)
					printf("%s\n", desc[demux_ctx->PIDs_programs[i]->printable_stream_type]);
			}

			break;
		case CCX_SM_PROGRAM:
			printf("Program Stream\n");
			break;
		case CCX_SM_ASF:
			printf("ASF\n");
			break;
		case CCX_SM_WTV:
			printf("WTV\n");
			break;
		case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
			printf("Not Found\n");
			break;
		case CCX_SM_MP4:
			printf("MP4\n");
			break;
		case CCX_SM_MCPOODLESRAW:
			printf("McPoodle's raw\n");
			break;
		case CCX_SM_RCWT:
			printf("BIN\n");
			break;
#ifdef WTV_DEBUG
		case CCX_SM_HEX_DUMP:
			printf("Hex\n");
			break;
#endif
		default:
			break;
	}
	if (list_empty(&demux_ctx->cinfo_tree.all_stream))
	{
		print_cc_report(ctx, NULL);
	}
	list_for_each_entry(program, &demux_ctx->cinfo_tree.pg_stream, pg_stream, struct cap_info)
	{
		struct cap_info *info = NULL;
		printf("//////// Program #%u: ////////\n", program->program_number);

		printf("DVB Subtitles: ");
		info = get_sib_stream_by_type(program, CCX_CODEC_DVB);
		if (info)
			printf("Yes\n");
		else
			printf("No\n");

		printf("Teletext: ");
		info = get_sib_stream_by_type(program, CCX_CODEC_TELETEXT);
		if (info)
		{
			printf("Yes\n");
			dec_ctx = update_decoder_list_cinfo(ctx, info);
			printf("Pages With Subtitles: ");
			tlt_print_seen_pages(dec_ctx);

			printf("\n");
		}
		else
			printf("No\n");

		printf("ATSC Closed Caption: ");
		info = get_sib_stream_by_type(program, CCX_CODEC_ATSC_CC);
		if (info)
		{
			printf("Yes\n");
			print_cc_report(ctx, info);
		}
		else
			printf("No\n");

		info = get_best_sib_stream(program);
		if (!info)
			continue;

		dec_ctx = update_decoder_list_cinfo(ctx, info);
		if (dec_ctx->in_bufferdatatype == CCX_PES &&
		    (demux_ctx->stream_mode == CCX_SM_TRANSPORT ||
		     demux_ctx->stream_mode == CCX_SM_PROGRAM ||
		     demux_ctx->stream_mode == CCX_SM_ASF ||
		     demux_ctx->stream_mode == CCX_SM_WTV))
		{
			printf("Width: %u\n", dec_ctx->current_hor_size);
			printf("Height: %u\n", dec_ctx->current_vert_size);
			printf("Aspect Ratio: %s\n", aspect_ratio_types[dec_ctx->current_aspect_ratio]);
			printf("Frame Rate: %s\n", framerates_types[dec_ctx->current_frame_rate]);
		}
		printf("\n");
	}

	printf("MPEG-4 Timed Text: %s\n", Y_N(ctx->freport.mp4_cc_track_cnt));
	if (ctx->freport.mp4_cc_track_cnt)
	{
		printf("MPEG-4 Timed Text tracks count: %d\n", ctx->freport.mp4_cc_track_cnt);
	}

	freep(&ctx->freport.data_from_608);
	memset(&ctx->freport, 0, sizeof(struct file_report));
#undef Y_N
}
