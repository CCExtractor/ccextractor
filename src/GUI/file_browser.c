#include "file_browser.h"
#ifdef _WIN32
#include "win_dirent.h"
#else
#include <dirent.h>
#endif
#include <stdio.h>
#include <assert.h>
#include  <stdlib.h>
#include <stdarg.h>
#ifndef STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif
#if UNIX
#include <unistd.h>
#endif
#include <GL/glew.h>
#include <string.h>

#ifndef NK_IMPLEMENTATION
#include "nuklear_lib/nuklear.h"
#endif

#include "ccextractorGUI.h"
#include "tabs.h"

void
die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputs("\n", stderr);
	exit(EXIT_FAILURE);
}

char*
file_load(const char* path, size_t* siz)
{
	char *buf;
	FILE *fd = fopen(path, "rb");
	if (!fd) die("Failed to open file: %s\n", path);
	fseek(fd, 0, SEEK_END);
	*siz = (size_t)ftell(fd);
	fseek(fd, 0, SEEK_SET);
	buf = (char*)calloc(*siz, 1);
	fread(buf, *siz, 1, fd);
	fclose(fd);
	return buf;
}

char*
str_duplicate(const char *src)
{
	char *ret;
	size_t len = strlen(src);
	if (!len) return 0;
	ret = (char*)malloc(len + 1);
	if (!ret) return 0;
	memcpy(ret, src, len);
	ret[len] = '\0';
	return ret;
}

void
dir_free_list(char **list, size_t size)
{
	size_t i;
	for (i = 0; i < size; ++i)
		free(list[i]);
	free(list);
}

char**
dir_list(const char *dir, int return_subdirs, size_t *count)
{
	size_t n = 0;
	char buffer[MAX_PATH_LEN];
	char **results = NULL;
	const DIR *none = NULL;
	size_t capacity = 32;
	size_t size;
	DIR *z;

	assert(dir);
	assert(count);
	strncpy(buffer, dir, MAX_PATH_LEN);
	n = strlen(buffer);

#ifdef _WIN32
	if (n > 0 && (buffer[n - 1] != '\\'))
		buffer[n++] = '\\';
#else
	if (n > 0 && (buffer[n - 1] != '/'))
		buffer[n++] = '/';
#endif

	size = 0;

	z = opendir(dir);
	if (z != none) {
		int nonempty = 1;
		struct dirent *data = readdir(z);
		nonempty = (data != NULL);
		if (!nonempty) return NULL;

		do {
			DIR *y;
			char *p;
			int is_subdir;
			if (data->d_name[0] == '.')
				continue;

			strncpy(buffer + n, data->d_name, MAX_PATH_LEN - n);
			y = opendir(buffer);
			is_subdir = (y != NULL);
			if (y != NULL) closedir(y);

			if ((return_subdirs && is_subdir) || (!is_subdir && !return_subdirs)) {
				if (!size) {
					results = (char**)calloc(sizeof(char*), capacity);
				}
				else if (size >= capacity) {
					void *old = results;
					capacity = capacity * 2;
					results = (char**)realloc(results, capacity * sizeof(char*));
					assert(results);
					if (!results) free(old);
				}
				p = str_duplicate(data->d_name);
				results[size++] = p;
			}
		} while ((data = readdir(z)) != NULL);
	}

	if (z) closedir(z);
	*count = size;
	return results;
}

struct file_group
	FILE_GROUP(enum file_groups group, const char *name, struct nk_image *icon)
{
	struct file_group fg;
	fg.group = group;
	fg.name = name;
	fg.icon = icon;
	return fg;
}

struct file
	FILE_DEF(enum file_types type, const char *suffix, enum file_groups group)
{
	struct file fd;
	fd.type = type;
	fd.suffix = suffix;
	fd.group = group;
	return fd;
}

struct nk_image*
	media_icon_for_file(struct media *media, const char *file)
{
	int i = 0;
	const char *s = file;
	char suffix[4];
	int found = 0;
	memset(suffix, 0, sizeof(suffix));

	/* extract suffix .xxx from file */
	while (*s++ != '\0') {
		if (found && i < 3)
			suffix[i++] = *s;

		if (*s == '.') {
			if (found) {
				found = 0;
				break;
			}
			found = 1;
		}
	}

	/* check for all file definition of all groups for fitting suffix*/
	for (i = 0; i < FILE_MAX && found; ++i) {
		struct file *d = &media->files[i];
		{
			const char *f = d->suffix;
			s = suffix;
			while (f && *f && *s && *s == *f) {
				s++; f++;
			}

			/* found correct file definition so */

			if (f && *s == '\0' && *f == '\0')
				return media->group[d->group].icon;
		}
	}
	return &media->icons.default_file;
}

