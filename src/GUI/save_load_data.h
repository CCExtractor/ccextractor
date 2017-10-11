#ifndef SAVE_LOAD_DATA_H
#define SAVE_LOAD_DATA_H

#include "ccextractorGUI.h"
#include "tabs.h"
#include "popups.h"
#include <stdio.h>

void load_data(FILE *file,
		struct main_tab* main_settings,
		struct input_tab* input,
		struct advanced_input_tab* advanced_input,
		struct output_tab* output,
		struct decoders_tab* decoders,
		struct credits_tab* credits,
		struct debug_tab* debug,
		struct hd_homerun_tab* hd_homerun,
		struct burned_subs_tab* burned_subs,
		struct network_popup* network_settings);

void save_data(FILE *file,
		struct main_tab* main_settings,
		struct input_tab* input,
		struct advanced_input_tab* advanced_input,
		struct output_tab* output,
		struct decoders_tab* decoders,
		struct credits_tab* credits,
		struct debug_tab* debug,
		struct hd_homerun_tab* hd_homerun,
		struct burned_subs_tab* burned_subs,
		struct network_popup* network_settings);

void write_credits(FILE* file, struct credits_tab* credits);
void read_credits(FILE* file, struct credits_tab* credits);
//Number of newlines in end_text
static int newlines_end;
//Number of newlines in start_text
static int newlines_start;

#endif //!SAVE_LOAD_DATA_H
