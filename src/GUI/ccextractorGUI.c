/* nuklear - v1.32.0 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define PATH_LENGTH 66
#define NAME_LENGTH 56
#define PREFIX_LENGTH_TRUNCATED 10

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL2_IMPLEMENTATION
#include "nuklear_lib/nuklear.h"
#include "nuklear_lib/nuklear_glfw_gl2.h"

#include "icon_data.c"

//#define WINDOW_WIDTH 1200
//#define WINDOW_HEIGHT 800
//#define true 1
//#define false 0
//#define UNUSED(a) (void)a
//#define MIN(a,b) ((a) < (b) ? (a) : (b))
//#define MAX(a,b) ((a) < (b) ? (b) : (a))
//#define LEN(a) (sizeof(a)/sizeof(a)[0])
#include "ccextractorGUI.h"
#include "tabs.h"
#include "activity.h"
#include "terminal.c"
#include "preview.h"
#include "popups.h"
#include "command_builder.h"
#include "ccx_cli_thread.h"
#include "file_browser.h"
#include "save_load_data.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static struct main_tab main_settings;

/*Trigger command for CLI*/
char command[20];

/*Global Variables for Drag and Drop files*/

/* Width and Height of all frames*/
const GLint WIDTH_mainPanelAndWindow = 530, HEIGHT_mainPanelandWindow = 550;
const GLint WIDTH_termORPreviewPanel = 530, HEIGHT_termORPreviewPanel = 100;
const GLint WIDTH_termANDPreviewPanel = 400, HEIGHT_termANDPreviewPanel = 650;
const GLint WIDTH_activityPanel = 400, HEIGHT_activityPanelSolo = 550, HEIGHT_activityPanelDuo = 650;
const GLint WIDTH_mainTermORPreviewWindow = 530, HEIGHT_mainORPreviewTermWindow = 650;
const GLint WIDTH_mainTermANDPreviewWindow = 930, HEIGHT_mainTermAndPreviewWindow = 650;
const GLint WIDTH_mainActivityWindow = 930, HEIGHT_mainActivityWindowSolo = 550, HEIGHT_mainActivityWindowDuo = 650;

/*Tab constants*/
static int tab_screen_height;

/*Parameter Constants*/
static int modifiedParams = 0;

static void error_callback(int e, const char *d)
{
	printf("Error %d: %s\n", e, d);
}

void drop_callback(GLFWwindow *window, int count, const char **paths)
{
	int i, j, k, z, copycount, prefix_length, slash_length, fileNameTruncated_index;

	printf("Number of selected paths:%d\n", count);

	if (main_settings.filename_count == 0 && main_settings.filenames == NULL)
		main_settings.filenames = (char **)calloc(count + 1, sizeof(char *));
	else
		main_settings.filenames = (char **)realloc(main_settings.filenames, (main_settings.filename_count + count + 1) * sizeof(char *));
	for (i = 0; i < count; i++)
	{
		printf("\n%d", main_settings.filename_count);

		main_settings.filenames[main_settings.filename_count] = (char *)calloc((strlen(paths[i]) + 5), sizeof(char));
		main_settings.filenames[main_settings.filename_count][0] = '\"';
		strcat(main_settings.filenames[main_settings.filename_count], paths[i]);
		strcat(main_settings.filenames[main_settings.filename_count], "\"");
		puts(main_settings.filenames[main_settings.filename_count]);
		main_settings.filename_count++;
	}
	main_settings.filenames[main_settings.filename_count] = NULL;
}

/*Rectangle to hold file names*/
// void draw_file_rectangle_widget(struct nk_context *ctx, struct nk_font *font)
//{
//	struct nk_command_buffer *canvas;
//	struct nk_input *input = &ctx->input;
//	canvas = nk_window_get_canvas(ctx);
//
//	struct nk_rect space;
//	enum nk_widget_layout_states state;
//	state = nk_widget(&space, ctx);
//	if (!state) return;
//
//	/*if (state != NK_WIDGET_ROM)
//	update_your_widget_by_user_input(...);*/
//	nk_fill_rect(canvas, space, 5, nk_rgb(88, 81, 96));
//	if (!strcmp(filePath[0], "\0")) {
//		space.y = space.y + (space.h / 2) -10;
//		space.x = space.x + 90;
//		nk_draw_text(canvas, space, "Drag and Drop files here for Extraction.", 40, &font->handle, nk_rgb(88, 81, 96), nk_rgb(0, 0, 0));
//	}
//	else {
//		for (int i = 0; i < fileCount; i++)
//		{
//			nk_draw_text(canvas, space, filePath[i], strlen(filePath[i]), &font->handle, nk_rgb(88, 81, 96), nk_rgb(0, 0, 0));
//			space.y = space.y + 20;
//		}
//	}
//
// }

