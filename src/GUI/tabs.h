#ifndef TABS_H
#define TABS_H

#include "ccextractorGUI.h"
/*Global variable for output tab*/
int show_color_from_picker;

/*Data containers for other functions*/

struct output_tab
{
	//General
	char **type;
	int type_select;
	int is_filename;
	char filename[260];
	int filename_len;
	int is_output_browser_active;
	int is_delay;
	char delay_sec_buffer[4];
	int is_export_xds;
	//Encoding
	enum { LATIN, UNIC, UTF } encoding;
	int is_bom;
	//Capitalization
	int is_cap_browser_active;
	int is_cap_standard;
	int is_cap_file;
	char cap_dictionary[260];
	int cap_dictionary_len;
	//Line Endings
	enum { CRLF, LF } line_ending;
	//Colors and Styles
	int is_center;
	int is_dash;
	int no_typesetting;
	enum { NO_COLOR, DEFAULT_COLOR } font_color;
	int color_popup;
	char color_hex[7];
	struct nk_color color_rgb;
	//Roll-up Captions
	enum {ONETIME, REALTIME} onetime_or_realtime;
	char **roll_limit;
	int roll_limit_select;
};

struct time {
	int hours, minutes, seconds;
};


struct input_tab {
	//General
	char **type;
	int type_select;
	int is_split;
	int is_live_stream;
	char wait_data_sec[4];
	int wait_data_sec_len;
	//Timing
	int is_process_from;
	int is_process_until;
	char from_time_buffer[10];
	char until_time_buffer[10];
	//Elementary Stream
	enum {AUTO_DETECT,STREAM_TYPE,STREAM_PID} elementary_stream;
	int is_assume_mpeg;
	char stream_type[10];
	int stream_type_len;
	char stream_pid[10];
	int stream_pid_len;
	char mpeg_type[10];
	int mpeg_type_len;
	//Teletext
	enum { AUTO_DECODE, FORCE, DISABLE } teletext_decoder;
	int is_process_teletext_page;
	char teletext_page_number[6];
	int teletext_page_numer_len;
	//Screenfuls limit
	enum { NO_LIMIT, LIMITED } is_limit;
	char screenful_limit_buffer[4];
	//Clock
	enum { AUTO, GOP, PTS } clock_input;
};



struct advanced_input_tab{

	//Multiple Programs
	int is_multiple_program;
	enum { FIRST_PROG, PROG_NUM } multiple_program;
	char prog_number[6];
	int prog_number_len;

	//Myth TV
	enum { AUTO_MYTH, FORCE_MYTH, DISABLE_MYTH } set_myth;

	//Miscellaneous
	int is_mpeg_90090;
	int is_padding_0000;
	int is_order_ccinfo;
	int is_win_bug;
	int is_hauppage_file;
	int is_process_mp4;
	int is_ignore_broadcast;

};

struct debug_tab{
	int is_elementary_stream;
	char elementary_stream[260];
	int elementary_stream_len;
	int is_dump_packets;
	int is_debug_608;
	int is_debug_708;
	int is_stamp_output;
	int is_debug_analysed_vid;
	int is_raw_608_708;
	int is_debug_parsed;
	int is_disable_sync;
	int is_no_padding;
	int is_debug_xds;
	int is_output_pat;
	int is_output_pmt;
	int is_scan_ccdata;
	int is_output_levenshtein;
	//File browser trigger
	int is_debug_browser_active;
};


struct burned_subs_tab{
	int is_burned_subs;
	enum {PRESET, CUSTOM} color_type;
	char** subs_color;
	int subs_color_select;
	char custom_hue[4];
	int custom_hue_len;
	enum {FRAME_WISE, WORD_WISE, LETTER_WISE} ocr_mode;
	char min_duration[4];
	int min_duration_len;
	int luminance_threshold;
	int confidence_threshold;
	int is_italic;
};

struct decoders_tab{
	//608 DECODER
	int is_field1;
	int is_field2;
	enum { CHANNEL_1, CHANNEL_2 } channel;

	//708 DECODER
	int is_708;
	char services[15];
	int services_len;

	//Teletext or DVB
	enum { TELETEXT, DVB } teletext_dvb;

	//Teletext line Duplication
	char min_distance[4];
	int min_distance_len;
	char max_distance[4];
	int max_distance_len;
};

struct credits_tab{
	//START CREDITS
	int is_start_text;
	char start_text[1000];
	int start_text_len;
	int is_before;
	int is_after;
	char before_time_buffer[10];
	char after_time_buffer[10];
	char start_atmost_sec[4];
	int start_atmost_sec_len;
	char start_atleast_sec[4];
	int start_atleast_sec_len;

	//END CREDITS
	int is_end_text;
	char end_text[1000];
	int end_text_len;
	char end_atmost_sec[4];
	int end_atmost_sec_len;
	char end_atleast_sec[4];
	int end_atleast_sec_len;
};

struct hd_homerun_tab{
	char location[260];
	int location_len;
	int device_select[50];
	int device_num;
	char **devices;
	int is_homerun_browser_active;
	char tuner[2];
	int tuner_len;
	char channel[5];
	int channel_len;
	char program[10];
	int program_len;
	char ipv4_address[16];
	int ipv4_address_len;
	char port_number[8];
	int port_number_len;
	int selected;
	int threadPopup;
};

/*Tab Functions*/
void setup_output_tab(struct output_tab *output);
void setup_decoders_tab(struct decoders_tab *decoders);
void setup_credits_tab(struct credits_tab *credits);
void setup_input_tab(struct input_tab *input);
void setup_advanced_input_tab(struct advanced_input_tab *advaned_input);
void setup_debug_tab(struct debug_tab *debug);
void setup_hd_homerun_tab(struct hd_homerun_tab *hd_homerun);
void setup_burned_subs_tab(struct burned_subs_tab *burned_subs);

void draw_input_tab(struct nk_context *ctx, int *tab_screen_height, struct input_tab *input,
		struct decoders_tab *decoders);
void draw_advanced_input_tab(struct nk_context *ctx, int *tab_screen_height,
		struct advanced_input_tab *advaned_input);
void draw_output_tab(struct nk_context *ctx, int *tab_screen_height, struct output_tab *output,
		struct main_tab *main_settings);
void draw_decoders_tab(struct nk_context *ctx, int *tab_screen_height, struct decoders_tab *decoders);
void draw_credits_tab(struct nk_context *ctx, int *tab_screen_height, struct credits_tab *credits);
void draw_debug_tab(struct nk_context *ctx, int *tab_screen_height,
		struct main_tab *main_settings,
		struct output_tab *output,
		struct debug_tab *debug);
void draw_hd_homerun_tab(struct nk_context *ctx, int *tab_screen_height,
		struct hd_homerun_tab *hd_homerun,
		struct main_tab *main_settings);
void draw_burned_subs_tab(struct nk_context *ctx, int *tab_screen_height, struct burned_subs_tab *burned_subs);

#endif //!TABS_H
