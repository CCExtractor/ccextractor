#include "save_load_data.h"
#include "ccextractorGUI.h"
#include "tabs.h"
#include "popups.h"

void load_data(FILE *file,
	       struct main_tab *main_settings,
	       struct input_tab *input,
	       struct advanced_input_tab *advanced_input,
	       struct output_tab *output,
	       struct decoders_tab *decoders,
	       struct credits_tab *credits,
	       struct debug_tab *debug,
	       struct hd_homerun_tab *hd_homerun,
	       struct burned_subs_tab *burned_subs,
	       struct network_popup *network_settings)
{
	int null_int, r, g, b;
	char null_char[260];

	// Read main_tab data
	fscanf(file, "port_or_files:%d\n", &main_settings->port_or_files);
	fscanf(file, "port_num_len:%d\n", &main_settings->port_num_len);
	if (main_settings->port_num_len > 0)
		fscanf(file, "port_num:%[^\n]\n", main_settings->port_num);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "is_check_common_extension:%d\n", &main_settings->is_check_common_extension);
	fscanf(file, "port_select:%d\n", &main_settings->port_select);

	// Read input_tab data
	fscanf(file, "type_select:%d\n", &input->type_select);
	fscanf(file, "is_split:%d\n", &input->is_split);
	fscanf(file, "is_live_stream:%d\n", &input->is_live_stream);
	fscanf(file, "wait_data_sec_len:%d\n", &input->wait_data_sec_len);
	if (input->wait_data_sec_len > 0)
		fscanf(file, "wait_data_sec:%[^\n]\n", input->wait_data_sec);
	else
		fscanf(file, "%[^\n]\n", null_char);
	fscanf(file, "is_process_from:%d\n", &input->is_process_from);
	fscanf(file, "is_process_until:%d\n", &input->is_process_until);

	fscanf(file, "from_time_buffer:%[^\n]\n", input->from_time_buffer);

	fscanf(file, "until_time_buffer:%[^\n]\n", input->until_time_buffer);

	fscanf(file, "elementary_stream:%d\n", &input->elementary_stream);
	fscanf(file, "is_assume_mpeg:%d\n", &input->is_assume_mpeg);
	fscanf(file, "stream_type_len:%d\n", &input->stream_type_len);
	if (input->stream_type_len > 0)
		fscanf(file, "stream_type:%[^\n]\n", input->stream_type);
	else
		fscanf(file, "%[^\n]\n", null_char);
	fscanf(file, "stream_pid_len:%d\n", &input->stream_pid_len);
	if (input->stream_pid_len > 0)
		fscanf(file, "stream_pid:%[^\n]\n", input->stream_pid);
	else
		fscanf(file, "%[^\n]\n", null_char);
	fscanf(file, "mpeg_type_len:%d\n", &input->mpeg_type_len);
	if (input->mpeg_type_len > 0)
		fscanf(file, "mpeg_type:%[^\n]\n", input->mpeg_type);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "teletext_decoder:%d\n", &input->teletext_decoder);
	fscanf(file, "is_process_teletext_page:%d\n", &input->is_process_teletext_page);
	fscanf(file, "teletext_page_number_len:%d\n", &input->teletext_page_numer_len);
	if (input->teletext_page_numer_len)
		fscanf(file, "teletext_page_number:%[^\n]\n", input->teletext_page_number);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "is_limit:%d\n", &input->is_limit);
	fscanf(file, "screenful_limit_buffer:%[^\n]\n", input->screenful_limit_buffer);
	fscanf(file, "clock_input:%d\n", &input->clock_input);

	// Read advanced_input_tab data
	fscanf(file, "is_multiple_program:%d\n", &advanced_input->is_multiple_program);
	fscanf(file, "multiple_program:%d\n", &advanced_input->multiple_program);
	fscanf(file, "prog_number_len:%d\n", &advanced_input->prog_number_len);
	if (advanced_input->prog_number_len)
		fscanf(file, "prog_number:%[^\n]\n", advanced_input->prog_number);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "set_myth:%d\n", &advanced_input->set_myth);
	fscanf(file, "is_mpeg_90090:%d\n", &advanced_input->is_mpeg_90090);
	fscanf(file, "is_padding_0000:%d\n", &advanced_input->is_padding_0000);
	fscanf(file, "is_order_ccinfo:%d\n", &advanced_input->is_order_ccinfo);
	fscanf(file, "is_win_bug:%d\n", &advanced_input->is_win_bug);
	fscanf(file, "is_hauppage_file:%d\n", &advanced_input->is_hauppage_file);
	fscanf(file, "is_process_mp4:%d\n", &advanced_input->is_process_mp4);
	fscanf(file, "is_ignore_broadcast:%d\n", &advanced_input->is_ignore_broadcast);

	// Read output_tab data
	fscanf(file, "type_select:%d\n", &output->type_select);
	fscanf(file, "is_filename:%d\n", &output->is_filename);
	fscanf(file, "filename_len:%d\n", &output->filename_len);
	if (output->filename_len > 0)
		fscanf(file, "filename:%[^\n]\n", output->filename);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "is_delay:%d\n", &output->is_delay);
	fscanf(file, "delay_sec_buffer:%[^\n]\n", output->delay_sec_buffer);
	fscanf(file, "is_export_xds:%d\n", &output->is_export_xds);
	fscanf(file, "encoding:%d\n", &output->encoding);
	fscanf(file, "is_bom:%d\n", &output->is_bom);
	fscanf(file, "is_cap_standard:%d\n", &output->is_cap_standard);
	fscanf(file, "is_cap_file:%d\n", &output->is_cap_file);
	fscanf(file, "cap_dictionary_len:%d\n", &output->cap_dictionary_len);
	if (output->cap_dictionary_len > 0)
		fscanf(file, "cap_dictionary:%[^\n]\n", output->cap_dictionary);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "line_ending:%d\n", &output->line_ending);
	fscanf(file, "is_center:%d\n", &output->is_center);
	fscanf(file, "is_dash:%d\n", &output->is_dash);
	fscanf(file, "no_typesetting:%d\n", &output->no_typesetting);
	fscanf(file, "font_color:%d\n", &output->font_color);
	fscanf(file, "color_hex:%[^\n]\n", output->color_hex);
	fscanf(file, "color_rgb_r:%d\n", &r);
	fscanf(file, "color_rgb_g:%d\n", &g);
	fscanf(file, "color_rgb_b:%d\n", &b);
	output->color_rgb.r = (nk_byte)r;
	output->color_rgb.g = (nk_byte)g;
	output->color_rgb.b = (nk_byte)b;

	fscanf(file, "onetime_or_realtime:%d\n", &output->onetime_or_realtime);
	fscanf(file, "roll_limit_select:%d\n", &output->roll_limit_select);

	// Read decoders_tab data
	fscanf(file, "is_field1:%d\n", &decoders->is_field1);
	fscanf(file, "is_field2:%d\n", &decoders->is_field2);
	fscanf(file, "channel:%d\n", &decoders->channel);
	fscanf(file, "is_708:%d\n", &decoders->is_708);
	fscanf(file, "services_len:%d\n", &decoders->services_len);
	if (decoders->services_len > 0)
		fscanf(file, "services:%[^\n]\n", decoders->services);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "teletext_dvb:%d\n", &decoders->teletext_dvb);
	fscanf(file, "min_distance_len:%d\n", &decoders->min_distance_len);
	if (decoders->min_distance_len > 0)
		fscanf(file, "min_distance:%[^\n]\n", decoders->min_distance);
	else
		fscanf(file, "%[^\n]\n", null_char);
	fscanf(file, "max_distance_len:%d\n", &decoders->max_distance_len);
	if (decoders->max_distance_len > 0)
		fscanf(file, "max_distance:%[^\n]\n", decoders->max_distance);
	else
		fscanf(file, "%[^\n]\n", null_char);

	// Read credits tab data
	fscanf(file, "is_start_text:%d\n", &credits->is_start_text);

	fscanf(file, "is_before:%d\n", &credits->is_before);
	fscanf(file, "is_after:%d\n", &credits->is_after);
	fscanf(file, "before_time_buffer:%[^\n]\n", credits->before_time_buffer);
	fscanf(file, "after_time_buffer:%[^\n]\n", credits->after_time_buffer);
	fscanf(file, "start_atmost_sec_len:%d\n", &credits->start_atmost_sec_len);
	if (credits->start_atmost_sec_len > 0)
		fscanf(file, "start_atmost_sec:%[^\n]\n", credits->start_atmost_sec);
	else
		fscanf(file, "%[^\n]\n", null_char);
	fscanf(file, "start_atleast_sec_len:%d\n", &credits->start_atleast_sec_len);
	if (credits->start_atleast_sec_len > 0)
		fscanf(file, "start_atleast_sec:%[^\n]\n", credits->start_atleast_sec);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "is_end_text:%d\n", &credits->is_end_text);
	fscanf(file, "end_atmost_sec_len:%d\n", &credits->end_atmost_sec_len);
	if (credits->end_atmost_sec_len > 0)
		fscanf(file, "end_atmost_sec:%[^\n]\n", credits->end_atmost_sec);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "end_atleast_sec_len:%d\n", &credits->end_atleast_sec_len);
	if (credits->end_atleast_sec_len > 0)
		fscanf(file, "end_atleast_sec:%[^\n]\n", credits->end_atleast_sec);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "start_text_len:%d\n", &credits->start_text_len);
	fscanf(file, "end_text_len:%d\n", &credits->end_text_len);
	read_credits(file, credits);

	// Read debug tab data
	fscanf(file, "is_elementary_stream:%d\n", &debug->is_elementary_stream);
	fscanf(file, "elementary_stream_len:%d\n", &debug->elementary_stream_len);
	if (debug->elementary_stream_len > 0)
		fscanf(file, "elementary_stream:%[^\n]\n", debug->elementary_stream);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "is_dump_packets:%d\n", &debug->is_dump_packets);
	fscanf(file, "is_debug_608:%d\n", &debug->is_debug_608);
	fscanf(file, "is_debug_708:%d\n", &debug->is_debug_708);
	fscanf(file, "is_stamp_output:%d\n", &debug->is_stamp_output);
	fscanf(file, "is_debug_analysed_vid:%d\n", &debug->is_debug_analysed_vid);
	fscanf(file, "is_raw_608_708:%d\n", &debug->is_raw_608_708);
	fscanf(file, "is_debug_parsed:%d\n", &debug->is_debug_parsed);
	fscanf(file, "is_disable_sync:%d\n", &debug->is_disable_sync);
	fscanf(file, "is_no_padding:%d\n", &debug->is_no_padding);
	fscanf(file, "is_debug_xds:%d\n", &debug->is_debug_xds);
	fscanf(file, "is_output_pat:%d\n", &debug->is_output_pat);
	fscanf(file, "is_output_pmt:%d\n", &debug->is_output_pmt);
	fscanf(file, "is_scan_ccdata:%d\n", &debug->is_scan_ccdata);
	fscanf(file, "is_output_levenshtein:%d\n", &debug->is_output_levenshtein);

	// Read HD Homerun Tab data
	fscanf(file, "location_len:%d\n", &hd_homerun->location_len);
	if (hd_homerun->location_len > 0)
		fscanf(file, "location:%[^\n]\n", hd_homerun->location);
	else
		fscanf(file, "%[^\n]\n", null_char);
	fscanf(file, "tuner_len:%d\n", &hd_homerun->tuner_len);
	if (hd_homerun->tuner_len > 0)
		fscanf(file, "tuner:%[^\n]\n", hd_homerun->tuner);
	else
		fscanf(file, "%[^\n]\n", null_char);
	fscanf(file, "channel_len:%d\n", &hd_homerun->channel_len);
	if (hd_homerun->channel_len > 0)
		fscanf(file, "channel:%[^\n]\n", hd_homerun->channel);
	else
		fscanf(file, "%[^\n]\n", null_char);
	fscanf(file, "program_len:%d\n", &hd_homerun->program_len);
	if (hd_homerun->program_len > 0)
		fscanf(file, "program:%[^\n]\n", hd_homerun->program);
	else
		fscanf(file, "%[^\n]\n", null_char);
	fscanf(file, "ipv4_address_len:%d\n", &hd_homerun->ipv4_address_len);
	if (hd_homerun->ipv4_address_len > 0)
		fscanf(file, "ipv4_address:%[^\n]\n", hd_homerun->ipv4_address);
	else
		fscanf(file, "%[^\n]\n", null_char);
	fscanf(file, "port_number_len:%d\n", &hd_homerun->port_number_len);
	if (hd_homerun->port_number_len > 0)
		fscanf(file, "port_number:%[^\n]\n", hd_homerun->port_number);
	else
		fscanf(file, "%[^\n]\n", null_char);

	// Read Burned Subs tab data
	fscanf(file, "is_burnded_subs:%d\n", &burned_subs->is_burned_subs);
	fscanf(file, "color_type:%d\n", &burned_subs->color_type);
	fscanf(file, "sub_color_select:%d\n", &burned_subs->subs_color_select);
	fscanf(file, "custom_hue_len:%d\n", &burned_subs->custom_hue_len);
	if (burned_subs->custom_hue_len > 0)
		fscanf(file, "custom_hue:%[^\n]\n", burned_subs->custom_hue);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "ocr_mode:%d\n", &burned_subs->ocr_mode);
	fscanf(file, "min_duration_len:%d\n", &burned_subs->min_duration_len);
	if (burned_subs->min_duration_len > 0)
		fscanf(file, "min_duration:%[^\n]\n", burned_subs->min_duration);
	else
		fscanf(file, "%[^\n]\n", null_char);

	fscanf(file, "luminance_threshold:%d\n", &burned_subs->luminance_threshold);
	fscanf(file, "confidence_threshold:%d\n", &burned_subs->confidence_threshold);
	fscanf(file, "is_italic:%d\n", &burned_subs->is_italic);

	// Read Network Settings data
	fscanf(file, "udp_ipv4_len:%d\n", &network_settings->udp_ipv4_len);
	if (network_settings->udp_ipv4_len > 0)
		fscanf(file, "udp_ipv4:%[^\n]\n", network_settings->udp_ipv4);
	else
		fscanf(file, "udp_ipv4:%[^\n]\n", null_char);
	fscanf(file, "tcp_pass_len:%d\n", &network_settings->tcp_pass_len);
	if (network_settings->tcp_pass_len > 0)
		fscanf(file, "tcp_pass:%[^\n]\n", network_settings->tcp_pass);
	else
		fscanf(file, "tcp_pass:%[^\n]\n", null_char);
	fscanf(file, "tcp_desc_len:%d\n", &network_settings->tcp_desc_len);
	if (network_settings->tcp_desc_len > 0)
		fscanf(file, "tcp_desc:%[^\n]\n", network_settings->tcp_desc);
	else
		fscanf(file, "tcp_desc:%[^\n]\n", null_char);
	fscanf(file, "send_port_len:%d\n", &network_settings->send_port_len);
	if (network_settings->send_port_len > 0)
		fscanf(file, "send_port:%[^\n]\n", network_settings->send_port);
	else
		fscanf(file, "send_port:%[^\n]\n", null_char);
	fscanf(file, "send_host_len:%d\n", &network_settings->send_host_len);
	if (network_settings->send_host_len > 0)
		fscanf(file, "send_host:%[^\n]\n", network_settings->send_host);
	fscanf(file, "send_host:%[^\n]\n", null_char);
}

