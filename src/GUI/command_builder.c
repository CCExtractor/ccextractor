#ifndef NK_IMPLEMENTATION
#include "nuklear_lib/nuklear.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif //! NK_IMPLEMENTATION

#include "ccextractorGUI.h"
#include "tabs.h"
#include "command_builder.h"

void command_builder(struct built_string *command,
		     struct main_tab *main_settings,
		     struct network_popup *network_settings, struct input_tab *input,
		     struct advanced_input_tab *advanced_input,
		     struct output_tab *output,
		     struct decoders_tab *decoders,
		     struct credits_tab *credits,
		     struct debug_tab *debug,
		     struct burned_subs_tab *burned_subs)
{
	static char buffer[1000];
#ifdef _WIN32
	strcpy(buffer, "ccextractorwin --gui_mode_reports");
#else
	strcpy(buffer, "./ccextractor --gui_mode_reports");
#endif

	/*INPUT COMMANDS*/
	if (main_settings->port_or_files == FILES)
	{
		if (input->type_select != 0)
		{
			strcat(buffer, " -in=");
			strcat(buffer, input->type[input->type_select]);
		}

		if (input->is_split)
			strcat(buffer, " --videoedited");

		if (input->is_process_from)
		{
			strcat(buffer, " -startat ");
			strcat(buffer, input->from_time_buffer);
		}

		if (input->is_process_until)
		{
			strcat(buffer, " -endat ");
			strcat(buffer, input->until_time_buffer);
		}

		switch (input->elementary_stream)
		{
			case AUTO_DETECT:
				break;
			case STREAM_TYPE:
				strcat(buffer, " -datastreamtype ");
				strncat(buffer, input->stream_type, input->stream_type_len);
				break;
			case STREAM_PID:
				strcat(buffer, " -datapid ");
				strncat(buffer, input->stream_pid, input->stream_pid_len);
		}

		if (input->is_assume_mpeg)
		{
			strcat(buffer, " -streamtype ");
			strncat(buffer, input->mpeg_type, input->mpeg_type_len);
		}

		if (decoders->teletext_dvb == TELETEXT)
		{
			switch (input->teletext_decoder)
			{
				case AUTO_DECODE:
					break;
				case FORCE:
					strcat(buffer, " -teletext");
					break;
				case DISABLE:
					strcat(buffer, " -noteletext");
			}

			if (input->is_process_teletext_page)
			{
				strcat(buffer, " -tpage ");
				strncat(buffer, input->teletext_page_number, input->teletext_page_numer_len);
			}
		}

		switch (input->is_limit)
		{
			case NO_LIMIT:
				break;
			case LIMITED:
				strcat(buffer, " --screenfuls ");
				strcat(buffer, input->screenful_limit_buffer);
		}

		switch (input->clock_input)
		{
			case AUTO:
				break;
			case GOP:
				strcat(buffer, " --goptime");
				break;
			case PTS:
				strcat(buffer, " --nogoptime");
				break;
		}
	}

	/*Main tab and network settings*/
	if (main_settings->port_or_files == PORT)
	{
		switch (main_settings->port_select)
		{
			case 0:
				strcat(buffer, " -udp ");
				if (!strstr(network_settings->udp_ipv4, "None"))
				{
					strncat(buffer, network_settings->udp_ipv4, network_settings->udp_ipv4_len);
					strcat(buffer, ":");
				}
				strncat(buffer, main_settings->port_num, main_settings->port_num_len);
				break;
			case 1:
				strcat(buffer, " -tcp ");
				strncat(buffer, main_settings->port_num, main_settings->port_num_len);
				if (!strstr(network_settings->tcp_pass, "None"))
				{
					strcat(buffer, " -tcppassword ");
					strncat(buffer, network_settings->tcp_pass, network_settings->tcp_pass_len);
				}
				if (!strstr(network_settings->tcp_desc, "None"))
				{
					strcat(buffer, " -tcpdesc ");
					strncat(buffer, network_settings->tcp_desc, network_settings->tcp_desc_len);
				}
				break;
			default:
				break;
		}

		if (input->is_live_stream)
		{
			strcat(buffer, " -s ");
			strncat(buffer, input->wait_data_sec, input->wait_data_sec_len);
		}

		if (input->is_process_from)
		{
			strcat(buffer, " -startat ");
			strcat(buffer, input->from_time_buffer);
		}

		if (input->is_process_until)
		{
			strcat(buffer, " -endat ");
			strcat(buffer, input->until_time_buffer);
		}

		switch (input->elementary_stream)
		{
			case AUTO_DETECT:
				break;
			case STREAM_TYPE:
				strcat(buffer, " -datastreamtype ");
				strncat(buffer, input->stream_type, input->stream_type_len);
				break;
			case STREAM_PID:
				strcat(buffer, " -datapid ");
				strncat(buffer, input->stream_pid, input->stream_pid_len);
		}

		if (input->is_assume_mpeg)
		{
			strcat(buffer, " -streamtype ");
			strncat(buffer, input->mpeg_type, input->mpeg_type_len);
		}

		switch (input->teletext_decoder)
		{
			case AUTO_DECODE:
				break;
			case FORCE:
				strcat(buffer, " -teletext");
				break;
			case DISABLE:
				strcat(buffer, " -noteletext");
		}

		if (input->is_process_teletext_page)
		{
			strcat(buffer, " -tpage ");
			strncat(buffer, input->teletext_page_number, input->teletext_page_numer_len);
		}

		switch (input->is_limit)
		{
			case NO_LIMIT:
				break;
			case LIMITED:
				strcat(buffer, " --screenfuls ");
				strcat(buffer, input->screenful_limit_buffer);
		}

		switch (input->clock_input)
		{
			case AUTO:
				break;
			case GOP:
				strcat(buffer, " --goptime");
				break;
			case PTS:
				strcat(buffer, " --nogoptime");
				break;
		}
	}

