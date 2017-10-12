# Documentation
## CCExtractor Graphical User Interface
### Code Structure:
```
src/GUI
├── activity.c             -Activity window definitions
├── activity.h             -Activity window declarations
├── ccextractorGUI.c       -Contains main()  and GUI code for 'Main' Tab + 'Menu'
├── ccextractorGUI.h       -Function and structure declarations
├── ccx_cli_thread.c       -All the functions (definitions) passed in threads
├── ccx_cli_thread.h       -Function, variables & structs declaration used in thread
├── command_builder.c      -Builds command to pass to CLI CCExtractor
├── command_builder.h      -Function, variables & structs declaration used
├── file_browser.c         -Function definition for File Browser
├── file_browser.h         -Function, struct & variable declaration
├── nuklear_lib            -Diretory contains Library Files
│   ├── nuklear_glfw_gl2.h -GLFW backend source header to interact with Nuklear
│   └── nuklear.h          -Nuklear library source code
├── popups.c               -Function definitions for all Popups used
├── popups.h               -Function & network struct declaration for all Popups
├── preview.c              -Preview window definitions
├── preview.h              -Preview window definitions
├── save_load_data.c       -Function definition to save last run state
├── save_load_data.h       -Function declaration to save last run state
├── stb_image.h            -Code to load images
├── tabs.c                 -Function definitions for all tabs except 'Main' tab
├── tabs.h                 -Function, variable and structure declarations
├── terminal.c             -Code for terminal Window
└── win_dirent.h           -Dirent API for Windows
```
### File by File functions:
	activity.c
