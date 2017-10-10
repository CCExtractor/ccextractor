#ifndef CCEXTRACTORGUI_H
#define CCEXTRACTORGUI_H

#ifndef NK_IMPLEMENTATION
#include "nuklear_lib/nuklear.h"
#endif // !NK_IMPLEMENTATION


struct main_tab
{
	enum {PORT, FILES} port_or_files;
	char port_num[8];
	int port_num_len;
	int is_check_common_extension;
	char **port_type;
	int port_select;
	char **filenames;
	int filename_count;
	int is_file_selected[1000];
	int is_file_browser_active;
	int scaleWindowForFileBrowser;
	nk_size progress_cursor;
	char** activity_string;
	int activity_string_count;
	char** preview_string;
	int preview_string_count;
	int threadPopup;
};

void setup_main_settings(struct main_tab *main_settings);
char* truncate_path_string(char *filePath);
void remove_path_entry(struct main_tab *main_settings, int indexToRemove);
char* concat(char* string1, char *string2);

#endif //!CCEXTRACTORGUI_H