void
media_init(struct media *media)
{
	/* file groups */
	struct icons *icons = &media->icons;
	media->group[FILE_GROUP_DEFAULT] = FILE_GROUP(FILE_GROUP_DEFAULT, "default", &icons->default_file);
	media->group[FILE_GROUP_TEXT] = FILE_GROUP(FILE_GROUP_TEXT, "textual", &icons->text_file);
	media->group[FILE_GROUP_MUSIC] = FILE_GROUP(FILE_GROUP_MUSIC, "music", &icons->music_file);
	media->group[FILE_GROUP_FONT] = FILE_GROUP(FILE_GROUP_FONT, "font", &icons->font_file);
	media->group[FILE_GROUP_IMAGE] = FILE_GROUP(FILE_GROUP_IMAGE, "image", &icons->img_file);
	media->group[FILE_GROUP_MOVIE] = FILE_GROUP(FILE_GROUP_MOVIE, "movie", &icons->movie_file);

	/* files */
	media->files[FILE_DEFAULT] = FILE_DEF(FILE_DEFAULT, NULL, FILE_GROUP_DEFAULT);
	media->files[FILE_TEXT] = FILE_DEF(FILE_TEXT, "txt", FILE_GROUP_TEXT);
	media->files[FILE_C_SOURCE] = FILE_DEF(FILE_C_SOURCE, "c", FILE_GROUP_TEXT);
	media->files[FILE_CPP_SOURCE] = FILE_DEF(FILE_CPP_SOURCE, "cpp", FILE_GROUP_TEXT);
	media->files[FILE_HEADER] = FILE_DEF(FILE_HEADER, "h", FILE_GROUP_TEXT);
	media->files[FILE_CPP_HEADER] = FILE_DEF(FILE_HEADER, "hpp", FILE_GROUP_TEXT);
	media->files[FILE_MP3] = FILE_DEF(FILE_MP3, "mp3", FILE_GROUP_MUSIC);
	media->files[FILE_WAV] = FILE_DEF(FILE_WAV, "wav", FILE_GROUP_MUSIC);
	media->files[FILE_OGG] = FILE_DEF(FILE_OGG, "ogg", FILE_GROUP_MUSIC);
	media->files[FILE_TTF] = FILE_DEF(FILE_TTF, "ttf", FILE_GROUP_FONT);
	media->files[FILE_BMP] = FILE_DEF(FILE_BMP, "bmp", FILE_GROUP_IMAGE);
	media->files[FILE_PNG] = FILE_DEF(FILE_PNG, "png", FILE_GROUP_IMAGE);
	media->files[FILE_JPEG] = FILE_DEF(FILE_JPEG, "jpg", FILE_GROUP_IMAGE);
	media->files[FILE_PCX] = FILE_DEF(FILE_PCX, "pcx", FILE_GROUP_IMAGE);
	media->files[FILE_TGA] = FILE_DEF(FILE_TGA, "tga", FILE_GROUP_IMAGE);
	media->files[FILE_GIF] = FILE_DEF(FILE_GIF, "gif", FILE_GROUP_IMAGE);
}

void
file_browser_reload_directory_content(struct file_browser *browser, const char *path)
{
	strncpy(browser->directory, path, MAX_PATH_LEN);
	dir_free_list(browser->files, browser->file_count);
	dir_free_list(browser->directories, browser->dir_count);
	browser->files = dir_list(path, 0, &browser->file_count);
	browser->directories = dir_list(path, 1, &browser->dir_count);
}

#ifdef _WIN32
void
get_drives(struct file_browser *browser)
{
	static int drive_num;
	static char drive_list[50][4];
	int c, prev_char;

	system("wmic logicaldisk get name 1> drive.txt");

	FILE *file;
	file = fopen("drive.txt", "r");
	if (file == NULL)
	{
		printf("cannot find any drives! try again with different settings/permissions");
	}
	else {
		puts("File opened");
		while ((c = getc(file)) != EOF)
		{
			if (c == ':')
			{
				sprintf(drive_list[drive_num], "%c", prev_char);
				drive_num++;
				continue;
			}
			if (c < 65 || c > 90)
				continue;

			prev_char = c;

		}
	}
	printf("drive nums:%d\n", drive_num);

	for (int i = 0; i < drive_num; i++)
		strcat(drive_list[i], ":\\");


	browser->drives_num = drive_num;
	browser->drives = (char**)calloc(drive_num + 1, sizeof(char*));
	for (int i = 0; i < drive_num; i++)
	{
		browser->drives[i] = (char*)calloc(strlen(drive_list[i]), sizeof(char));
		browser->drives[i] = strdup(drive_list[i]);
	}
	browser->drives[browser->drives_num] = NULL;

	for (int i = 0; i< drive_num; i++)
		puts(browser->drives[i]);

	fclose(file);
	remove("drive.txt");
}
#endif