[nk_begin](#nk-begin)(ctx, "Activity", nk_rect(x, y, width, height), NK_WINDOW_TITLE|NK_WINDOW_BACKGROUND);  
[nk_end](#nk-end)(ctx);  
[nk_layout_row_dynamic](#nk-layout-row-dynamic)(ctx, 40, 1);  
[nk_label_wrap](#nk-label-wrap)(ctx, [main_settings](#struct-main-tab)->activity_string[i]);  
[nk_window_is_closed](#nk-window-is-closed)(ctx, "Activity");  

	activity.h
int [activity](#func-activity)(struct [nk_context](#nk-context) &ast;ctx, int x, int y, int width, int height, struct [main_tab](#struct-main-tab) &ast;main_settings);

	ccextractorGUI.c
[nk_menubar_begin](#nk-menubar-begin)(ctx);  
[nk_layout_row_begin](#nk-layout-row-begin)(ctx, NK_STATIC, 30, 3);  
[nk_layout_row_push](#nk-layout-row-push)(ctx, 80);  
[nk_menu_begin_label](#nk-menu-begin-label)(ctx, "Preferences", NK_TEXT_LEFT, [nk_vec2](#nk-vec2)(120, 200));  
[nk_menu_end](#nk-menu-end)(ctx);  
[nk_menubar_end](#nk-menubar-end)(ctx);  
[nk_layout_space_begin](#nk-layout-space-begin)(ctx, NK_STATIC, 15, 1);  
[nk_layout_space_end](#nk-layout-space-end)(ctx);  
[nk_style_push_vec2](#nk-style-push-vec2)(ctx, &ctx->style.window.spacing, [nk_vec2(](#nk-vec2)0, 0));  
[nk_style_push_float](#nk-style-push-float)(ctx, &ctx->style.button.rounding, 0);  
[nk_button_label](#nk-label-button)(ctx, names[i]);  
[nk_style_pop_float](#nk-style-pop-float)(ctx);  
[nk_group_begin](#nk-group-begin)(ctx, "Advanced Tabs", NK_WINDOW_NO_SCROLLBAR);  
[nk_group_end](#nk-group-end)(ctx);  
[nk_layout_row](#nk-layout-row)(ctx, NK_DYNAMIC, 20, 3, ratio_adv_mode);  
[nk_spacing](#nk-spacing)(ctx, 1);  
[nk_checkbox_label](#nk-checkbox-label)(ctx, "Advanced Mode", &advanced_mode_check);  
[nk_option_label](#nk-option-label)(ctx, "Extract from files below:", [main_settings](#struct-main-tab).port_or_files == FILES));  
[nk_selectable_label](#nk-selectable-label)(ctx, [truncate_path_string](#func-truncate-path-string)([main_settings](#struct-main-tab).filenames[i]), NK_TEXT_LEFT, &[main_settings](#struct-main-tab).is_file_selected[i]);  
[nk_combo](#nk-combo)(ctx, [main_settings](#struct-main-tab).port_type, 2, [main_settings](#struct-main-tab).port_select, 20, [nk_vec2](#nk-vec2)_(85,100));  
[nk_label](#nk-label)(ctx, "Drag and Drop files for extraction.", NK_TEXT_CENTERED  
[nk_progress](#nk-progress)(ctx, &[main_settings](#struct-main-tab).progress_cursor, 101, nk_true);  

    ccextractorGUI.h
 void [setup_main_settings](#func-setup-main-settings)(struct main_tab &ast;main_settings);  
char&ast; [truncate_path_string](#func-truncate-path-string)(char &ast;filePath);  
void [remove_path_entry](#func-remove-path-entry)(struct [main_tab](#struct-main-tab) &ast;main_settings, int indexToRemove);  

    ccx_cli_thread.c || ccx_cli_thread.h
void&ast; [read_activity_data](#func-read-activity-data)(void &ast;read_args);  
void&ast; [read_data_from_thread](#func-read-data-from-thread)(void&ast; read_args);  
void&ast; [extract_thread](#func-extract-thread)(void&ast; extract_args);  
void&ast; [feed_files_for_extraction](#func-feed-files-for-extraction)(void&ast; file_args);  
void [setup_and_create_thread](#fun-setup-and-create-thread)(struct [main_tab](#struct-main-tab) &ast;main_settings, struct [built_string](#struct-built-string) &ast;command);  
void&asst; [find_hd_homerun_devices](#func-hd-homerun-devices)(void &ast;args);  
void&ast; [setup_hd_homerun_device](#func-setup-hd-homerun-device)(void &ast;args);  

    command_builder.c || command_builder.h
void [command_builder](#func-command-builder)(struct [built_string](#struct-built-string) &ast;command,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [main_tab](#struct-main-tab) &ast;main_settings,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [network_popup](#struct-network-popup) &ast;network_settings,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [input_tab](#struct-input-tab) &ast;input,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [advanced_input_tab](#struct-advanced-input-tab) &ast;advanced_input,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [output_tab](#struct-output-tab) &ast;output,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [decoders_tab](#struct-output-tab) &ast;decoders,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [credits_tab](#struct-output-tab) &ast;credits,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [debug_tab](#struct-debug-tab) &ast;debug,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [burned_subs_tab](#struct-debug-tab) &ast;burned_subs);  
		
    file_browser.c || file_browser.h
void [die](#func-die)(const char &ast;fmt, ...);  
char&ast; [file_load](#func-file-load)(const char&ast; path, size_t&ast; siz);  
char&ast; [str_duplicate](#func-str-duplicate)(const char &ast;src);  
void [dir_free_list](#func-dir-free-list)(char &ast;&ast;list, size_t size);  
char&ast;&ast; [dir_list](#func-dir-list)(const char &ast;dir, int return_subdirs, size_t &ast;count);  
struct file_group [FILE_GROUP](#func-file-group)(enum file_groups group, const char &ast;name, struct nk_image &ast;icon);  
struct file [FILE_DEF](#func-file-def)(enum file_types type, const char &ast;suffix, enum file_groups group);  
struct nk_image&ast; [media_icon_for_file](#func-media-icon-for-file)(struct media &ast;media, const char &ast;file);  
void [media_init](#func-media-init)(struct media &ast;media);  
void [file_browser_reload_directory_content](#func-file-browser-reload-directory-content)(struct file_browser &ast;browser, const char &ast;path);  
void [get_drives](#func-get-drives)(struct file_browser &ast;browser);  
void [file_browser_init](#func-file-browser-init)(struct file_browser &ast;browser, struct media &ast;media);  
void [file_browser_free](#func-file-browser-free)(struct file_browser &ast;browser);  
int [file_browser_run](#func-file-browser-run)(struct file_browser &ast;browser, struct [nk_context](#nk-context) &ast;ctx, struct [main_tab](#struct-main-tab) &ast;main_settings, struct [output_tab](#struct-output-tab) &ast;output, struct [debug_tab](#struct-debug-tab) &ast;debug, struct [hd_homerun_tab](#struct-hd-homerun-tab) &ast;hd_homerun);  

    popups.c || popups.h
void [draw_network_popup](#func-draw-network-popup)(struct [nk_context](#nk-context) &ast;ctx, int &ast;show_preferences_network, struct [network_popup](#struct-network-popup) &ast;network_settings);  
void [draw_getting_started_popup](#func-draw-getting-started-popup)(struct [nk_context](#nk-context) &ast;ctx, int &ast;show_getting_started);  
void [draw_about_ccx_popup](#func-draw-about-ccx-popup)(struct [nk_context](#nk-context) &ast;ctx, int &ast;show_about_ccx, struct nk_user_font &ast;droid_big, struct nk_user_font &ast;droid_head);  
void [draw_progress_details_popup](#func-draw-progress-details-popup)(struct [nk_context](#nk-context) &ast;ctx, int &ast;show_progress_details, struct [main_tab](#struct-main-tab) &ast;main_settings);  
void [draw_color_popup](#func-draw-color-popup)(struct [nk_context](#nk-context) &ast;ctx, struct [output_tab](#struct-output-tab) &ast;output);  
void [draw_thread_popup](#fun-draw-thread-popup)(struct [nk_context](#nk-context) &ast;ctx, int &ast;show_thread_popup);  
void [setup_network_settings](#func-setup-network-settings)(struct [network_popup](#struct-network-popup) &ast;network_settings);  

    preview.c || preview.h
int [preview](#func-preview)(struct [nk_context](#nk-context) &ast;ctx, int x, int y, int width, int height, struct [main_tab](#struct-main-tab) &ast;main_settings);  

    save_load_data.c || save_load_data.h
void [load_data](#func-load-data)(FILE *file,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [main_tab](#struct-main-tab) &ast;main_settings,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [input_tab](#struct-input-tab) &ast;input,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [advanced_input_tab](#struct-advanced-input-tab) &ast;advanced_input,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [output_tab](#struct-output-tab) &ast;output,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [decoders_tab](#struct-decoders-tab) &ast;decoders,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [credits_tab](#struct-credits-tab) &ast;credits,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [debug_tab](#struct-debug-tab) &ast;debug,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [hd_homerun_tab](#struct-hd-homerun-tab) &ast;hd_homerun,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [burned_subs_tab](#struct-burned-subs-tab) &ast;burned_subs);  

void [save_data](#func-save-data)(FILE *file,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [main_tab](#struct-main-tab) &ast;main_settings,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [input_tab](#struct-input-tab) &ast;input,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [advanced_input_tab](#struct-advanced-input-tab) &ast;advanced_input,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [output_tab](#struct-output-tab) &ast;output,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [decoders_tab](#struct-decoders-tab) &ast;decoders,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [credits_tab](#struct-credits-tab) &ast;credits,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [debug_tab](#struct-debug-tab) &ast;debug,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [hd_homerun_tab](#struct-hd-homerun-tab) &ast;hd_homerun,  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct [burned_subs_tab](#struct-burned-subs-tab) &ast;burned_subs);  

void [write_credits](#func-write-credits)(FILE* file, struct [credits_tab](#struct-credits-tab) &ast;credits);  
void [read_credits](#func-read-credits)(FILE* file, struct [credits_tab](#struct-credits-tab) &ast;credits);  

    terminal.c
int [terminal](#func-terminal)(struct [nk_context](#nk-context) &ast;ctx, int x, int y, int width, int height, char &ast;command);  

### About CCExtractor specific functions
#### int <a id="func-activity">activity</a>(struct nk_context &ast;ctx, int x, int y, int width, int height, struct [main_tab](#struct-main-tab) &ast;main_settings);
##### Info:
--Contains the procedure to be carried out when Activity Window is toggled.
##### Parameters:
* &ast;ctx - pointer to `nk_context` structure.
* x - X co-ordinate to draw Activity Window
* y - Y co-ordinate to draw Activty Window.
* width - width of window to draw.
* height - height of window to draw.
* &ast;main_settings - pointer to [`main_tab`](#struct-main-tab) structure.
##### Return Type: int
 * Returns non-zero value if window is not closed.
 * Returns zero if window is closed.
#### void <a id="func-setup-main-settings">setup_main_settings</a>(struct [main_tab](#struct-main-tab) &ast;main_settings);
##### Info:
Setups the required defaults of variables in [`main_tab`](#struct-main-tab) structure.
##### Parameters:
 * &ast;main_settings - pointer to [`main_tab`](#struct-main-tab) structure.
##### Return Type: void
#### char* <a id="func-truncate-path-string">truncate_path_string</a>(char &ast;filePath);
##### Info:
Truncated the Path String of file to visible area using `...`
##### Parameters:
 * &ast;filePath - Pointer to string to be truncated.
##### Return Type: *char
 * Returns pointer to truncated string. 
#### void <a id="func-remove-path-entry">remove_path_entry</a>(struct [main_tab](#struct-main-tab) &ast;main_settings, int indexToRemove);
##### Info:
Removes the selected path in the extraction queue (Selected entry's index is passed).
##### Parameters:
 * &ast;main_settings - pointer to [`main_tab`](#struct-main-tab) structure.
 * indexToRemove - index of the string to be removed from dynamic array of many strings.
##### Return Type: void

#### void&ast; <a id="func-read-activity-data">read_activity_data</a>(void &ast;read_args);
##### Info:
Reads activity data related to CCExtractor on `stdout`. And outputs to activity window (Updates variables that code of activity window uses).
##### Parameters:
 * &ast;read_args - Pointer to void, because thread functions don't allow any datatype as argument or return type. Therefore they are passed as void then typecasted later in the function.
##### Return type: void&ast;

#### void&ast; <a id="func-read-data-from-thread>read_data_from_thread</a>(void&ast; read_args);
##### Info:
Reads data from`--gui_mode_reports` redirected from `stderr` to a file. Reads the subtitles extracted in realtime and updates the variables for the same, updates the state of progress bar. Also, lanches [read_activity_data](#func-read-activity-data) in a new thread.
##### Parameters:
* &ast;read_args - Pointer to void, because thread functions don't allow any datatype as argument or return type. Therefore they are passed as void then typecasted later in the function.
##### Return type: void&ast;

#### void&ast; <a id="func-extract-thread">extract_thread</a>(void&ast; extract_args);
##### Info:
Passes command with all options from GUI to CLI CCExtractor.
##### Parameters:
 * &ast;extract_args - Pointer to void, because thread functions don't allow any datatype as argument or return type. Therefore they are passed as void then typecasted later in the function.
##### Return type: void&ast;

#### void&ast; <a id="func-feed-files-for-extraction">feed_files_for_extraction</a>(void&ast; file_args);
##### Info:
Feeds file by file to a new thread and waits until its extraction is done. This is done until all the files in extraction queue are extracted.
##### Parameters:
 * &ast;file_args - Pointer to void, because thread functions don't allow any datatype as argument or return type. Therefore they are passed as void then typecasted later in the function.
##### Return type: void&ast;

#### void <a id="func-setup-and-create-thread">setup_and_create_thread</a>(struct [main_tab](#struct-main-tab) &ast;main_settings, struct [built_string](#struct-built-string) &ast;command);
##### Info:
Initialises some values for the structure used in thread arguments and creates [feed_files_for_extraction](#feed-files-for-extraction).
##### Parameters:
 * &ast;main_settings - Pointer to `main_tab` struct.
 * &ast;command - Pointer to `built_string` struct.
##### Return type: void&ast;

#### void&ast; <a id="func-hind-hd-homerun-devices">find_hd_homerun_devices</a>(void &ast;args);
Finds devices connected to HD HomeRun Network.
#### Parameters:
 * &ast;args - Pointer to void, because thread functions don't allow any datatype as argument or return type. Therefore they are passed as void then typecasted later in the function.
#### Return type: void&ast;

#### void&ast; <a id="func-setup-hd-homerun-device">setup_hd_homerun_device</a>(void &ast;args);
##### Info:
Sets up various parameters required to extract subtitle from incoming stream from a HD HomeRun Device.
##### Parameters:
 * &ast;args - Pointer to void, because thread functions don't allow any datatype as argument or return type. Therefore they are passed as void then typecasted later in the function.
##### Return type: void&ast;

#### void [command_builder](#func-command-builder)(struct [built_string](#struct-built-string) &ast;command, struct [main_tab](#struct-main-tab) &ast;main_settings,	struct [network_popup](#struct-network-popup) &ast;network_settings, 	struct [input_tab](#struct-input-tab) &ast;input, struct [advanced_input_tab](#struct-advanced-input-tab) &ast;advanced_input, struct [output_tab](#struct-output-tab) &ast;output, struct [decoders_tab](#struct-output-tab) &ast;decoders,	struct [credits_tab](#struct-output-tab) &ast;credits,	struct [debug_tab](#struct-debug-tab) &ast;debug, struct [burned_subs_tab](#struct-debug-tab) &ast;burned_subs);
##### Info:
Fetches the options from the whole GUI and adds the respective CLI commands to the `term_string` in `built_string` struct.
##### Parameters:
 * &ast;command - Pointer to `built_string` command.
 * &ast;main_settings - Pointer to `main_tab` struct.
 * &ast;network_settings - Pointer to `network_popup` struct.
 * &ast;input - Pointer to `input_tab` struct.
 * &ast;advance_input - Pointer to `advanced_input` struct.
 * &ast;output - Pointer to `output_tab` struct.
 * &ast;decoders - Pointer to `decoders_tab` struct.
 * &ast;credits - Pointer to `credits_tab` struct.
 * &ast;debug - Pointer to `debug_tab` struct.
 * &ast;burned_subs - Pointer to `burned_subs_tab` struct.
##### Return type: void

#### void <a id="func-die">die</a>(const char &ast;fmt, ...);
##### Info:
Custom function to generate error if something in File Browser goes wrong.
##### Parameters:
 * &ast;fmt - Format of char string along with place holder for variables.
 * ... - Variables in order of their specified place holder.
##### Return type: void

#### char&ast; <a id="func-file-load">file_load</a>(const char&ast; path, size_t&ast; siz);
##### Info:
Custom function to load file and read data from loaded file.
##### Parameters:
 * &ast;path - Pointer to string literal (Path of the file).
 * &ast;siz - Size of string literal provided (To allocate memory accordingly).
##### Return type: void

#### char&ast; <a id="func-str-duplicate">str_duplicate</a>(const char &ast;src);
##### Info:
Dynamically copies specified string to memory.
##### Parameters:
 * &ast;src - The String to be copied.
##### Return type: char&ast;
 * Pointer to the string in the memory.

#### void <a id="func-dir-free-list">dir_free_list</a>(char &ast;&ast;list, size_t size);
##### Info:
Frees the memory allocated to Files' and Directories' name and path.
##### Parameters:
 * &ast;&ast;char - Pointer to list (array of strings) to be freed
##### Return type: void

#### char&ast;&ast; <a id="func-dir-list">dir_list</a>(const char &ast;dir, int return_subdirs, size_t &ast;count);
##### Info:
Opens the selected directory and adds its path to list and returns the same list.
#####Parameters:
 * &ast;dir - Pointer to string (name of directory to be opened).
 * return_subdirs - `nk_true` if subdirectories are to be returned then.
 * &ast;count - Number of directories in opened directories.
#### Retrun type: char&ast;&ast;
 * Pointer to List (Array of strings, name of directories and files) is returned.

#### struct file_group <a id="func-file-group">FILE_GROUP</a>(enum file_groups group, const char &ast;name, struct nk_image &ast;icon);
##### Info:
Initialises variables for `file_group` struct.
##### Parameters:
 * group - specifies to which group does the file belong to. Selected from `file_groups` enum, like `FILE_GROUP_MUSIC`.
 * &ast;name - Pointer to a string literal (to set `name` member in `file_group`.
 * &ast;icon - Pointer to `nk_image` struct (Holds attributes for loaded image file) to set to `icon`member of `file_group`.
##### Returnt type: struct `file_group`
 * Returns a `file_group` instance with set variables.

#### struct file <a id="func-file-def">FILE_DEF</a>(enum file_types type, const char &ast;suffix, enum file_groups group);
##### Info:
Initialises variables for `file` struct.
##### Parameters:
 * type - specifies which type does the file belong to. Selected from `file_types` enum, like `FILE_TEXT`.
 * &ast;suffix - Pointer to string( to set `suffix` member in `file`).
 * group - specifies to which group does the file belong to. Selected from `file_groups` enum, like `FILE_GROUP_MUSIC`.
##### Return type: struct `file`
 * Returns a `file` instance with set variables.

#### struct nk_image&ast; <a id="func-media-icon-for-file">media_icon_for_file</a>(struct media &ast;media, const char &ast;file);
##### Info:
Analyses the files and checks to which `file` or `file_group` they belong and assign appropriate icon to the file and returns the same.
##### Parameters:
* &ast;media - pointer to `media` struct.
* &ast;file - pointer to string literal (name of file with extension)
##### Return type: struct `nk_image`&ast;
 * Returns appropriate `nk_image` corresponding to the file.

#### void <a id="func-media-init">media_init</a>(struct media &ast;media);
##### Info:
Assigns icons to `file` and `file_group` members from.
##### Parameters:
 * &ast;media - pointer to `media` struct.
#### Return type: void

#### void <a is="func-file-browser-reload-directory-content">file_browser_reload_directory_content</a>(struct file_browser &ast;browser, const char &ast;path);
##### Info:
Updates various variables related to Files/Directories path and names when screen of File Browser reloads. (Due to clicking on a directory or any other button leading to different directory).
##### Parameters:
 * &ast;browser - Pointer to `file_browser` struct.
 * &ast;path - Path of the new directory whose contents are to be reloaded and showed on file browser.
##### Return type: void

#### void <a id="func-get-drives">get_drives</a>(struct file_browser &ast;browser);
##### Info:
NOTE: Windows Specific Function.
Detects the number of drives and their respective Drive Letters to show the same in File Browser.
#####Parameters:
 * &ast;browser - pointer to `file_browser` struct.
##### Return type: void

#### void <a id="func-file-browser-init">file_browser_init</a>(struct file_browser &ast;browser, struct media &ast;media);
##### Info:
Initialised various variables/attributes required whenever the File Browser is run.
##### Parameters:
 * &ast;browser - Pointer to `file_browser` struct.
 * &ast;media - pointer to `media` struct.
##### Return type: void

#### void <a id="func-file-browser-free">file_browser_free</a>(struct file_browser &ast;browser);
##### Info:
Frees the memory allocated to various variables in [file_browser_init](#func-file-browser-init).
##### Parameters:
 * &ast;browser - pointer to `file_browser` struct.
##### Return type: void

#### int <a id="func-file-browser-run">file_browser_run</a>(struct file_browser &ast;browser, struct [nk_context](#nk-context) &ast;ctx, struct [main_tab](#struct-main-tab) &ast;main_settings, struct [output_tab](#struct-output-tab) &ast;output, struct [debug_tab](#struct-debug-tab) &ast;debug, struct [hd_homerun_tab](#struct-hd-homerun-tab) &ast;hd_homerun);
##### Info:
Provides runtime of File Browser on GUI.
##### Parameters:
 * &ast;browser - pointer to `file_browser` struct.
 * &ast;ctx - pointer to `nk_context` struct.
 * &ast;main_settings - pointer to `main_tab` struct.
 * &ast;output - poiter to `output_tab` struct.
 * &ast;debug - pointer to `debug_tab` struct.
 * &ast;hd_homerun - pointer to `hd_homerun_tab` struct.
##### Return type: int
 * Returns `1` if any File name/path is copied to current variable.
 * Returns `0` otherwise.

#### void <a -d="func-draw-network-popup">draw_network_popup</a>(struct [nk_context](#nk-context) &ast;ctx, int &ast;show_preferences_network, struct [network_popup](#struct-network-popup) &ast;network_settings);
##### Info:
Draws popup with Network Settings on GUI.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
 * &ast;show_preferences_network - pointer to variable status if which triggers the popup.
 * &ast;network_settings - pointer to `network_popup` struct.
##### Return type: void

#### void <a id="func-draw-getting-started-popup">draw_getting_started_popup</a>(struct [nk_context](#nk-context) &ast;ctx, int &ast;show_getting_started);
##### Info: 
Draws popup on screen which shows Getting Started Info.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
 * &ast;show_getting_started - pointer to variable status if which triggers the popup.
##### Return type: void

#### void <a id="func-draw-about-ccx-popup">draw_about_ccx_popup</a>(struct [nk_context](#nk-context) &ast;ctx, int &ast;show_about_ccx, struct nk_user_font &ast;droid_big, struct nk_user_font &ast;droid_head);
##### Info:
Draws popup on screen containing information about CCExtractor.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
 * &ast;show_about_ccx - pointer to variable status if which triggers the popup.
 * &ast;droid_big - pointer to `nk_user_font` struct.
 * &ast;droid_head - pointer to `nk_user_font` struct.
##### Return type: void

#### void <a id="func-draw-progress-details-popup">draw_progress_details_popup</a>(struct [nk_context](#nk-context) &ast;ctx, int &ast;show_progress_details, struct [main_tab](#struct-main-tab) &ast;main_settings);
##### Info:
Draws popup on screen which shows progress details.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
 * &ast;show_pogress_details - pointer to variable status if which triggers the popup.
 * &ast;main_settings - pointer to `main_tab` struct.
##### Return type: void
#### void <a id="func-draw-color-popup">draw_color_popup</a>(struct [nk_context](#nk-context) &ast;ctx, struct [output_tab](#struct-output-tab) &ast;output);
##### Info:
Draws popup on screen which shows color-picker.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
 * &ast;output - pointer to `output_tab` struct.
##### Return type: void

#### void <a id="func-draw-thread-popup">draw_thread_popup</a>(struct [nk_context](#nk-context) &ast;ctx, int &ast;show_thread_popup);
##### Info:
This popup is shown if anyhow the GUI is unable to read file.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
 * &ast;show_thread_popup - pointer to variable status if which triggers the popup.
##### Return type: void

#### void <a id="func-setup-network-settings">setup_network_settings</a>(struct [network_popup](#struct-network-popup) &ast;network_settings);
##### Info:
Sets up defaults for Network Settings.
##### Parameters:
 * &ast;network_settings - pointer to `network_popup` struct.
##### Return type: void

#### int <a id="func-preview">preview</a>(struct [nk_context](#nk-context) &ast;ctx, int x, int y, int width, int height, struct [main_tab](#struct-main-tab) &ast;main_settings);
##### Info:
Draws `Preview` Nuklear window and shows preview strings (lines of subtitles extracted in realtime).
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
 * x - X co-ordinate from where to draw window.
 * y - Y co-ordinate from where to draw window.
 * width - width of window.
 * height - height of window.
 * &ast;main_settings - pointer to `main_tab ` struct.
##### Return type: 
 * Returns non-zero value if window is not closed.
 * Returns zero if window is closed.

#### void <a id="func-load-data">load_data</a>(FILE *file, struct [main_tab](#struct-main-tab) &ast;main_settings, struct [input_tab](#struct-input-tab) &ast;input, struct [advanced_input_tab](#struct-advanced-input-tab) &ast;advanced_input, struct [output_tab](#struct-output-tab) &ast;output, struct [decoders_tab](#struct-decoders-tab) &ast;decoders, struct [credits_tab](#struct-credits-tab) &ast;credits, struct [debug_tab](#struct-debug-tab) &ast;debug, struct [hd_homerun_tab](#struct-hd-homerun-tab) &ast;hd_homerun, struct [burned_subs_tab](#struct-burned-subs-tab) &ast;burned_subs);
##### Info:
Loads values of all the variables stored in a file at last exit of GUI.
##### Parameters:
 * &ast;file - pointer to `FILE`.
 * &ast;main_settings - pointer to `main_tab` struct.
 * &ast;intput - pointer to `input_tab` struct.
 * &ast;advanced_input - pointer to `advanced_input_tab` struct.
 * &ast;output - pointer to `output_tab` struct.
 * &ast;decoders - pointer to `decoders_tab` struct.
 * &ast;credits - poitner to `credits_tab` struct.
 * &ast;debug - pointer to `debug_tab` struct.
 * &ast;hd_homerun - pointer to `hd_homerun_tab` struct.
 * &ast;burned_subs - pointer to `burned_subs_tab` struct.
##### Return type: void

#### void <a id="func-save-data">save_data</a>(FILE *file, struct [main_tab](#struct-main-tab) &ast;main_settings, struct [input_tab](#struct-input-tab) &ast;input, struct [advanced_input_tab](#struct-advanced-input-tab) &ast;advanced_input, struct [output_tab](#struct-output-tab) &ast;output, struct [decoders_tab](#struct-decoders-tab) &ast;decoders, struct [credits_tab](#struct-credits-tab) &ast;credits, struct [debug_tab](#struct-debug-tab) &ast;debug, struct [hd_homerun_tab](#struct-hd-homerun-tab) &ast;hd_homerun, struct [burned_subs_tab](#struct-burned-subs-tab) &ast;burned_subs);
##### info:
Saves values of all the variables as a "Current State" in a file on exit.
##### Parameters:
 * &ast;file - pointer to `FILE`.
 * &ast;main_settings - pointer to `main_tab` struct.
 * &ast;intput - pointer to `input_tab` struct.
 * &ast;advanced_input - pointer to `advanced_input_tab` struct.
 * &ast;output - pointer to `output_tab` struct.
 * &ast;decoders - pointer to `decoders_tab` struct.
 * &ast;credits - poitner to `credits_tab` struct.
 * &ast;debug - pointer to `debug_tab` struct.
 * &ast;hd_homerun - pointer to `hd_homerun_tab` struct.
 * &ast;burned_subs - pointer to `burned_subs_tab` struct.
##### Return type: void

#### void <a id="func-write-credits">write_credits</a>(FILE &ast;file, struct [credits_tab](#struct-credits-tab) &ast;credits);
##### Info:
Writes Credits to files after some operations, since extra`\n` character gives problems while reading file.
##### Parameters:
 * &ast;file - pointer to `FILE`.
 * &ast;credits - pointer to `credits_tab` struct.
##### Return type: void

#### void <a id="func-read-credits">read_credits</a>(FILE* file, struct [credits_tab](#struct-credits-tab) &ast;credits);
##### Info:
Reads credits from file in a specific format (as written by [write_credits](#func-write-credits)) from file.
##### Parameters:
 * &ast;file - pointer to `FILE`.
 * &ast;credits - pointer to `credits_tab` struct.
##### Return type: void

#### int <a id="func-terminal">terminal</a>(struct [nk_context](#nk-context) &ast;ctx, int x, int y, int width, int height, char &ast;command);
##### Info:
Writes the command string (that would be passed to CLI CCExtractor) in "Terminal" Nuklear Window.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
 * x - X co-ordinate from where to draw the window.
 * y - Y co-ordinate from where to draw the window.
 * width - Width of the window.
 * height - height of the window.
 * &ast;command - String to write on window (the command to be passed).
##### Return type: int
 * Returns non-zero value if window is not closed.
 * Returns zero if window is closed.


### About CCExtractor specific Structures/Variables
#### <a id="struct-main-tab">main_tab</a>
##### Info:
Contains all the variables for `Main` tab.
##### Variables worth noting:
 * `int is_file_browser_active`
  * `nk_true` if File Browser is triggered by any event.
  * `nk_false` otherwise.
 * `int scaleWindowForFileBrowser`
  * Sets to `nk_true` if `is_file_browser_active` is `nk_true` to scale the `glfwWindow` to required size to accommodate File Browser.
  * Sets to `nk_false` otherwise.

#### <a id="struct-input-tab">input_tab</a>
##### Info:
Contains all variables to hold data of options selected/changed and view dynamically generated data to GUI in `Input` tab.

#### <a id="struct-advanced-input">advanced_input_tab</a>
Info: 
Contains all variables to hold data of options selected/changed and view dynamically generated data to GUI in `Advanced Input` tab.

#### <a id="struct-output-tab">output_tab</a>
#####Info:
Contains all variables to hold data of options selected/changed and view dynamically generated data to GUI in `Advanced Input` tab.

#### <a id="struct-decoders-tab">decoders_tab</a>
##### Info:
Contains all variables to hold data of options selected/changed and view dynamically generated data to GUI in `Decoders` tab.

#### <a id="struct-credits-tab">credits_tab</a>
##### Info:
Contains all variables to hold data of options selected/changed and view dynamically generated data to GUI in `Credits` tab.

#### <a id="struct-debug-tab">debug_tab</a>
##### Info:
Contains all variables to hold data of options selected/changed and view dynamically generated data to GUI in `Debug` tab.

#### <a id="struct-hd-homerun-tab">hd_homerun_tab</a>
##### Info:
Contains all variables to hold data of options selected/changed and view dynamically generated data to GUI in `HDHomeRun` tab.

#### <a id="struct-burned-subs-tab">burned_subs</a>
##### Info:
Contains all variables to hold data of options selected/changed and view dynamically generated data to GUI in `HDHomeRun` tab.

#### <a id="struct-network-popup">networ_popup</a>
##### Info:
Contains all the variables to store all the Network related options or showing them in GUI dynamically.

### About Nuklear Specific functions
#### int <a id="nk-begin">nk_begin</a>(struct nk_context&ast;, const char &ast;title, struct nk_rect bounds, nk_flags flags);
##### Info:
Draws a basic(and blank) window(Nuklear Window inside main GLFW window) to hold other Nuklear widgets.
##### Parameters:
 * nk_context&ast; - Pointer to `nk_context` structure.
 * &ast;title - Title for the so drawn Nuklear Window.
 * bounds - instance of `nk_rect` structure to hold co-ordinates, width and height of the Nuklear Window.
 * flags - Which flags to pass( from those contained in `enum flags`) to change behaviour of the Nuklear Window.
##### Return Type: int
 * Returns true if window creation is successful.
 * Returns false if window creation fails.

#### void <a id="nk-end">nk_end</a>(struct nk_context *ctx)
##### Info:
Marks the end of the Nuklear Window.
##### Parameter:
 * &ast;ctx - Pointer to `nk_context` structure.
##### Return type: void

#### void <a id="nk-layout-row-dynamic">nk_layout_row_dynamic</a>(struct nk_context&ast;, float height, int cols);
##### Info:
Used to define a dynamic row layout(to hold widgets), dynamic in the sense that the width is dynamically allocated to widgets.
##### Parameters:
 * &ast;nk_context - Pointer to `nk_context` structure.
 * height - height to set for widgets of that row.
 * cols - Columns to set for layout (generally the number of widgets to place).
##### Return Type: void

#### void <a id="nk-label-wrap">nk_label_wrap</a>(struct nk_context&ast;, const char&ast;);
##### Info:
Writes a label ( A plain String) and wraps it to the next line if the border of Nuklear Window, Group or Popup is reached.
*Note*: If the text wraps to next line, height for a new line must be considered while defining a layout, else the wrapped text won't be visible (but it will be there).
##### Parameters:
 * nk_context&ast; - Pointer to `nk_context` structure.
 * char&ast; - Pointer to string literal (to view).

#### int <a id="nk-window-is-closed">nk_window_is_closed</a>(struct nk_context &ast;ctx, const char &ast;name);
##### Info:
Checks if the active Nuklear Window is closed (by any trigger).
##### Parameters:
 * &ast;ctx - Pointer to `nk_context` structure.
 * &ast;name - Pointer to String literal(Name of window to check).
##### Return type: int
 * Returns true if window is closed (by any trigger).
 * Returns false of window is not closed.

#### void <a id="nk-menubar-begin">nk_menubar_begin</a>(struct nk_context &ast;ctx);
##### Info:
Marks the end of Menu Bar definition(Menubar code).
##### Parameters:
 * &ast;ctx - Pointer to `nk_context` structure.
##### Return type: void

#### void <a id="nk-layout-row-begin">nk_layout_row_begin</a>(struct nk_context *ctx, enum nk_layout_format fmt, float row_height, int cols);
##### Info:
Marks the beginning of custom layout. Which means, marking that layout has begun, now the widgets will be pushed row by row as per requirement using [nk_layout_row_push](#nk-layout-row-push).
##### Parameters:
 * &ast;ctx - Pointer to `nk_context` structure.
 * fmt - Layout format from provided formats (`enum nk_layout_format`), example - `NK_STATIC`, `NK_DYNAMIC`.
 * row_height - height of row pushed.
 * cols - Number of columns pushed in row.
##### Return type: void

#### void <a id="nk-layout-row-push">nk_layout_row_push</a>(struct nk_context&ast;, float value);
##### Info:
Pushes a row to hold widgets after defining the beginning of custom layout by [nk_layout_row_begin](#nk-layout-row-begin).
##### Parameters:
 * nk_context&ast; - Pointer to `nk_context` structure.
 * value - ratio or width of the widget to be pushed next.
##### Return Type: void

#### int <a id="nk-menu-begin-label">nk_menu_begin_label</a>(struct nk_context &ast;ctx, const char &ast;text, nk_flags align, struct [nk_vec2](#nk-vec2) size);
##### Info:
The label of the Menu Item to be pushed, for example - "Preferences" is marked by this function.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` structure.
 * &ast;text - pointer to string literal (Title of the Menu, example - "Settings").
 * align - alignment enumeration in `nk_flags`, example `NK_TEXT_LEFT`.
 * size - Size of label (as `nk_vec2` struct)
##### Return type: int
 * Returns true if label creation successful.
 * Returns false if label creation fails. 

#### void <a id="nk-menubar-end">nk_menubar_end</a>(struct nk_context&ast;);
##### Info:
Marks the end of the MenuBar definition.
##### Parameters:
 * nk_context&ast; - Pointer to `nk_context` structure.
##### Return type: void

#### void <a id="nk-layout-space-begin">nk_layout_space_begin</a>(struct nk_context &ast;ctx, enum nk_layout_format fmt, float height, int widget_count);
##### Info:
Marks the beginning of an empty space (Custom space for proper placement of widgets).
##### Parameters:
 * &ast;ctx - pointer to `nk_context` structure.
 * fmt - Layout format as in `enu nk_layout_format`, example - `NK_STATIC`, `NK_DYNAMIC`.
 * height = height of space to be added.
 * widget_count - Number of spaces to add.
##### Return type: void

#### void <a id="nk-layout-space-end">nk_layout_space_end</a>(struct nk_context &ast;ctx);
##### Info:
Marks the end of custom space (empty) definition.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` structure.
##### Return type: void

#### int <a id="nk-style-push-vec2">nk_style_push_vec2</a>(struct nk_context&ast; struct nk_vec2&ast;, struct nk_vec2);
##### Info:
Comes under `Style Stack`. Used to temporarily modify length, width, spacing related attributes of Styles of Nuklear Context.
##### Parameters:
 * nk_context&ast; - Pointer to `nk_context` structure.
 * nk_vec2&ast; - Pointer to attribute to be modified.
 * nk_vec2&ast; - New value in the form `nk_vec2(x, y)` as an instance of nk_vec2 structure.
##### Return type: int
 * Returns true if successful.
 * Returns false if unsuccessful.

#### int <a id="nk-style-push-float">nk_style_push_float</a>(struct nk_context&ast;, float&ast;, float);
##### Info:
Comes under `Style Stack`. Used to temporarily modify attributes requiring precision with floating point such as rounding value for buttons.
##### Parameters:
 * nk_context&ast; - Pointer to `nk_context` structure.
 * float&ast; - Pointer to variable whose value is to be changed.
 * float - new value to set.

#### int <a id="nk-button-label">nk_button_label</a>(struct nk_context&ast;, const char &ast;title);
##### Info:
Draws a Button with provided label.
##### Parameters:
 * nk_context&ast; - Pointer to `nk_context` struct.
 * &ast;title - Pointer to string literal (Label to put on button).
##### Return type: int
 * Returns true of Button is clicked.
 * Returns false of Button is in 'unclicked' state.

#### int <a id="nk-style-pop-float">nk_style_pop_float</a>(struct nk_context&ast;);
##### Info:
Pops the float values modified off the `Style Stack`. Which means, returns them to original state as they were before being modified by [nk_style_push_float](#nk-style-push-float).
##### Paramaters:
 * nk_context&ast; - Pointer to `nk_context` struct.
##### Return type: int
 * Returns true if successful.
 * Returns false if unsuccessful.

#### int <a id="nk-group-begin">nk_group_begin</a>(struct nk_context &ast;ctx, const char &ast;title, nk_flags flags);
##### Info:
Makes a group with given flags. Looks just like a window created by [nk_begin](#nk-begin) but can be created inside a window.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
 * &ast;title - string literal (Title of the group).
 * flags - All the required flags among available flags in `nk_flags`.
##### Return Type: int
 * Returns false if creation unsuccessful.
 * Returns true if creation successful. 

#### void <a id="nk-group-end">nk_group_end</a>(struct nk_context &ast;ctx);
##### Info:
Marks the end of the group created by [nk_group_begin](#nk-group-begin).
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
##### Return type: void

#### void <a id="nk-layout-row">nk_layout_row</a>(struct nk_context&ast;, enum nk_layout_format, float height, int cols, const float &ast;ratio);
##### Info:
Used to create custom row layout in which widget placement (including spacing) is done using ratios in floating point. Maximum ratio allowed is one. So, if there are two widgets (say buttons) need to placed in 50% available area each. Then `ratio` will be {0.5f, 0.5f}.
##### Parameters:
 * nk_context&ast; - pointer to `nk_context` struct.
 * nk_layout_format - format from available formats in `enum nk_layout_format` like `NK_STATIC` , `NK_DYNAMIC`.
 * height - height of the layout.
 * cols - Number of widgets(including spaces) to be used.
 * &ast;ratio - Ratio for widget placement.
##### Return type: void

#### void <a id="nk-spacing">nk_spacing</a>(struct nk_context&ast;, int cols);
##### Info:
Used to create spacing (blank) of specified columns.
##### Parameters:
 * nk_context&ast; - pointer to `nk_context` struct.
 * cols - Number of columns for which spacing has to be true.
##### Return type: void

#### int <a id="nk-checkbox-label">nk_checkbox_label</a>(struct nk_context &ast;ctx, const char &ast;label, int &ast;active);
##### Info:
Creates a checkbox with specified label.
##### Parameters:
 * &ast;ctx - Pointer to `nk_context` struct.
 * &ast; - Pointer to string literal(Label of checkbox).
 * &ast; - Pointer to variable to store the active value. `nk_false` if unchecked, `nk_true` if checked.
##### Return type: int
 * Returns false if unable to draw widget or old value of `*active` = new value of `*active`.
 * Returns true of old value of `*active` != new value of `*active`.

#### int <a id="nk-option-label">nk_option_label</a>(struct nk_context &ast;ctx, const char &ast;label, int active);
##### Info:
Draws radio button (among radio group) with specified label.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` struct.
 * &ast;label - Pointer to string literal (label of radio button).
 * active - Any check to specify if the radio button is active.
##### Return type: int
 * Returns true if radio button is active.
 * Returns false if radio button is inactive.

#### int <a id="nk-selectable-label">nk_selectable_label</a>(struct nk_context&ast;, const char&ast;, nk_flags align, int &ast;value);
##### Info:
Draws a selectable label. (Just like a regular [nk_label](#nk-label) but with a difference that it can be selected)
##### Parameters:
 * nk_context&ast; - pointer to `nk_context` struct.
 * char&ast; - Pointer to string literal (Label to display on GUI).
 * align - required alignment flags from `nk_flags` like `NK_TEXT_LEFT`.
 * &ast;value - Pointer to integer variable to store the value if the label is triggered or not.
	 * Sets to `nk_true` if label selected.
	 * Sets to `nk_false` if label is in unselected state.
##### Return type: int
* Returns false if unable to draw widget or old value of `*value` = new value of `*value`.
* Returns true of old value of `*value` != new value of `*value`.

#### int <a id="nk-combo">nk_combo</a>(struct nk_context&ast;, const char &ast;&ast;items, int count, int selected, int item_height, struct nk_vec2 size);
##### Info:
Draws combobox with given items as array of strings.
##### Parameters:
 * nk_context&ast; - Pointer to `nk_context` structure.
 * &ast;&ast;items - Array of strings of items to populate the list of combobox.
 * count - Number of items in the combobox.
 * selected - variable to store the index of selected item.
 * item_height - Height to allocate to each item in combobox.
 * size - size of combobox after expansion(when dropdown arrow is clicked). Given as [nk_vec2](#nk-vec2)(x, y).
##### Return type: int
 * Returns the index of selected item.

#### void <a id="nk-label">nk_label</a>(struct nk_context &ast;ctx, const char &ast;str, nk_flags alignment);
##### Info: 
Draws a plain text on Nuklear Window, Popup or group.
##### Parameters:
 * &ast;ctx - pointer to `nk_context` structure.
 * &ast;str - Pointer to string literal (Text to draw).
 * alignment - required flags for text alignment from `nk_flags`, like `NK_TEXT_LEFT`.
##### Return type: void

#### int <a id="nk-progress">nk_progress</a>(struct nk_context &ast;ctx, nk_size &ast;cur, nk_size max, int is_modifyable);
##### Info:
Draws a progress bar.
##### Parameters:
 * &ast;ctx - Poitner to `nk_context` struct.
 * &ast;cur - Realtime value to update in progress bar.
 * max - Maximum value `*cur` can achieve (usually 100, for 100% progress).
 * is_modifyable - 
  * `nk_true`  if progress bar can be modified with other events like mouse click and drag.
  * `nk_false` if progress bar needs to be modified only by value of `*cur`
##### Return type: int
* Returns false if unable to draw widget or old value of `*cur` = new value of `*cur`.
* Returns true of old value of `*cur` != new value of `*cur`.
 



### About Nuklear Specific Structures/Variables
#### <a id="nk-context">nk_context</a>
##### Info:
Contains various Variables/attributes related to current Window.

#### <a id="nk-vec2">nk_vec2</a>
##### Info:
A simple structure containing 2 variables `x` and `y`. Used for various purposes where 2 variables are required for example.. using offset for position or size of any widget/window.