/*Rectangle to hold extraction info*/
// void draw_info_rectangle_widget(struct nk_context *ctx, struct nk_font *font)
//{
//	struct nk_command_buffer *canvas;
//	struct nk_input *input = &ctx->input;
//	canvas = nk_window_get_canvas(ctx);
//
//	struct nk_rect space;
//	enum nk_widget_layout_states state;
//	state = nk_widget(&space, ctx);
//	if (!state) return;
//
//	/*if (state != NK_WIDGET_ROM)
//	update_your_widget_by_user_input(...);*/
//	nk_fill_rect(canvas, space, 5, nk_rgb(88, 81, 96));
//	space.x = space.x + 3;
//	nk_draw_text(canvas, space, "Input Type: Auto", 16, &font->handle, nk_rgb(88, 81, 96), nk_rgb(0, 0, 0));
//	space.y = space.y + 20;
//	nk_draw_text(canvas, space, "Output Type: Default(.srt)", 26, &font->handle, nk_rgb(88, 81, 96), nk_rgb(0, 0, 0));
//	space.y = space.y + 20;
//	nk_draw_text(canvas, space, "Output Path: Default", 20, &font->handle, nk_rgb(88, 81, 96), nk_rgb(0, 0, 0));
//	space.y = space.y + 20;
//	nk_draw_text(canvas, space, "Hardsubs Extraction: Yes", 24, &font->handle, nk_rgb(88, 81, 96), nk_rgb(0, 0, 0));
// }

int main(void)
{

	// Platform
	static GLFWwindow *win;
	struct nk_context *ctx;
	int screenWidth, screenHeight;
	// int winWidth, winHeight;

	// GLFW
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
	{
		fprintf(stdout, "GLFW failed to initialise.\n");
	}
	win = glfwCreateWindow(WIDTH_mainPanelAndWindow, HEIGHT_mainPanelandWindow, "CCExtractor", NULL, NULL);
	if (win == NULL)
		printf("Window Could not be created!\n");
	glfwMakeContextCurrent(win);
	glfwSetWindowSizeLimits(win, WIDTH_mainPanelAndWindow, HEIGHT_mainPanelandWindow, WIDTH_mainPanelAndWindow, HEIGHT_mainPanelandWindow);
	glfwSetWindowUserPointer(win, &ctx);
	glfwSetDropCallback(win, drop_callback);

	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Failed to setup GLEW\n");
		exit(1);
	}

	// GUI

	struct file_browser browser;
	static const struct file_browser reset_browser;
	struct media media;

	ctx = nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);
	struct nk_font_atlas *font_atlas;
	nk_glfw3_font_stash_begin(&font_atlas);
	struct nk_font *droid = nk_font_atlas_add_from_memory(font_atlas, roboto_regular_font, sizeof(roboto_regular_font), 16, 0);
	struct nk_font *droid_big = nk_font_atlas_add_from_memory(font_atlas, roboto_regular_font, sizeof(roboto_regular_font), 25, 0);
	struct nk_font *droid_head = nk_font_atlas_add_from_memory(font_atlas, roboto_regular_font, sizeof(roboto_regular_font), 20, 0);
	nk_glfw3_font_stash_end();
	nk_style_set_font(ctx, &droid->handle);

	// CHECKBOX VALUES
	static int show_terminal_check = nk_false;
	static int show_preview_check = nk_false;
	static int show_activity_check = nk_false;
	static int advanced_mode_check = nk_false;
	static int file_extension_check = nk_true;

	/*Settings and tab options*/
	setup_main_settings(&main_settings);
	static struct network_popup network_settings;
	setup_network_settings(&network_settings);
	static struct output_tab output;
	setup_output_tab(&output);
	static struct decoders_tab decoders;
	setup_decoders_tab(&decoders);
	static struct credits_tab credits;
	setup_credits_tab(&credits);
	static struct input_tab input;
	setup_input_tab(&input);
	static struct advanced_input_tab advanced_input;
	setup_advanced_input_tab(&advanced_input);
	static struct debug_tab debug;
	setup_debug_tab(&debug);
	static struct hd_homerun_tab hd_homerun;
	setup_hd_homerun_tab(&hd_homerun);
	static struct burned_subs_tab burned_subs;
	setup_burned_subs_tab(&burned_subs);
	static struct built_string command;

	/* icons */

	media.icons.home = icon_load(home_icon_data, sizeof(home_icon_data));
	media.icons.directory = icon_load(directory_icon_data, sizeof(directory_icon_data));
	media.icons.computer = icon_load(computer_icon_data, sizeof(computer_icon_data));