void
file_browser_init(struct file_browser *browser, struct media *media)
{
	memset(browser, 0, sizeof(*browser));
	browser->media = media;

#ifdef _WIN32
	get_drives(browser);
#endif
	{
		/* load files and sub-directory list */
		const char *home = getenv("HOME");
#ifdef _WIN32
		if (!home) home = getenv("USERPROFILE");
#else
		if (!home) home = getpwuid(getuid());
#endif
		{
			size_t l;
			strncpy(browser->home, home, MAX_PATH_LEN);
			l = strlen(browser->home);
#ifdef _WIN32
			strcpy(browser->home + l, "\\");
#else
			strcpy(browser->home + l, "/");
#endif
			strcpy(browser->directory, browser->home);
		}
		{
			size_t l;
			strcpy(browser->desktop, browser->home);
			l = strlen(browser->desktop);
#ifdef _WIN32
			strcpy(browser->desktop + l, "Desktop\\");
#else
			strcpy(browser->desktop + l, "Desktop/");
#endif
		}
		browser->files = dir_list(browser->directory, 0, &browser->file_count);
		browser->directories = dir_list(browser->directory, 1, &browser->dir_count);
	}
}

void
file_browser_free(struct file_browser *browser)
{
	if (browser->files)
		dir_free_list(browser->files, browser->file_count);
	if (browser->directories)
		dir_free_list(browser->directories, browser->dir_count);
	browser->files = NULL;
	browser->directories = NULL;
	memset(browser, 0, sizeof(*browser));
}

int
file_browser_run(struct file_browser *browser,
	struct nk_context *ctx,
	struct main_tab *main_settings,
	struct output_tab *output,
	struct debug_tab *debug,
	struct hd_homerun_tab *hd_homerun)
{
	static int isFileAdded = nk_false;
	int ret = 0;
	struct media *media = browser->media;
	struct nk_rect total_space;


