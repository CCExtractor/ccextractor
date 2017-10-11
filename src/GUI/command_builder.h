#ifndef COMMAND_BUILDER_H
#define COMMAND_BUILDER_H

#include "ccextractorGUI.h"
#include "tabs.h"
#include "popups.h"

struct built_string
{
	char term_string[1000];
};

void command_builder(struct built_string *command,
		struct main_tab *main_settings,
		struct network_popup *network_settings,
		struct input_tab *input,
		struct advanced_input_tab *advanced_input,
		struct output_tab *output,
		struct decoders_tab *decoders,
		struct credits_tab *credits,
		struct debug_tab *debug,
		struct burned_subs_tab *burned_subs);

#endif //!COMMAND_BUILDER_H