void save_data(FILE *file,
	       struct main_tab *main_settings,
	       struct input_tab *input,
	       struct advanced_input_tab *advanced_input,
	       struct output_tab *output,
	       struct decoders_tab *decoders,
	       struct credits_tab *credits,
	       struct debug_tab *debug,
	       struct hd_homerun_tab *hd_homerun,
	       struct burned_subs_tab *burned_subs,
	       struct network_popup *network_settings)
{
	// Write main_tab data
	fprintf(file, "port_or_files:%d\n", main_settings->port_or_files);
	fprintf(file, "port_num_len:%d\n", main_settings->port_num_len);
	fprintf(file, "port_num:%s\n", main_settings->port_num);

	fprintf(file, "is_check_common_extension:%d\n", main_settings->is_check_common_extension);
	fprintf(file, "port_select:%d\n", main_settings->port_select);

	// Write input_tab data
	fprintf(file, "type_select:%d\n", input->type_select);
	fprintf(file, "is_split:%d\n", input->is_split);
	fprintf(file, "is_live_stream:%d\n", input->is_live_stream);
	fprintf(file, "wait_data_sec_len:%d\n", input->wait_data_sec_len);
	fprintf(file, "wait_data_sec:%s\n", input->wait_data_sec);

	fprintf(file, "is_process_from:%d\n", input->is_process_from);
	fprintf(file, "is_process_until:%d\n", input->is_process_until);
	fprintf(file, "from_time_buffer:%s\n", input->from_time_buffer);
	fprintf(file, "until_time_buffer:%s\n", input->until_time_buffer);
	fprintf(file, "elementary_stream:%d\n", input->elementary_stream);
	fprintf(file, "is_assume_mpeg:%d\n", input->is_assume_mpeg);
	fprintf(file, "stream_type_len:%d\n", input->stream_type_len);
	fprintf(file, "stream_type:%s\n", input->stream_type);
	fprintf(file, "stream_pid_len:%d\n", input->stream_pid_len);
	fprintf(file, "stream_pid:%s\n", input->stream_pid);
	fprintf(file, "mpeg_type_len:%d\n", input->mpeg_type_len);
	fprintf(file, "mpeg_type:%s\n", input->mpeg_type);

