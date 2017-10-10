#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H

#include "ccextractorGUI.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifndef STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif
#include "tabs.h"


/* ===============================================================
 *
 *                          GUI
 *
 * ===============================================================*/
struct icons {
    struct nk_image desktop;
#ifdef _WIN32
	struct nk_image drives;
#endif
    struct nk_image home;
    struct nk_image computer;
    struct nk_image directory;

    struct nk_image default_file;
    struct nk_image text_file;
    struct nk_image music_file;
    struct nk_image font_file;
    struct nk_image img_file;
    struct nk_image movie_file;
};

enum file_groups {
    FILE_GROUP_DEFAULT,
    FILE_GROUP_TEXT,
    FILE_GROUP_MUSIC,
    FILE_GROUP_FONT,
    FILE_GROUP_IMAGE,
    FILE_GROUP_MOVIE,
    FILE_GROUP_MAX
};

enum file_types {
    FILE_DEFAULT,
    FILE_TEXT,
    FILE_C_SOURCE,
    FILE_CPP_SOURCE,
    FILE_HEADER,
    FILE_CPP_HEADER,
    FILE_MP3,
    FILE_WAV,
    FILE_OGG,
    FILE_TTF,
    FILE_BMP,
    FILE_PNG,
    FILE_JPEG,
    FILE_PCX,
    FILE_TGA,
    FILE_GIF,
    FILE_MAX
};

struct file_group {
    enum file_groups group;
    const char *name;
    struct nk_image *icon;
};

struct file {
    enum file_types type;
    const char *suffix;
    enum file_groups group;
};

struct media {
    int font;
    int icon_sheet;
    struct icons icons;
    struct file_group group[FILE_GROUP_MAX];
    struct file files[FILE_MAX];
};

#define MAX_PATH_LEN 512
struct file_browser {
    /* path */
    char file[MAX_PATH_LEN];
    char home[MAX_PATH_LEN];
    char desktop[MAX_PATH_LEN];
    char directory[MAX_PATH_LEN];
#ifdef _WIN32
	char **drives;
	int drives_num;
#endif

    /* directory content */
    char **files;
    char **directories;
    size_t file_count;
    size_t dir_count;
    struct media *media;
};


void
die(const char *fmt, ...);

char*
file_load(const char* path, size_t* siz);

char*
str_duplicate(const char *src);

void
dir_free_list(char **list, size_t size);

char**
dir_list(const char *dir, int return_subdirs, size_t *count);

struct file_group
FILE_GROUP(enum file_groups group, const char *name, struct nk_image *icon);

struct file
FILE_DEF(enum file_types type, const char *suffix, enum file_groups group);

struct nk_image*
media_icon_for_file(struct media *media, const char *file);

void
media_init(struct media *media);

void
file_browser_reload_directory_content(struct file_browser *browser, const char *path);

#if _WIN32
void
get_drives(struct file_browser *browser);
#endif

void
file_browser_init(struct file_browser *browser, struct media *media);

void
file_browser_free(struct file_browser *browser);

int
file_browser_run(struct file_browser *browser,
		struct nk_context *ctx,
		struct main_tab *main_settings,
		struct output_tab *output,
		struct debug_tab *debug,
		struct hd_homerun_tab *hd_homerun);

struct nk_image
icon_load(const char *filename);

#endif