#ifdef _WIN32
	media.icons.drives = icon_load(drive_icon_data, sizeof(drive_icon_data));
#endif
	media.icons.desktop = icon_load(desktop_icon_data, sizeof(desktop_icon_data));
	media.icons.default_file = icon_load(default_icon_data, sizeof(default_icon_data));
	media.icons.text_file = icon_load(text_icon_data, sizeof(text_icon_data));
	media.icons.music_file = icon_load(music_icon_data, sizeof(music_icon_data));
	media.icons.font_file = icon_load(font_icon_data, sizeof(font_icon_data));
	media.icons.img_file = icon_load(img_icon_data, sizeof(img_icon_data));
	media.icons.movie_file = icon_load(movie_icon_data, sizeof(movie_icon_data));

	media_init(&media);

	file_browser_init(&browser, &media);

	/*Read Last run state*/
	FILE *loadFile;
	loadFile = fopen("ccxGUI.ini", "r");
	if (loadFile != NULL)
	{
		printf("File found and reading it!\n");
		load_data(loadFile, &main_settings, &input, &advanced_input, &output, &decoders, &credits, &debug, &hd_homerun, &burned_subs, &network_settings);
		fclose(loadFile);
	}

	/*Main GUI loop*/
	while (nk_true)
	{
		if (glfwWindowShouldClose(win))
		{
			FILE *saveFile;
			saveFile = fopen("ccxGUI.ini", "w");
			save_data(saveFile, &main_settings, &input, &advanced_input, &output, &decoders, &credits, &debug, &hd_homerun, &burned_subs, &network_settings);
			fclose(saveFile);
			break;
		}
		// Input
		glfwPollEvents();
		nk_glfw3_new_frame();

		// Popups
		static int show_progress_details = nk_false;
		static int show_about_ccx = nk_false;
		static int show_getting_started = nk_false;

		// GUI
		if (nk_begin(ctx, "CCExtractor", nk_rect(0, 0, WIDTH_mainPanelAndWindow, HEIGHT_mainPanelandWindow),
			     NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND))
		{

			// MENUBAR
			nk_menubar_begin(ctx);
			nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
			nk_layout_row_push(ctx, 80);
			if (nk_menu_begin_label(ctx, "Preferences", NK_TEXT_LEFT, nk_vec2(120, 200)))
			{
				nk_layout_row_dynamic(ctx, 30, 1);
				if (nk_menu_item_label(ctx, "Reset Defaults", NK_TEXT_LEFT))
				{
					remove("ccxGUI.ini");
					setup_main_settings(&main_settings);
					setup_network_settings(&network_settings);
					setup_output_tab(&output);
					setup_decoders_tab(&decoders);
					setup_credits_tab(&credits);
					setup_input_tab(&input);
					setup_advanced_input_tab(&advanced_input);
					setup_debug_tab(&debug);
					setup_hd_homerun_tab(&hd_homerun);
					setup_burned_subs_tab(&burned_subs);
				}
				if (nk_menu_item_label(ctx, "Network Settings", NK_TEXT_LEFT))
					network_settings.show_network_settings = nk_true;
				nk_menu_end(ctx);
			}
			nk_layout_row_push(ctx, 70);
			if (nk_menu_begin_label(ctx, "Windows", NK_TEXT_LEFT, nk_vec2(120, 200)))
			{
				nk_layout_row_dynamic(ctx, 30, 1);
				nk_checkbox_label(ctx, "Activity", &show_activity_check);
				nk_checkbox_label(ctx, "Terminal", &show_terminal_check);
				nk_checkbox_label(ctx, "Preview", &show_preview_check);
				nk_menu_end(ctx);
			}
			nk_layout_row_push(ctx, 45);
			if (nk_menu_begin_label(ctx, "Help", NK_TEXT_LEFT, nk_vec2(120, 200)))
			{
				nk_layout_row_dynamic(ctx, 30, 1);
				if (nk_menu_item_label(ctx, "Getting Started", NK_TEXT_LEFT))
					show_getting_started = nk_true;
				if (nk_menu_item_label(ctx, "About CCExtractor", NK_TEXT_LEFT))
					show_about_ccx = nk_true;
				nk_menu_end(ctx);
			}

			// Network Settings
			if (network_settings.show_network_settings)
				draw_network_popup(ctx, &network_settings);

			// About CCExtractor Popup
			if (show_about_ccx)
				draw_about_ccx_popup(ctx, &show_about_ccx, &droid_big->handle, &droid_head->handle);

			// Getting Started
			if (show_getting_started)
				draw_getting_started_popup(ctx, &show_getting_started);

			// Color Popup
			if (output.color_popup)
				draw_color_popup(ctx, &output);

			// File Browser as Popup
			if (main_settings.scaleWindowForFileBrowser)
			{
				int width = 0, height = 0;
				glfwGetWindowSize(win, &width, &height);
				glfwSetWindowSize(win, 930, 650);
				glfwSetWindowSizeLimits(win, 930, 650, 930, 650);
				file_browser_run(&browser, ctx, &main_settings, &output, &debug, &hd_homerun);
			}

			// Thread popop when file can't be read
			if (main_settings.threadPopup)
				draw_thread_popup(ctx, &main_settings.threadPopup);

			// Thread popup for hd_homerun thread
			if (hd_homerun.threadPopup)
				draw_thread_popup(ctx, &hd_homerun.threadPopup);

			nk_layout_row_end(ctx);
			nk_menubar_end(ctx);
			nk_layout_space_begin(ctx, NK_STATIC, 15, 1);
			nk_layout_space_end(ctx);

			/*TABS TRIGGERED IN ADVANCED MODE FLAG*/
			if (advanced_mode_check)
			{
				static int current_tab = 0;
				enum tab_name
				{
					MAIN,
					INPUT,
					ADV_INPUT,
					OUTPUT,
					DECODERS,
					CREDITS,
					DEBUG,
					HDHOMERUN,
					BURNEDSUBS
				};
				const char *names[] = {"Main", "Input", "Advanced Input", "Output", "Decoders", "Credits", "Debug", "HDHomeRun", "BurnedSubs"};
				float id = 0;
				int i;

				nk_style_push_vec2(ctx, &ctx->style.window.spacing, nk_vec2(0, 0));
				nk_style_push_float(ctx, &ctx->style.button.rounding, 0);
				nk_layout_row_begin(ctx, NK_STATIC, 20, 9);
				for (i = 0; i < 9; ++i)
				{
					/*Make sure button perfectly fits text*/
					const struct nk_user_font *f = ctx->style.font;
					float text_width = f->width(f->userdata, f->height, names[i], nk_strlen(names[i]));
					float widget_width = text_width + 3 * ctx->style.button.padding.x;
					nk_layout_row_push(ctx, widget_width);
					if (current_tab == i)
					{
						/*Active tab gets highlighted*/
						struct nk_style_item button_color = ctx->style.button.normal;
						ctx->style.button.normal = ctx->style.button.active;
						current_tab = nk_button_label(ctx, names[i]) ? i : current_tab;
						ctx->style.button.normal = button_color;
					}
					else
						current_tab = nk_button_label(ctx, names[i]) ? i : current_tab;
				}
				nk_style_pop_float(ctx);
				/*Body*/

				nk_layout_row_dynamic(ctx, tab_screen_height, 1);
				if (nk_group_begin(ctx, "Advanced Tabs", NK_WINDOW_NO_SCROLLBAR))
				{
					nk_style_pop_vec2(ctx);
					switch (current_tab)
					{
						case MAIN:
							tab_screen_height = 0;
							break;

						case INPUT:
							draw_input_tab(ctx, &tab_screen_height, &input, &decoders);
							break;

						case ADV_INPUT:
							draw_advanced_input_tab(ctx, &tab_screen_height, &advanced_input);
							break;

						case OUTPUT:
							draw_output_tab(ctx, &tab_screen_height, &output, &main_settings);
							break;

						case DECODERS:
							draw_decoders_tab(ctx, &tab_screen_height, &decoders);
							break;

						case CREDITS:
							draw_credits_tab(ctx, &tab_screen_height, &credits);
							break;

						case DEBUG:
							draw_debug_tab(ctx, &tab_screen_height, &main_settings, &output, &debug);
							break;

						case HDHOMERUN:
							draw_hd_homerun_tab(ctx, &tab_screen_height, &hd_homerun, &main_settings);
							break;

						case BURNEDSUBS:
							draw_burned_subs_tab(ctx, &tab_screen_height, &burned_subs);
							break;
					}
					nk_group_end(ctx);
				}
				else
					nk_style_pop_vec2(ctx);
			}

			// ADVANCED MODE FLAG
			static const float ratio_adv_mode[] = {0.75f, 0.22f, .03f};
			nk_layout_row(ctx, NK_DYNAMIC, 20, 3, ratio_adv_mode);
			nk_spacing(ctx, 1);
			nk_checkbox_label(ctx, "Advanced Mode", &advanced_mode_check);

			// RADIO BUTTON 1
			static const float ratio_button[] = {.10f, .90f};
			static const float check_extension_ratio[] = {.10f, .53f, .12f, .15f, .10f};
			// static int op = FILES;
			nk_layout_row(ctx, NK_DYNAMIC, 20, 2, ratio_button);
			nk_spacing(ctx, 1);
			if (nk_option_label(ctx, "Extract from files below:", main_settings.port_or_files == FILES))
			{
				// op = FILES;
				main_settings.port_or_files = FILES;
			}

			// CHECKBOX FOR FILE TYPES
			static int add_remove_button = nk_false;
			nk_layout_row(ctx, NK_DYNAMIC, 20, 5, check_extension_ratio);
			nk_spacing(ctx, 1);
			nk_checkbox_label(ctx, "Check for common video file extensions", &file_extension_check);
			if (main_settings.filename_count > 0)
			{
				if (nk_button_label(ctx, "Add"))
				{
					main_settings.is_file_browser_active = nk_true;
					main_settings.scaleWindowForFileBrowser = nk_true;
				}

				for (int i = 0; i < main_settings.filename_count; i++)
				{
					if (main_settings.is_file_selected[i])
					{
						add_remove_button = nk_true;
						break;
					}
					else
						add_remove_button = nk_false;
				}
				if (add_remove_button)
				{

					if (nk_button_label(ctx, "Remove"))
					{
						for (int i = main_settings.filename_count - 1; i != -1; i--)
							if (main_settings.is_file_selected[i])
							{
								remove_path_entry(&main_settings, i);
								main_settings.is_file_selected[i] = nk_false;
							}
					}
				}

				else if (nk_button_label(ctx, "Clear"))
				{
					free(main_settings.filenames);
					main_settings.filename_count = 0;
				}
			}

			// RECTANGLE-FILES
			static const float ratio_rect_files[] = {0.10f, 0.80f};
			nk_layout_row(ctx, NK_DYNAMIC, 180, 2, ratio_rect_files);
			nk_spacing(ctx, 1);
			if (nk_group_begin(ctx, "Files in extraction queue:", NK_WINDOW_BORDER | NK_WINDOW_TITLE))
			{
				if (main_settings.filename_count != 0)
				{
					int i = 0;
					nk_layout_row_static(ctx, 18, 380, 1);
					for (i = 0; i < main_settings.filename_count; ++i)
						nk_selectable_label(ctx, truncate_path_string(main_settings.filenames[i]), NK_TEXT_LEFT, &main_settings.is_file_selected[i]);
				}

				else
				{
					nk_layout_row_dynamic(ctx, 1, 1);
					nk_spacing(ctx, 1);
					nk_layout_row_dynamic(ctx, 25, 1);
					nk_label(ctx, "Drag and Drop files for extraction.", NK_TEXT_CENTERED);
					nk_layout_row_dynamic(ctx, 25, 1);
					nk_label(ctx, "OR", NK_TEXT_CENTERED);
					nk_layout_row_dynamic(ctx, 25, 3);
					nk_spacing(ctx, 1);
					if (nk_button_label(ctx, "Browse Files"))
					{
						main_settings.is_file_browser_active = nk_true;
						main_settings.scaleWindowForFileBrowser = nk_true;
					}
					nk_spacing(ctx, 1);
				}
				nk_group_end(ctx);
			}

			// RadioButton 2 along with combobox
			static const float ratio_port[] = {0.10f, 0.20f, 0.20f, 0.20f, 0.20f, 0.10f};
			nk_layout_row(ctx, NK_DYNAMIC, 20, 6, ratio_port);
			nk_spacing(ctx, 1);
			if (nk_option_label(ctx, "Extract from", main_settings.port_or_files == PORT))
			{
				// op = PORT;
				main_settings.port_or_files = PORT;
			}
			main_settings.port_select = nk_combo(ctx, main_settings.port_type, 2, main_settings.port_select, 20, nk_vec2(85, 100));
			nk_label(ctx, " stream, on port:", NK_TEXT_LEFT);

			// RADDIO BUTTON 2, TEXTEDIT FOR ENTERING PORT NUMBER

			static int len;
			static char buffer[10];
			nk_edit_string(ctx, NK_EDIT_SIMPLE, main_settings.port_num, &main_settings.port_num_len, 8, nk_filter_decimal);
			nk_layout_space_begin(ctx, NK_STATIC, 10, 1);
			nk_layout_space_end(ctx);

			// Extraction Information
			nk_layout_row_dynamic(ctx, 10, 1);
			nk_text(ctx, "Extraction Info:", 16, NK_TEXT_CENTERED);

			// RECTANGLE-INFO
			static const float ratio_rect_info[] = {0.10f, 0.80f, 0.10f};
			nk_layout_row(ctx, NK_DYNAMIC, 75, 2, ratio_rect_info);
			nk_spacing(ctx, 1);
			if (nk_group_begin(ctx, "Extraction Info:", NK_WINDOW_BORDER))
			{
				if (main_settings.filename_count != 0)
				{
					nk_layout_row_static(ctx, 18, 380, 1);
					nk_label(ctx, concat("Input type: ", input.type[input.type_select]), NK_TEXT_LEFT);
					nk_label(ctx, concat("Output type: ", output.type[output.type_select]), NK_TEXT_LEFT);
					if (output.is_filename)
						nk_label(ctx, concat("Output path: ", output.filename), NK_TEXT_LEFT);
					else
						nk_label(ctx, "Output path: Default", NK_TEXT_LEFT);
					if (burned_subs.is_burned_subs)
						nk_label(ctx, "Hardsubtitles extraction: Yes", NK_TEXT_LEFT);
					else
						nk_label(ctx, "Hardsubtitles extraction: No", NK_TEXT_LEFT);
				}
				nk_group_end(ctx);
			}

			nk_layout_space_begin(ctx, NK_STATIC, 10, 1);
			nk_layout_space_end(ctx);
			// PROGRESSBAR
			static const float ratio_progress[] = {0.10f, 0.03f, 0.57f, 0.03f, 0.17f, 0.10f};
			nk_layout_row(ctx, NK_DYNAMIC, 20, 6, ratio_progress);
			nk_spacing(ctx, 1);
			nk_spacing(ctx, 1);
			nk_progress(ctx, &main_settings.progress_cursor, 101, nk_false);

			// Extract Button
			nk_spacing(ctx, 1);
			if (nk_button_label(ctx, "Extract"))
			{

				setup_and_create_thread(&main_settings, &command);
			}

			nk_layout_space_begin(ctx, NK_STATIC, 10, 1);
			nk_layout_space_end(ctx);

			// PROGRESS_DETAILS_BUTTON
			if (!show_activity_check)
			{
				nk_layout_row_dynamic(ctx, 20, 3);
				nk_spacing(ctx, 1);
				if (nk_button_label(ctx, "Progress Details"))
				{
					show_progress_details = nk_true;
				}
				nk_spacing(ctx, 1);
			}

			// PROGRESS_DETAILS_POPUP
			if (show_progress_details)
				draw_progress_details_popup(ctx, &show_progress_details, &main_settings);

			// build command string
			command_builder(&command, &main_settings, &network_settings, &input, &advanced_input, &output, &decoders, &credits, &debug, &burned_subs);
		}
		nk_end(ctx);

		glfwGetWindowSize(win, &screenWidth, &screenHeight);

		if (!main_settings.scaleWindowForFileBrowser)
		{
			if (show_activity_check && show_preview_check && show_terminal_check)
			{
				if (screenWidth != 930 || screenHeight != 650)
				{
					glfwSetWindowSize(win, 930, 550);
					glfwSetWindowSizeLimits(win, 930, 650, 930, 650);
				}
				preview(ctx, 530, 0, 400, 550, &main_settings);
				terminal(ctx, 0, 550, 530, 100, &command.term_string);
				activity(ctx, 530, 550, 400, 100, &main_settings);
			}
			if (show_activity_check && show_preview_check && !show_terminal_check)
			{
				if (screenWidth != 930 || screenHeight != 650)
				{
					glfwSetWindowSize(win, 930, 650);
					glfwSetWindowSizeLimits(win, 930, 650, 930, 650);
				}
				preview(ctx, 530, 0, 400, 650, &main_settings);
				activity(ctx, 0, 550, 530, 100, &main_settings);
			}
			if (show_activity_check && !show_preview_check && show_terminal_check)
			{
				if (screenWidth != 930 || screenHeight != 650)
				{
					glfwSetWindowSize(win, 930, 650);
					glfwSetWindowSizeLimits(win, 930, 650, 930, 650);
				}
				activity(ctx, 530, 0, 400, 650, &main_settings);
				terminal(ctx, 0, 550, 530, 100, &command.term_string);
			}
			if (show_terminal_check && show_preview_check && !show_activity_check)
			{
				if (screenWidth != 930 || screenHeight != 650)
				{
					glfwSetWindowSize(win, 930, 650);
					glfwSetWindowSizeLimits(win, 930, 650, 930, 650);
				}
				terminal(ctx, 0, 550, 530, 100, &command.term_string);
				preview(ctx, 530, 0, 400, 650, &main_settings);
			}
			if (show_activity_check && !show_preview_check && !show_terminal_check)
			{
				if (screenWidth != 930 || screenHeight == 650)
				{
					glfwSetWindowSize(win, 930, 550);
					glfwSetWindowSizeLimits(win, 930, 550, 930, 550);
				}
				activity(ctx, 530, 0, 400, 550, &main_settings);
			}
			if (show_terminal_check && !show_activity_check && !show_preview_check)
			{
				if (screenHeight != 650 || screenWidth == 930)
				{
					glfwSetWindowSize(win, 530, 650);
					glfwSetWindowSizeLimits(win, 530, 650, 530, 650);
				}
				terminal(ctx, 0, 550, 530, 100, &command.term_string);
			}
			if (show_preview_check && !show_terminal_check && !show_activity_check)
			{
				if (screenHeight != 650 || screenWidth == 930)
				{
					glfwSetWindowSize(win, 930, 550);
					glfwSetWindowSizeLimits(win, 930, 550, 930, 550);
				}
				preview(ctx, 530, 0, 400, 550, &main_settings);
			}
			if (!show_preview_check && !show_terminal_check && !show_activity_check)
			{
				glfwSetWindowSize(win, WIDTH_mainPanelAndWindow, HEIGHT_mainPanelandWindow);
				glfwSetWindowSizeLimits(win, WIDTH_mainPanelAndWindow, HEIGHT_mainPanelandWindow,
							WIDTH_mainPanelAndWindow, HEIGHT_mainPanelandWindow);
			}
		}
		else
		{
			glfwSetWindowSize(win, 930, 650);
			glfwSetWindowSizeLimits(win, 930, 650, 930, 650);
		}

		glViewport(0, 0, screenWidth, screenHeight);
		glClear(GL_COLOR_BUFFER_BIT);
		/* IMPORTANT: `nk_glfw_render` modifies some global OpenGL state
		 * with blending, scissor, face culling and depth test and defaults everything
		 * back into a default state. Make sure to either save and restore or
		 * reset your own state after drawing rendering the UI. */
		nk_glfw3_render(NK_ANTI_ALIASING_ON);
		glfwSwapBuffers(win);
	}

	glDeleteTextures(1, (const GLuint *)&media.icons.home.handle.id);
	glDeleteTextures(1, (const GLuint *)&media.icons.directory.handle.id);
	glDeleteTextures(1, (const GLuint *)&media.icons.computer.handle.id);