	fprintf(file, "teletext_decoder:%d\n", input->teletext_decoder);
	fprintf(file, "is_process_teletext_page:%d\n", input->is_process_teletext_page);
	fprintf(file, "teletext_page_number_len:%d\n", input->teletext_page_numer_len);
	fprintf(file, "teletext_page_number:%s\n", input->teletext_page_number);

	fprintf(file, "is_limit:%d\n", input->is_limit);
	fprintf(file, "screenful_limit_buffer:%s\n", input->screenful_limit_buffer);
	fprintf(file, "clock_input:%d\n", input->clock_input);

	// Write advanced_input_tab data
	fprintf(file, "is_multiple_program:%d\n", advanced_input->is_multiple_program);
	fprintf(file, "multiple_program:%d\n", advanced_input->multiple_program);
	fprintf(file, "prog_number_len:%d\n", advanced_input->prog_number_len);
	fprintf(file, "prog_number:%s\n", advanced_input->prog_number);

	fprintf(file, "set_myth:%d\n", advanced_input->set_myth);
	fprintf(file, "is_mpeg_90090:%d\n", advanced_input->is_mpeg_90090);
	fprintf(file, "is_padding_0000:%d\n", advanced_input->is_padding_0000);
	fprintf(file, "is_order_ccinfo:%d\n", advanced_input->is_order_ccinfo);
	fprintf(file, "is_win_bug:%d\n", advanced_input->is_win_bug);
	fprintf(file, "is_hauppage_file:%d\n", advanced_input->is_hauppage_file);
	fprintf(file, "is_process_mp4:%d\n", advanced_input->is_process_mp4);
	fprintf(file, "is_ignore_broadcast:%d\n", advanced_input->is_ignore_broadcast);

