#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "configuration.h"
#include <stddef.h>
#define CNF_FILE "ccextractor.cnf"
struct conf_map
{
	char *name;
	int offset;
	int (*parse_val)(void *var, char *val);
};

static int set_string(void *var, char *val)
{
	char **p = (char **)var;
	char *val1 = NULL;

	if (val == NULL)
		return -1;

	val1 = strdup(val);
	*p = val1;
	return 0;
}
static int set_time(void *var, char *val)
{
	struct ccx_boundary_time **p = (struct ccx_boundary_time **)var;
	if (val == NULL)
		return -1;

	return stringztoms(val, *p);
}
static int set_int(void *var, char *val)
{
	int *p = (int *)var;
	if (val == NULL)
		return -1;

	*p = atoi_hex(val);
	return 0;
}

struct conf_map configuration_map[] = {
    {"INPUT_SOURCE", offsetof(struct ccx_s_options, input_source), set_int},
    {"BUFFER_INPUT", offsetof(struct ccx_s_options, buffer_input), set_int},
    {"NOFONT_COLOR", offsetof(struct ccx_s_options, nofontcolor), set_int},
    {"NOTYPE_SETTING", offsetof(struct ccx_s_options, notypesetting), set_int},
    {"OUTPUT_FORMAT", offsetof(struct ccx_s_options, write_format), set_int},
    {"VIDEO_EDITED", offsetof(struct ccx_s_options, binary_concat), set_int},
    {"GOP_TIME", offsetof(struct ccx_s_options, use_gop_as_pts), set_int},
    {"FIX_PADDINDG", offsetof(struct ccx_s_options, fix_padding), set_int},
    {"GUI_MODE_REPORTS", offsetof(struct ccx_s_options, gui_mode_reports), set_int},
    {"NO_PROGRESS_BAR", offsetof(struct ccx_s_options, no_progress_bar), set_int},
    {"CAP_FILE", offsetof(struct ccx_s_options, sentence_cap_file), set_string},
    {"PROFANITY_FILE", offsetof(struct ccx_s_options, filter_profanity_file), set_string},
    {"START_AT", offsetof(struct ccx_s_options, extraction_start), set_time},
    {"END_AT", offsetof(struct ccx_s_options, extraction_end), set_time},
    {"INVASTIGATE_PACKET", offsetof(struct ccx_s_options, investigate_packets), set_int},
    {"FULL_BIN", offsetof(struct ccx_s_options, fullbin), set_int},
    {"NO_SYNC", offsetof(struct ccx_s_options, nosync), set_int},
    {"HAUPPAUGE_MODE", offsetof(struct ccx_s_options, hauppauge_mode), set_int},
    {"MP4_VIDEO_TRACK", offsetof(struct ccx_s_options, mp4vidtrack), set_int},
    {"USE_PIC_ORDER", offsetof(struct ccx_s_options, usepicorder), set_int},
    {"AUTO_MYTH", offsetof(struct ccx_s_options, auto_myth), set_int},
    {"WTV_MPEG2", offsetof(struct ccx_s_options, wtvmpeg2), set_int},
    {"DATE_FORMAT", offsetof(struct ccx_s_options, date_format), set_int},
    // Settings for 608 decoder
    {"NO_ROLL_UP", offsetof(struct ccx_s_options, settings_608.no_rollup), set_int},
    {"FORCED_RU", offsetof(struct ccx_s_options, settings_608.force_rollup), set_int},
    {"DIRECT_ROLLUP", offsetof(struct ccx_s_options, settings_608.direct_rollup), set_int},
    {"SCREEN_TO_PROCESS", offsetof(struct ccx_s_options, settings_608.screens_to_process), set_int},

    {NULL}};
static int parse_opts(char *str, struct ccx_s_options *opt)
{
	struct conf_map *lmap = configuration_map;
	char *var = strtok(str, "=");
	if (!var)
		return -1;
	while (lmap->name)
	{
		if (!strcmp(lmap->name, var))
		{
			char *val = strtok(NULL, "=");
			return lmap->parse_val((void *)((char *)opt + lmap->offset), val);
		}
		lmap++;
	}
	return 0;
}
static void parse_file(FILE *f, struct ccx_s_options *opt)
{
	char *str = (char *)malloc(128);
	char c = '\0';
	int comments = 0;
	int i = 0;
	int ret = 0;
	*str = '\0';
	while ((c = (char)fgetc(f)) != EOF)
	{
		if (c == '\n')
		{
			if (str[0] != '\0')
			{
				ret = parse_opts(str, opt);
				if (ret < 0)
					mprint("invalid configuration file\n");
			}
			comments = 0;
			i = 0;
			str[0] = '\0';
			continue;
		}
		if (comments || c == '#')
		{
			comments = 1;
			continue;
		}
		str[i] = c;
		i++;
	}
	free(str);
}
void parse_configuration(struct ccx_s_options *opt)
{
	FILE *f = NULL;
	if ((f = fopen(CNF_FILE, "r")) != NULL)
	{
		parse_file(f, opt);
		fclose(f);
	}
}