	/*ADVANCED INPUT SETTINGS*/
	if (advanced_input->is_multiple_program)
	{
		switch (advanced_input->multiple_program)
		{
			case FIRST_PROG:
				strcat(buffer, " -autoprogram");
				break;
			case PROG_NUM:
				strcat(buffer, " -pn ");
				strcat(buffer, advanced_input->prog_number);
				break;
		}
	}

	switch (advanced_input->set_myth)
	{
		case AUTO_MYTH:
			break;
		case FORCE_MYTH:
			strcat(buffer, " -myth");
			break;
		case DISABLE_MYTH:
			strcat(buffer, " -nomyth");
			break;
	}

	if (advanced_input->is_mpeg_90090)
		strcat(buffer, " -90090");
	if (advanced_input->is_padding_0000)
		strcat(buffer, " -fp");
	if (advanced_input->is_order_ccinfo)
		strcat(buffer, " -poc");
	if (advanced_input->is_win_bug)
		strcat(buffer, " -wtvconvertfix");
	if (advanced_input->is_hauppage_file)
		strcat(buffer, " -haup");
	if (advanced_input->is_process_mp4)
		strcat(buffer, " -mp4vidtrack");
	if (advanced_input->is_ignore_broadcast)
		strcat(buffer, " -noautotimeref");

	/*DECODERS TAB*/
	if (decoders->is_field2)
		strcat(buffer, " -12");

	switch (decoders->channel)
	{
		case CHANNEL_1:
			break;
		case CHANNEL_2:
			strcat(buffer, " -cc2");
			break;
	}

	if (decoders->is_708)
	{
		strcat(buffer, " -svc ");
		strncat(buffer, decoders->services, decoders->services_len);
	}

	switch (decoders->teletext_dvb)
	{
		case TELETEXT:
			if (strcmp(decoders->min_distance, "2"))
			{
				strcat(buffer, " -levdistmincnt ");
				strncat(buffer, decoders->min_distance, decoders->min_distance_len);
			}
			if (strcmp(decoders->max_distance, "10"))
			{
				strcat(buffer, " -levdistmaxpct ");
				strncat(buffer, decoders->max_distance, decoders->max_distance_len);
			}
			break;

		case DVB:
			strcat(buffer, " -codec dvdsub");
			break;
	}

	/*CREDITS TAB*/
	if (credits->is_start_text)
	{
		strcat(buffer, " --startcreditstext \"");
		strncat(buffer, credits->start_text, credits->start_text_len);
		strcat(buffer, "\" --startcreditsforatleast ");
		strncat(buffer, credits->start_atleast_sec, credits->start_atleast_sec_len);
		strcat(buffer, " --startcreditsforatmost ");
		strncat(buffer, credits->start_atmost_sec, credits->start_atmost_sec_len);
		if (credits->is_before)
		{
			strcat(buffer, " --startcreditsnotbefore ");
			strcat(buffer, credits->before_time_buffer);
		}
		if (credits->is_after)
		{
			strcat(buffer, " --startcreditsnotafter ");
			strcat(buffer, credits->after_time_buffer);
		}
	}

	if (credits->is_end_text)
	{
		strcat(buffer, " --endcreditstext \"");
		strncat(buffer, credits->end_text, credits->end_text_len);
		strcat(buffer, "\" --endcreditsforatleast ");
		strncat(buffer, credits->end_atleast_sec, credits->end_atleast_sec_len);
		strcat(buffer, " --endcreditsforatmost ");
		strncat(buffer, credits->end_atmost_sec, credits->end_atmost_sec_len);
	}