#ifdef _WIN32
	glDeleteTextures(1, (const GLuint *)&media.icons.drives.handle.id);
#endif
	glDeleteTextures(1, (const GLuint *)&media.icons.desktop.handle.id);
	glDeleteTextures(1, (const GLuint *)&media.icons.default_file.handle.id);
	glDeleteTextures(1, (const GLuint *)&media.icons.text_file.handle.id);
	glDeleteTextures(1, (const GLuint *)&media.icons.music_file.handle.id);
	glDeleteTextures(1, (const GLuint *)&media.icons.font_file.handle.id);
	glDeleteTextures(1, (const GLuint *)&media.icons.img_file.handle.id);
	glDeleteTextures(1, (const GLuint *)&media.icons.movie_file.handle.id);

	file_browser_free(&browser);
	// free(main_settings.filenames);

	nk_glfw3_shutdown();
	glfwTerminate();
	return 0;
}

void setup_main_settings(struct main_tab *main_settings)
{

	main_settings->is_check_common_extension = nk_false;
	main_settings->port_num_len = 0;
	main_settings->port_or_files = FILES;
	main_settings->port_type = (char **)malloc(2 * sizeof(char *));
	main_settings->port_type[0] = "UDP";
	main_settings->port_type[1] = "TCP";
	main_settings->port_select = 0;
	main_settings->is_file_browser_active = nk_false;
	main_settings->scaleWindowForFileBrowser = nk_false;
	main_settings->preview_string_count = 0;
	main_settings->activity_string_count = 0;
	main_settings->threadPopup = nk_false;
}

