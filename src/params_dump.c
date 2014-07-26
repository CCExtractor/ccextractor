#include "ccextractor.h"

void params_dump(void) 
{
    // Display parsed parameters
    mprint ("Input: ");
	switch (ccx_options.input_source)
	{
		case CCX_DS_FILE:
			for (int i=0;i<num_input_files;i++)
			mprint ("%s%s",inputfile[i],i==(num_input_files-1)?"":",");	
			break;
		case CCX_DS_STDIN:
			mprint ("stdin");
			break;
		case CCX_DS_NETWORK:
		    if (ccx_options.udpaddr == NULL)
				mprint ("Network, UDP/%u",ccx_options.udpport);
			else
			{
				mprint ("Network, %s:%d",ccx_options.udpport, ccx_options.udpport);
			}
			break;
		case CCX_DS_TCP:
			mprint("Network, TCP/%s", ccx_options.tcpport);
			break;
	}
    mprint ("\n");    
    mprint ("[Extract: %d] ", ccx_options.extract);
    mprint ("[Stream mode: ");
    switch (auto_stream)
    {
        case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
            mprint ("Elementary");
            break;
        case CCX_SM_TRANSPORT:
            mprint ("Transport");
            break;
        case CCX_SM_PROGRAM:
            mprint ("Program");
            break;
        case CCX_SM_ASF:
            mprint ("DVR-MS");
            break;
        case CCX_SM_WTV:
            mprint ("Windows Television (WTV)");
            break;
        case CCX_SM_MCPOODLESRAW:
            mprint ("McPoodle's raw");
            break;
        case CCX_SM_AUTODETECT:
            mprint ("Autodetect");
            break;
		case CCX_SM_RCWT:
			mprint ("BIN");
			break;
		case CCX_SM_MP4:
			mprint ("MP4");
			break;
#ifdef WTV_DEBUG
		case CCX_SM_HEX_DUMP:
			mprint ("Hex");
			break;
#endif
        default:
            fatal (EXIT_BUG_BUG, "BUG: Unknown stream mode.\n");
            break;
    }
    mprint ("]\n");
	mprint ("[Program : ");
	if (ccx_options.ts_forced_program_selected != 0)
		mprint ("%u ]",ccx_options.ts_forced_program);
	else
		mprint ("Auto ]");
	mprint (" [Hauppage mode: %s]",ccx_options.hauppauge_mode?"Yes":"No");
	
    mprint (" [Use MythTV code: ");
    switch (ccx_options.auto_myth)
    {
        case 0:
            mprint ("Disabled");
            break;
        case 1:
            mprint ("Forced - Overrides stream mode setting");
            break;
        case 2:
            mprint ("Auto");
            break;
    }
    mprint ("]");
    if (ccx_options.wtvconvertfix)
    {
        mprint (" [Windows 7 wtv to dvr-ms conversion fix: Enabled]");
    }
    mprint ("\n");

    if (ccx_options.wtvmpeg2)
    {
        mprint (" [WTV use MPEG2 stream: Enabled]");
    }
    mprint ("\n");

    mprint ("[Timing mode: ");
	switch (ccx_options.use_gop_as_pts)
	{
		case 1:
			mprint ("GOP (forced)");
			break;
		case -1:
			mprint ("PTS (forced)");
			break;
		case 0:
			mprint ("Auto");
			break;
	}
	mprint ("] ");		
    mprint ("[Debug: %s] ", (ccx_options.debug_mask & CCX_DMT_VERBOSE) ? "Yes": "No");
    mprint ("[Buffer input: %s]\n", ccx_options.buffer_input ? "Yes": "No");
    mprint ("[Use pic_order_cnt_lsb for H.264: %s] ", ccx_options.usepicorder ? "Yes": "No");
    mprint ("[Print CC decoder traces: %s]\n", (ccx_options.debug_mask & CCX_DMT_608) ? "Yes": "No");
    mprint ("[Target format: %s] ",extension);    
    mprint ("[Encoding: ");
    switch (ccx_options.encoding)
    {
        case CCX_ENC_UNICODE:
            mprint ("Unicode");
            break;
        case CCX_ENC_UTF_8:
            mprint ("UTF-8");
            break;
        case CCX_ENC_LATIN_1:
            mprint ("Latin-1");
            break;
    }
    mprint ("] ");
    mprint ("[Delay: %lld] ",subs_delay);    

    mprint ("[Trim lines: %s]\n",ccx_options.trim_subs?"Yes":"No");
    mprint ("[Add font color data: %s] ", ccx_options.nofontcolor? "No" : "Yes");
	mprint ("[Add font typesetting: %s]\n", ccx_options.notypesetting? "No" : "Yes");
    mprint ("[Convert case: ");
    if (ccx_options.sentence_cap_file!=NULL)
        mprint ("Yes, using %s", ccx_options.sentence_cap_file);
    else
    {
        mprint ("%s",ccx_options.sentence_cap?"Yes, but only built-in words":"No");
    }
    mprint ("]");
    mprint (" [Video-edit join: %s]", ccx_options.binary_concat?"No":"Yes");
    mprint ("\n[Extraction start time: ");
    if (ccx_options.extraction_start.set==0)
        mprint ("not set (from start)");
    else
        mprint ("%02d:%02d:%02d", ccx_options.extraction_start.hh,
        ccx_options.extraction_start.mm,
		ccx_options.extraction_start.ss);
    mprint ("]\n");
    mprint ("[Extraction end time: ");
    if (ccx_options.extraction_end.set==0)
        mprint ("not set (to end)");
    else
        mprint ("%02d:%02d:%02d", ccx_options.extraction_end.hh,
		ccx_options.extraction_end.mm,
        ccx_options.extraction_end.ss);
    mprint ("]\n");
    mprint ("[Live stream: ");
    if (ccx_options.live_stream==0)
        mprint ("No");
    else
    {
        if (ccx_options.live_stream<1)
            mprint ("Yes, no timeout");
        else
            mprint ("Yes, timeout: %d seconds",ccx_options.live_stream);
    }
    mprint ("] [Clock frequency: %d]\n",MPEG_CLOCK_FREQ);
	mprint ("Teletext page: [");
	if (tlt_config.page)
		mprint ("%d]\n",tlt_config.page);
	else
		mprint ("Autodetect]\n");
    mprint ("Start credits text: [%s]\n", 
		ccx_options.start_credits_text?ccx_options.start_credits_text:"None");
    if (ccx_options.start_credits_text)
    {
        mprint ("Start credits time: Insert between [%ld] and [%ld] seconds\n",
            (long) (ccx_options.startcreditsnotbefore.time_in_ms/1000), 
            (long) (ccx_options.startcreditsnotafter.time_in_ms/1000)
            );
        mprint ("                    Display for at least [%ld] and at most [%ld] seconds\n",
            (long) (ccx_options.startcreditsforatleast.time_in_ms/1000), 
            (long) (ccx_options.startcreditsforatmost.time_in_ms/1000)
            );
    }
    if (ccx_options.end_credits_text)
    {
        mprint ("End credits text: [%s]\n", 
			ccx_options.end_credits_text?ccx_options.end_credits_text:"None");
        mprint ("                    Display for at least [%ld] and at most [%ld] seconds\n",
            (long) (ccx_options.endcreditsforatleast.time_in_ms/1000), 
            (long) (ccx_options.endcreditsforatmost.time_in_ms/1000)
            );
    }

}