	/*DEBUG TAB*/
	if (debug->is_elementary_stream)
	{
		strcat(buffer, " -cf ");
		strncat(buffer, debug->elementary_stream, debug->elementary_stream_len);
	}
	if (debug->is_dump_packets)
		strcat(buffer, " -debug");
	if (debug->is_debug_608)
		strcat(buffer, " -608");
	if (debug->is_debug_708)
		strcat(buffer, " -708");
	if (debug->is_stamp_output)
		strcat(buffer, " -goppts");
	if (debug->is_debug_analysed_vid)
		strcat(buffer, " -vides");
	if (debug->is_raw_608_708)
		strcat(buffer, " -cbraw");
	if (debug->is_debug_parsed)
		strcat(buffer, " -parsedebug");
	if (!strcmp(output->type[output->type_select], "bin"))
	{
		if (debug->is_disable_sync)
			strcat(buffer, " -nosync");
		if (debug->is_no_padding)
			strcat(buffer, " -fullbin");
	}
	if (debug->is_debug_xds)
		strcat(buffer, " -xdsdebug");
	if (debug->is_output_pat)
		strcat(buffer, " -parsePAT");
	if (debug->is_output_pmt)
		strcat(buffer, " -parsePMT");
	if (debug->is_scan_ccdata)
		strcat(buffer, " -investigate_packets");
	if (debug->is_output_levenshtein)
		strcat(buffer, " -deblev");

	/*HARD_BURNED SUBS SETTINGS*/
	if (burned_subs->is_burned_subs)
	{
		strcat(buffer, " -hardsubx -ocr_mode");
		switch (burned_subs->ocr_mode)
		{
			case FRAME_WISE:
				strcat(buffer, " frame");
				break;
			case WORD_WISE:
				strcat(buffer, " word");
				break;
			case LETTER_WISE:
				strcat(buffer, " letter");
				break;
		}

		strcat(buffer, " -min_sub_duration ");
		strcat(buffer, burned_subs->min_duration);

		if (!burned_subs->subs_color_select && burned_subs->color_type == PRESET)
			sprintf(buffer, "%s -whiteness_thresh %d", buffer, burned_subs->luminance_threshold);

		sprintf(buffer, "%s -conf_thresh %d", buffer, burned_subs->confidence_threshold);

		if (burned_subs->is_italic)
			strcat(buffer, " -detect_italics");
	}

	// Output
	{
		strcat(buffer, " -out=");
		strcat(buffer, output->type[output->type_select]);
		if (output->is_filename)
		{
			strcat(buffer, " -o \"");
			strncat(buffer, output->filename, output->filename_len);
			strcat(buffer, "\"");
		}

		if (output->is_delay)
		{
			strcat(buffer, " -delay ");
			strcat(buffer, output->delay_sec_buffer);
		}

		if (output->is_export_xds)
			strcat(buffer, " -xds");

		switch (output->encoding)
		{
			case LATIN:
				strcat(buffer, " -latin1");
				break;
			case UNIC:
				strcat(buffer, " -unicode");
				break;
			case UTF:
				strcat(buffer, " -utf8");
				break;
		}

		if (output->is_bom)
			strcat(buffer, " -bom");
		else
			strcat(buffer, " -nobom");

		if (output->is_cap_standard)
			strcat(buffer, " --sentencecap");

		if (output->is_cap_file)
		{
			strcat(buffer, " --capfile \"");
			strncat(buffer, output->cap_dictionary, output->cap_dictionary_len);
			strcat(buffer, "\"");
		}

		switch (output->line_ending)
		{
			case CRLF:
				break;
			case LF:
				strcat(buffer, " -lf");
				break;
		}

		if (output->is_center)
			strcat(buffer, " -trim");

		if (output->is_dash)
			strcat(buffer, " -autodash");

		if (output->no_typesetting)
			strcat(buffer, " --notypesetting");

		switch (output->font_color)
		{
			case NO_COLOR:
				strcat(buffer, " --nofontcolor");
				break;
			case DEFAULT_COLOR:
				strcat(buffer, " --defaultcolor #");
				strcat(buffer, output->color_hex);
				break;
		}

		switch (output->onetime_or_realtime)
		{
			case ONETIME:
				strcat(buffer, " --norollup");
				break;
			case REALTIME:
				strcat(buffer, " -dru");
				switch (output->roll_limit_select)
				{
					case 1:
						strcat(buffer, " -ru1");
						break;
					case 2:
						strcat(buffer, " -ru2");
						break;
					case 3:
						strcat(buffer, " -ru3");
						break;
					default:
						break;
				}
		}
	}

	memset(command->term_string, 0, sizeof(command->term_string));
	strncpy(command->term_string, buffer, strlen(buffer));
}