	// Write output_tab data
	fprintf(file, "type_select:%d\n", output->type_select);
	fprintf(file, "is_filename:%d\n", output->is_filename);
	fprintf(file, "filename_len:%d\n", output->filename_len);
	fprintf(file, "filename:%s\n", output->filename);
	fprintf(file, "is_delay:%d\n", output->is_delay);
	fprintf(file, "delay_sec_buffer:%s\n", output->delay_sec_buffer);
	fprintf(file, "is_export_xds:%d\n", output->is_export_xds);
	fprintf(file, "encoding:%d\n", output->encoding);
	fprintf(file, "is_bom:%d\n", output->is_bom);
	fprintf(file, "is_cap_standard:%d\n", output->is_cap_standard);
	fprintf(file, "is_cap_file:%d\n", output->is_cap_file);
	fprintf(file, "cap_dictionary_len:%d\n", output->cap_dictionary_len);
	fprintf(file, "cap_dictionary:%s\n", output->cap_dictionary);

	fprintf(file, "line_ending:%d\n", output->line_ending);
	fprintf(file, "is_center:%d\n", output->is_center);
	fprintf(file, "is_dash:%d\n", output->is_dash);
	fprintf(file, "no_typesetting:%d\n", output->no_typesetting);
	fprintf(file, "font_color:%d\n", output->font_color);
	fprintf(file, "color_hex:%s\n", output->color_hex);
	fprintf(file, "color_rgb_r:%d\n", output->color_rgb.r);
	fprintf(file, "color_rgb_g:%d\n", output->color_rgb.g);
	fprintf(file, "color_rgb_b:%d\n", output->color_rgb.b);
	fprintf(file, "onetime_or_realtime:%d\n", output->onetime_or_realtime);
	fprintf(file, "roll_limit_select:%d\n", output->roll_limit_select);