void print_file_report(void) 
{
	#define Y_N(cond) ((cond) ? "Yes" : "No")

	printf("File: ");
	switch (ccx_options.input_source)
	{
		case CCX_DS_FILE:
			printf("%s\n", inputfile[current_file]);
			break;
		case CCX_DS_STDIN:
			printf("stdin\n");
			break;
		case CCX_DS_TCP:
		case CCX_DS_NETWORK:
			printf("network\n");
			break;
	}

	printf("Stream Mode: ");
	switch (stream_mode)
	{
		case CCX_SM_TRANSPORT:
			printf("Transport Stream\n");

			printf("Program Count: %d\n", file_report.program_cnt);

			printf("Program Numbers: ");
			for (int i = 0; i < pmt_array_length; i++)
			{
				if (pmt_array[i].program_number == 0)
					continue;

				printf("%u ", pmt_array[i].program_number);
			}
			printf("\n");

			for (int i = 0; i < 65536; i++) 
			{
				if (PIDs_programs[i] == 0)
					continue;

				printf("PID: %u, Program: %u, ", i, PIDs_programs[i]->program_number);
				int j;
				for (j = 0; j < SUB_STREAMS_CNT; j++) 
				{
					if (file_report.dvb_sub_pid[j] == i)
					{
						printf("DVB Subtitles\n");
						break;
					}
					if (file_report.tlt_sub_pid[j] == i)
					{
						printf("Teletext Subtitles\n");
						break;
					}
				}
				if (j == SUB_STREAMS_CNT)
					printf("%s\n", desc[PIDs_programs[i]->printable_stream_type]);
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

	if (ccx_bufferdatatype == CCX_PES && 
		(stream_mode == CCX_SM_TRANSPORT ||
		stream_mode == CCX_SM_PROGRAM ||
		stream_mode == CCX_SM_ASF ||
		stream_mode == CCX_SM_WTV))
	{
		printf("Width: %u\n", file_report.width);
		printf("Height: %u\n", file_report.height);
		printf("Aspect Ratio: %s\n", aspect_ratio_types[file_report.aspect_ratio]);
		printf("Frame Rate: %s\n", framerates_types[file_report.frame_rate]);
	}

	if (file_report.program_cnt > 1)
		printf("//////// Program #%u: ////////\n", TS_program_number);

	printf("DVB Subtitles: ");
	int j;
	for (j = 0; j < SUB_STREAMS_CNT; j++) 
	{
		unsigned pid = file_report.dvb_sub_pid[j];
		if (pid == 0)
			continue;
		if (PIDs_programs[pid]->program_number == TS_program_number)
		{
			printf("Yes\n");
			break;
		}
	}
	if (j == SUB_STREAMS_CNT)
		printf("No\n");

	printf("Teletext: ");
	for (j = 0; j < SUB_STREAMS_CNT; j++) 
	{
		unsigned pid = file_report.tlt_sub_pid[j];
		if (pid == 0)
			continue;
		if (PIDs_programs[pid]->program_number == TS_program_number)
		{
			printf("Yes\n");

			printf("Pages With Subtitles: ");
			for (int i = 0; i < MAX_TLT_PAGES; i++) 
			{
				if (seen_sub_page[i] == 0)
					continue;

				printf("%d ", i);
			}
			printf("\n");
			break;
		}
	}
	if (j == SUB_STREAMS_CNT)
		printf("No\n");

	printf("EIA-608: %s\n", Y_N(cc_stats[0] > 0 || cc_stats[1] > 0));

	if (cc_stats[0] > 0 || cc_stats[1] > 0) 
	{
		printf("XDS: %s\n", Y_N(file_report.xds));

		printf("CC1: %s\n", Y_N(file_report.cc_channels_608[0]));
		printf("CC2: %s\n", Y_N(file_report.cc_channels_608[1]));
		printf("CC3: %s\n", Y_N(file_report.cc_channels_608[2]));
		printf("CC4: %s\n", Y_N(file_report.cc_channels_608[3]));
	}

	printf("CEA-708: %s\n", Y_N(cc_stats[2] > 0 || cc_stats[3] > 0));

	if (cc_stats[2] > 0 || cc_stats[3] > 0) 
	{
		printf("Services: ");
		for (int i = 0; i < 63; i++) 
		{
			if (file_report.services708[i] == 0)
				continue;
			printf("%d ", i);
		}
		printf("\n");

		printf("Primary Language Present: %s\n", Y_N(file_report.services708[1]));

		printf("Secondary Language Present: %s\n", Y_N(file_report.services708[2]));
	}

	printf("MPEG-4 Timed Text: %s\n", Y_N(file_report.mp4_cc_track_cnt));
	if (file_report.mp4_cc_track_cnt) {
		printf("MPEG-4 Timed Text tracks count: %d\n", file_report.mp4_cc_track_cnt);
	}

	memset(&file_report, 0, sizeof (struct file_report_t));

	#undef Y_N
}