	if (nk_popup_begin(ctx, NK_POPUP_STATIC, "File Browser", NK_WINDOW_CLOSABLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_MOVABLE,
		nk_rect(0, 0, 930, 650)))
	{
		static float ratio[] = { 0.25f, NK_UNDEFINED };
		float spacing_x = ctx->style.window.spacing.x;

		/* output path directory selector in the menubar */
		ctx->style.window.spacing.x = 0;
		nk_menubar_begin(ctx);
		{
			char *d = browser->directory;
			char *begin = d + 1;
			nk_layout_row_dynamic(ctx, 25, 6);
			while (*d++) {
#ifdef _WIN32
				if (*d == '\\')
#else
				if (*d == '/')
#endif
				{
					*d = '\0';
					if (nk_button_label(ctx, begin)) {
#ifdef _WIN32
						*d++ = '\\';
#else
						*d++ = '/';
#endif
						*d = '\0';
						file_browser_reload_directory_content(browser, browser->directory);

						break;
					}
#ifdef _WIN32
					*d = '\\';
#else
					*d = '/';
#endif
					begin = d + 1;
				}
			}
		}
		nk_menubar_end(ctx);
		ctx->style.window.spacing.x = spacing_x;

		/* window layout */
		total_space = nk_window_get_content_region(ctx);
		nk_layout_row(ctx, NK_DYNAMIC, total_space.h, 2, ratio);
		nk_group_begin(ctx, "Special", NK_WINDOW_NO_SCROLLBAR);
		{
			struct nk_image home = media->icons.home;
			struct nk_image desktop = media->icons.desktop;
			struct nk_image computer = media->icons.computer;
#ifdef _WIN32
			struct nk_image drive = media->icons.drives;
#endif

			nk_layout_row_dynamic(ctx, 40, 1);
			if (nk_button_image_label(ctx, home, "Home", NK_TEXT_CENTERED))
				file_browser_reload_directory_content(browser, browser->home);
			if (nk_button_image_label(ctx, desktop, "Desktop", NK_TEXT_CENTERED))
				file_browser_reload_directory_content(browser, browser->desktop);
#ifdef _WIN32
			for (int drive_counter = 0; drive_counter < browser->drives_num; drive_counter++)
			{
				if (nk_button_image_label(ctx, drive, browser->drives[drive_counter], NK_TEXT_CENTERED))
					file_browser_reload_directory_content(browser, browser->drives[drive_counter]);
			}
#else
			if (nk_button_image_label(ctx, computer, "Computer", NK_TEXT_CENTERED))
				file_browser_reload_directory_content(browser, "/");
#endif
			nk_group_end(ctx);
		}

		/* output directory content window */
		nk_group_begin(ctx, "Content", 0);
		{
			int index = -1;
			size_t i = 0, j = 0, k = 0;
			size_t rows = 0, cols = 0;
			size_t count = browser->dir_count + browser->file_count;

			cols = 4;
			rows = count / cols;
			for (i = 0; i <= rows; i += 1) {
				{size_t n = j + cols;
				nk_layout_row_dynamic(ctx, 135, (int)cols);
				for (; j < count && j < n; ++j) {
					/* draw one row of icons */
					if (j < browser->dir_count) {
						/* draw and execute directory buttons */
						if (nk_button_image(ctx, media->icons.directory))
							index = (int)j;
					}
					else {
						/* draw and execute files buttons */
						struct nk_image *icon;
						size_t fileIndex = ((size_t)j - browser->dir_count);
						icon = media_icon_for_file(media, browser->files[fileIndex]);
						if (nk_button_image(ctx, *icon)) {
							strncpy(browser->file, browser->directory, MAX_PATH_LEN);
							n = strlen(browser->file);
							strncpy(browser->file + n, browser->files[fileIndex], MAX_PATH_LEN - n);
							ret = 1;


							if (hd_homerun->is_homerun_browser_active)
							{
								hd_homerun->location_len = strlen(browser->file);
								strncpy(hd_homerun->location, browser->file, hd_homerun->location_len);
								isFileAdded = nk_true;
								hd_homerun->is_homerun_browser_active = nk_false;
								break;
							}
							if (debug->is_debug_browser_active)
							{
								debug->elementary_stream_len = strlen(browser->file);
								strcpy(debug->elementary_stream, browser->file);
								isFileAdded = nk_true;
								debug->is_debug_browser_active = nk_false;
								break;
							}
							if (output->is_output_browser_active)
							{
								output->filename_len = strlen(browser->file);
								strcpy(output->filename, browser->file);
								isFileAdded = nk_true;
								output->is_output_browser_active = nk_false;
								break;
							}
							if (output->is_cap_browser_active)
							{
								output->cap_dictionary_len = strlen(browser->file);
								strcpy(output->cap_dictionary, browser->file);
								isFileAdded = nk_true;
								output->is_cap_browser_active = nk_false;
								break;
							}
							if (main_settings->is_file_browser_active)
							{
								if (main_settings->filename_count == 0)								
									main_settings->filenames = (char**)calloc(2, sizeof(char*));
								else
									main_settings->filenames = (char**)realloc(main_settings->filenames, (main_settings->filename_count + 2) * sizeof(char*));

                                main_settings->filenames[main_settings->filename_count] = (char*)calloc((strlen(browser->file) + 5), sizeof(char));
								main_settings->filenames[main_settings->filename_count][0] = '\"';
								strcat(main_settings->filenames[main_settings->filename_count], browser->file);
								strcat(main_settings->filenames[main_settings->filename_count], "\"");
								main_settings->filename_count++;
                                main_settings->filenames[main_settings->filename_count] = NULL;
								isFileAdded = nk_true;
								main_settings->is_file_browser_active = nk_false;
								break;
							}

						}
					}
				}}
				{size_t n = k + cols;
				nk_layout_row_dynamic(ctx, 20, (int)cols);
				for (; k < count && k < n; k++) {
					/* draw one row of labels */
					if (k < browser->dir_count) {
						nk_label(ctx, browser->directories[k], NK_TEXT_CENTERED);
					}
					else {
						size_t t = k - browser->dir_count;
						nk_label(ctx, browser->files[t], NK_TEXT_CENTERED);
					}
				}}
			}

			if (index != -1) {
				size_t n = strlen(browser->directory);
				strncpy(browser->directory + n, browser->directories[index], MAX_PATH_LEN - n);
				n = strlen(browser->directory);
				if (n < MAX_PATH_LEN - 1) {
#ifdef _WIN32
					browser->directory[n] = '\\';
#else
					browser->directory[n] = '/';
#endif
					browser->directory[n + 1] = '\0';
				}
				file_browser_reload_directory_content(browser, browser->directory);
			}
			nk_group_end(ctx);


		}
		if (isFileAdded) {
			isFileAdded = nk_false;
			main_settings->scaleWindowForFileBrowser = nk_false;
			nk_popup_close(ctx);
		}

		nk_popup_end(ctx);
		return ret;
	}
	else {
		main_settings->scaleWindowForFileBrowser = nk_false;
		return 0;
	}
}