	// Write decoders_tab data
	fprintf(file, "is_field1:%d\n", decoders->is_field1);
	fprintf(file, "is_field2:%d\n", decoders->is_field2);
	fprintf(file, "channel:%d\n", decoders->channel);
	fprintf(file, "is_708:%d\n", decoders->is_708);
	fprintf(file, "services_len:%d\n", decoders->services_len);
	fprintf(file, "services:%s\n", decoders->services);

	fprintf(file, "teletext_dvb:%d\n", decoders->teletext_dvb);
	fprintf(file, "min_distance_len:%d\n", decoders->min_distance_len);
	fprintf(file, "min_distance:%s\n", decoders->min_distance);
	fprintf(file, "max_distance_len:%d\n", decoders->max_distance_len);
	fprintf(file, "max_distance:%s\n", decoders->max_distance);

	// Write credits tab data
	fprintf(file, "is_start_text:%d\n", credits->is_start_text);

	fprintf(file, "is_before:%d\n", credits->is_before);
	fprintf(file, "is_after:%d\n", credits->is_after);
	fprintf(file, "before_time_buffer:%s\n", credits->before_time_buffer);
	fprintf(file, "after_time_buffer:%s\n", credits->after_time_buffer);
	fprintf(file, "start_atmost_sec_len:%d\n", credits->start_atmost_sec_len);
	fprintf(file, "start_atmost_sec:%s\n", credits->start_atmost_sec);
	fprintf(file, "start_atleast_sec_len:%d\n", credits->start_atleast_sec_len);
	fprintf(file, "start_atleast_sec:%s\n", credits->start_atleast_sec);