char *truncate_path_string(char *filePath)
{
	char *file_path = strdup(filePath);
	int i, j, z, slash_length, fileNameTruncated_index, copycount, prefix_length;
	char file_name[PATH_LENGTH], *ptr_slash, fileNameTruncated[NAME_LENGTH];
	// strcpy(filePath[i], paths[i]);
	if (strlen(filePath) >= PATH_LENGTH - 1)
	{
#ifdef _WIN32
		ptr_slash = strrchr(file_path, '\\');
#else
		ptr_slash = strrchr(file_path, '/');
#endif
		slash_length = strlen(ptr_slash);
		if (slash_length >= NAME_LENGTH)
		{
			fileNameTruncated_index = NAME_LENGTH - 1;
			for (z = 0; z < 6; z++)
			{
				fileNameTruncated[fileNameTruncated_index] = ptr_slash[slash_length];
				fileNameTruncated_index--;
				slash_length--;
			}
			for (z = 0; z < 4; z++)
			{
				fileNameTruncated[fileNameTruncated_index] = '.';
				fileNameTruncated_index--;
			}
			strncpy(fileNameTruncated, ptr_slash, 47);

			strncpy(file_name, file_path, 7);
			file_name[7] = '.';
			file_name[8] = '.';
			file_name[9] = '.';
			file_name[10] = '\0';
			file_name[11] = '\0';
			file_name[12] = '\0';
			strcat(file_name, fileNameTruncated);
			strcpy(file_path, file_name);
		}

		else
		{
			copycount = PATH_LENGTH - 1;
			prefix_length = copycount - slash_length - 3;
			strncpy(file_name, file_path, prefix_length);
			while (slash_length >= 0)
			{
				file_name[copycount] = ptr_slash[slash_length];
				copycount--;
				slash_length--;
			}
			for (j = 0; j < 3; j++, copycount--)
				file_name[copycount] = '.';

			file_name[65] = '\0';
			strcpy(file_path, file_name);
		}
		return file_path;
	}
	else
		return filePath;
}