	fprintf(file, "is_end_text:%d\n", credits->is_end_text);
	fprintf(file, "end_atmost_sec_len:%d\n", credits->end_atmost_sec_len);
	fprintf(file, "end_atmost_sec:%s\n", credits->end_atmost_sec);
	fprintf(file, "end_atleast_sec_len:%d\n", credits->end_atleast_sec_len);
	fprintf(file, "end_atleast_sec:%s\n", credits->end_atleast_sec);
	fprintf(file, "start_text_len:%d\n", credits->start_text_len);
	fprintf(file, "end_text_len:%d\n", credits->end_text_len);
	write_credits(file, credits);

	// Write debug tab data
	fprintf(file, "is_elementary_stream:%d\n", debug->is_elementary_stream);
	fprintf(file, "elementary_stream_len:%d\n", debug->elementary_stream_len);
	fprintf(file, "elementary_stream:%s\n", debug->elementary_stream);

	fprintf(file, "is_dump_packets:%d\n", debug->is_dump_packets);
	fprintf(file, "is_debug_608:%d\n", debug->is_debug_608);
	fprintf(file, "is_debug_708:%d\n", debug->is_debug_708);
	fprintf(file, "is_stamp_output:%d\n", debug->is_stamp_output);
	fprintf(file, "is_debug_analysed_vid:%d\n", debug->is_debug_analysed_vid);
	fprintf(file, "is_raw_608_708:%d\n", debug->is_raw_608_708);
	fprintf(file, "is_debug_parsed:%d\n", debug->is_debug_parsed);
	fprintf(file, "is_disable_sync:%d\n", debug->is_disable_sync);
	fprintf(file, "is_no_padding:%d\n", debug->is_no_padding);
	fprintf(file, "is_debug_xds:%d\n", debug->is_debug_xds);
	fprintf(file, "is_output_pat:%d\n", debug->is_output_pat);
	fprintf(file, "is_output_pmt:%d\n", debug->is_output_pmt);
	fprintf(file, "is_scan_ccdata:%d\n", debug->is_scan_ccdata);
	fprintf(file, "is_output_levenshtein:%d\n", debug->is_output_levenshtein);

	// Write HD Homerun Tab data
	fprintf(file, "location_len:%d\n", hd_homerun->location_len);
	fprintf(file, "location:%s\n", hd_homerun->location);
	fprintf(file, "tuner_len:%d\n", hd_homerun->tuner_len);
	fprintf(file, "tuner:%s\n", hd_homerun->tuner);
	fprintf(file, "channel_len:%d\n", hd_homerun->channel_len);
	fprintf(file, "channel:%s\n", hd_homerun->channel);
	fprintf(file, "program_len:%d\n", hd_homerun->program_len);
	fprintf(file, "program:%s\n", hd_homerun->program);
	fprintf(file, "ipv4_address_len:%d\n", hd_homerun->ipv4_address_len);
	fprintf(file, "ipv4_address:%s\n", hd_homerun->ipv4_address);
	fprintf(file, "port_number_len:%d\n", hd_homerun->port_number_len);
	fprintf(file, "port_number:%s\n", hd_homerun->port_number);

	// Write Burned Subs tab data
	fprintf(file, "is_burnded_subs:%d\n", burned_subs->is_burned_subs);
	fprintf(file, "color_type:%d\n", burned_subs->color_type);
	fprintf(file, "sub_color_select:%d\n", burned_subs->subs_color_select);
	fprintf(file, "custom_hue_len:%d\n", burned_subs->custom_hue_len);
	fprintf(file, "custom_hue:%s\n", burned_subs->custom_hue);

	fprintf(file, "ocr_mode:%d\n", burned_subs->ocr_mode);
	fprintf(file, "min_duration_len:%d\n", burned_subs->min_duration_len);
	fprintf(file, "min_duration:%s\n", burned_subs->min_duration);

	fprintf(file, "luminance_threshold:%d\n", burned_subs->luminance_threshold);
	fprintf(file, "confidence_threshold:%d\n", burned_subs->confidence_threshold);
	fprintf(file, "is_italic:%d\n", burned_subs->is_italic);

	// Write Network Settings popup data
	if (network_settings->save_network_settings)
	{
		fprintf(file, "udp_ipv4_len:%d\n", network_settings->udp_ipv4_len);
		fprintf(file, "udp_ipv4:%s\n", network_settings->udp_ipv4);
		fprintf(file, "tcp_pass_len:%d\n", network_settings->tcp_pass_len);
		fprintf(file, "tcp_pass:%s\n", network_settings->tcp_pass);
		fprintf(file, "tcp_desc_len:%d\n", network_settings->tcp_desc_len);
		fprintf(file, "tcp_desc:%s\n", network_settings->tcp_desc);
		fprintf(file, "send_port_len:%d\n", network_settings->send_port_len);
		fprintf(file, "send_port:%s\n", network_settings->send_port);
		fprintf(file, "send_host_len:%d\n", network_settings->send_host_len);
		fprintf(file, "send_host:%s\n", network_settings->send_host);
	}
}

void write_credits(FILE *file, struct credits_tab *credits)
{
	// Number of newlines in end_text
	static int newlines_end;
	// Number of newlines in start_text
	static int newlines_start;
	int newline_char = 10; // '\n' is 10 in ascii encoding
	for (int i = 0; i < credits->start_text_len; i++)
	{
		if (credits->start_text[i] == newline_char)
			newlines_start++;
	}

	for (int i = 0; i < credits->end_text_len; i++)
	{
		if (credits->end_text[i] == newline_char)
			newlines_end++;
	}

	fprintf(file, "start_text:%d\n", newlines_start);
	if (credits->start_text_len > 0)
		fprintf(file, "%s\n", credits->start_text);
	fprintf(file, "end_text:%d\n", newlines_end);
	if (credits->end_text_len > 0)
		fprintf(file, "%s\n", credits->end_text);
}

void read_credits(FILE *file, struct credits_tab *credits)
{
	// Number of newlines in end_text
	static int newlines_end;
	// Number of newlines in start_text
	static int newlines_start;
	static char buffer[1000], null_char[260];
	if (credits->start_text_len == 0)
		fscanf(file, "%[^\n]\n", null_char);
	else
	{
		fscanf(file, "start_text:%d\n", &newlines_start);
		for (int i = 0; i != newlines_start + 1; i++)
		{
			static char line[200];
			fscanf(file, "%[^\n]\n", line);
			if (!(i == newlines_start))
				strcat(line, "\n");
			if (strlen(buffer) > 0)
				strcat(buffer, line);
			else
				strcpy(buffer, line);
		}

		memset(credits->start_text, 0, sizeof(credits->start_text));
		strcpy(credits->start_text, buffer);
		memset(buffer, 0, sizeof(buffer));
	}

	if (credits->end_text_len == 0)
		fscanf(file, "%[^\n]\n", null_char);
	else
	{
		fscanf(file, "end_text:%d\n", &newlines_end);
		for (int i = 0; i != newlines_end + 1; i++)
		{
			static char line[200];
			fscanf(file, "%[^\n]\n", line);
			if (!(i == newlines_end))
				strcat(line, "\n");
			if (strlen(buffer) > 0)
				strcat(buffer, line);
			else
				strcpy(buffer, line);
		}
		memset(credits->end_text, 0, sizeof(credits->end_text));
		strcpy(credits->end_text, buffer);
	}
}