void remove_path_entry(struct main_tab *main_settings, int indexToRemove)
{
	// printf("Beginning processing. Array is currently: ");
	// for (int i = 0; i < arraySize; ++i)
	//	printf("%d ", (*array)[i]);
	// printf("\n");

	char **temp = (char **)calloc(main_settings->filename_count, sizeof(char *)); // allocate an array with a size 1 less than the current one

	memmove(
	    temp,
	    main_settings->filenames,
	    (indexToRemove + 1) * sizeof(char *)); // copy everything BEFORE the index

	memmove(
	    temp + indexToRemove,
	    (main_settings->filenames) + (indexToRemove + 1),
	    (main_settings->filename_count - indexToRemove) * sizeof(char *)); // copy everything AFTER the index

	free(main_settings->filenames);
	main_settings->filenames = temp;
	main_settings->filename_count--;
	main_settings->filenames[main_settings->filename_count] = NULL;
}

struct nk_image
icon_load(char icon_data[], int len)
{
	int x, y, n;
	GLuint tex;

	unsigned char *data = stbi_load_from_memory(icon_data, len, &x, &y, &n, 0);
	if (!data)
		die("[SDL]: failed to load icons");

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	return nk_image_id((int)tex);
}

char *concat(char *string1, char *string2)
{
	static char prefix[300], suffix[300];
	strcpy(prefix, string1);
	strcpy(suffix, string2);
	return strcat(prefix, suffix);
}
