#include "ccx_cli_thread.h"
#include "ccextractorGUI.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define MAX_WAIT 10
#define PROGRESS_COMPLETE 100

void *extract_thread(void *extract_args)
{
	struct args_extract *params = (struct args_extract *)extract_args;
	static char term_string[500];
	strcpy(term_string, params->command_string);
	strcat(term_string, " ");
	strcat(term_string, params->file_string);
	strcat(term_string, " 1>>ccx.log 2>>gui_report.log");
	printf("%s", term_string);
	system(term_string);
	pthread_exit(0);
}

void *read_activity_data(void *read_args)
{
	puts("Inside activity thread!");
	char line[500];
	char buffer[500];
#if UNIX
	struct timespec time;
	time.tv_sec = 0;
	time.tv_nsec = 10000000L;
#endif
	int wait = 0;
	struct args_extract *read_params = (struct args_extract *)read_args;
	FILE *file;
	char current_input[500];
	int concat_index = 0;
	file = fopen("ccx.log", "r");

	while (file == NULL)
	{
		printf("Cannot open ccx.log, trying again.\n");
		file = fopen("ccx.log", "r");
#if UNIX
		nanosleep(&time, NULL);
#else
		_sleep(10);
#endif
		wait++;
		if (wait == MAX_WAIT)
		{
			read_params->main_threadsettings->threadPopup = nk_true;
			return 0;
		}
	}

	while (!feof(file))
	{
		if (fgets(current_input, sizeof(current_input), file) == NULL)
			continue;
		if (concat_index == 0)
		{
			strcpy(line, current_input);
		}
		else
		{
			strcat(line, current_input);
		}
		concat_index++;
		if (current_input[strlen(current_input) - 1] != '\n')
			continue;

		sscanf(line, "%[^\n]", buffer);
		if (read_params->main_threadsettings->activity_string_count == 0)
			read_params->main_threadsettings->activity_string =
			    malloc(sizeof(*read_params->main_threadsettings->activity_string));
		else
			read_params->main_threadsettings->activity_string =
			    realloc(read_params->main_threadsettings->activity_string,
				    (read_params->main_threadsettings->activity_string_count + 1) * sizeof(char *));

		read_params->main_threadsettings->activity_string[read_params->main_threadsettings->activity_string_count] = strdup(buffer);
		read_params->main_threadsettings->activity_string_count++;

		memset(line, 0, sizeof(line));
		memset(buffer, 0, sizeof(buffer));
		concat_index = 0;
	}
}

void *read_data_from_thread(void *read_args)
{
	pthread_t tid_activity;
	pthread_attr_t attr_activity;
	static char buffer[500];
	char t_start[6], t_end[6], subtitle1[100], subtitle2[100];
#if UNIX
	struct timespec time;
	time.tv_sec = 0;
	time.tv_nsec = 10000000L;
#endif

	int wait = 0;
	struct args_extract *read_params = (struct args_extract *)read_args;
	int unknown1 = 0, unknown2 = 0, progress_count = 0;
	FILE *file;
	char prev_line[500];
	char line[500];
	char current_input[500];
	int concat_index = 0;
	char sub_line[500];
	char prog_line[500];
	int subs_success1, subs_success2, progress_success;

	/*Setup activity thread*/
	pthread_attr_init(&attr_activity);
	int err = pthread_create(&tid_activity, &attr_activity, read_activity_data, read_params);
	if (!err)
		puts("Activity Thread created");

	file = fopen("gui_report.log", "r");

	while (file == NULL)
	{
		printf("Cannot open gui_report.log, trying again.\n");
		file = fopen("gui_report.log", "r");
#if UNIX
		nanosleep(&time, NULL);
#else
		_sleep(10);
#endif
		wait++;
		if (wait >= MAX_WAIT)
		{
			read_params->main_threadsettings->threadPopup = nk_true;
			return 0;
		}
	}

	while (1)
	{
		if (fgets(current_input, sizeof(current_input), file) == NULL)
			continue;
		if (concat_index == 0)
		{
			strcpy(line, current_input);
		}
		else
		{
			strcat(line, current_input);
		}
		concat_index++;
		if (current_input[strlen(current_input) - 1] != '\n')
			continue;

		progress_success = sscanf(line, "###PROGRESS#%d#%d#%d", &progress_count, &unknown1, &unknown2);
		if (progress_success == 3)
			read_params->main_threadsettings->progress_cursor = progress_count;
		subs_success1 = sscanf(line, "###SUBTITLE#%[^#]#%[^#]#%[^\n]", t_start, t_end, subtitle1);
		subs_success2 = sscanf(line, "###SUBTITLE###%[^\n]", subtitle2);
		if (subs_success1 == 3)
		{
			sprintf(buffer, "%s-%s: %s", t_start, t_end, subtitle1);
			if (read_params->main_threadsettings->preview_string_count == 0)
				read_params->main_threadsettings->preview_string =
				    malloc(sizeof(*read_params->main_threadsettings->preview_string));
			else
				read_params->main_threadsettings->preview_string =
				    realloc(read_params->main_threadsettings->preview_string,
					    (read_params->main_threadsettings->preview_string_count + 1) * sizeof(char *));

			read_params->main_threadsettings->preview_string[read_params->main_threadsettings->preview_string_count] = strdup(buffer);
			read_params->main_threadsettings->preview_string_count++;
		}

		if (subs_success2 == 1)
		{
			sprintf(buffer, "                        %s", subtitle2);
			if (read_params->main_threadsettings->preview_string_count == 0)
				read_params->main_threadsettings->preview_string =
				    malloc(sizeof(*read_params->main_threadsettings->preview_string));
			else
				read_params->main_threadsettings->preview_string =
				    realloc(read_params->main_threadsettings->preview_string,
					    (read_params->main_threadsettings->preview_string_count + 1) * sizeof(char *));

			read_params->main_threadsettings->preview_string[read_params->main_threadsettings->preview_string_count] = strdup(buffer);
			read_params->main_threadsettings->preview_string_count++;
		}

		if (progress_count == PROGRESS_COMPLETE)
			break;
		memset(line, 0, sizeof(line));
		concat_index = 0;
	}

	printf("progress count:%d\n", progress_count);
	fclose(file);
	printf("File closed\n");
	for (int i = 0; i < read_params->main_threadsettings->preview_string_count; i++)
		printf("%s\n", read_params->main_threadsettings->preview_string[i]);
	pthread_exit(0);
}

void *feed_files_for_extraction(void *file_args)
{
	printf("Inside feeder\n");

	struct args_extract *extract_params = (struct args_extract *)file_args;
	printf("count:%d\n", extract_params->main_threadsettings->filename_count);
	extract_params->command_string = extract_params->threadcommand->term_string;
	int count = extract_params->main_threadsettings->filename_count;
	pthread_t tid_extract, tid_read;
	pthread_attr_t attr_extract, attr_read;

	for (int i = 0; count != 0; i++, count--)
	{
		pthread_t tid_extract, tid_read;
		pthread_attr_t attr_extract, attr_read;

		pthread_attr_init(&attr_extract);
		pthread_attr_init(&attr_read);

		extract_params->main_threadsettings->is_file_selected[i] = nk_true;
		extract_args.file_string = extract_params->main_threadsettings->filenames[i];
		int err1 = pthread_create(&tid_extract, &attr_extract, extract_thread, extract_params);
		int err2 = pthread_create(&tid_read, &attr_read, read_data_from_thread, extract_params);
		if (!err1)
			printf("Extraction Thread Complete:%d\n", i);
		if (!err2)
			printf("Read Thread Complete:%d\n", i);

		pthread_join(tid_extract, NULL);
		printf("Extract thread joined\n");
		pthread_join(tid_read, NULL);
		printf("Read thread joined\n");

		extract_params->main_threadsettings->is_file_selected[i] = nk_false;

		remove("gui_report.log");
		remove("ccx.log");
	}
	printf("File feeding over\n");
}

void setup_and_create_thread(struct main_tab *main_settings, struct built_string *command)
{
	extract_args.main_threadsettings = (struct main_tab *)main_settings;
	extract_args.threadcommand = (struct built_string *)command;

	pthread_attr_init(&attr_launch);
	int err = pthread_create(&tid_launch, &attr_launch, feed_files_for_extraction, &extract_args);
	if (!err)
		printf("Feeder created!\n");
}

/*THREAD FUNCTIONS FOR HD_HOMERUN*/
void *find_hd_homerun_devices(void *args)
{
	char command[300];
	extract_args.homerun_thread = (struct hd_homerun_tab *)args;
	int wait = 0;
	FILE *file;
	char line[200];
	int device_success;
	char device[200];

#if UNIX
	struct timespec time;
	time.tv_sec = 0;
	time.tv_nsec = 10000000L;
#endif

#if HD_HOMERUN
	strcpy(command, "hdhomerun_config");
#else
	strncpy(command, extract_args.homerun_thread->location, extract_args.homerun_thread->location_len);
#endif
	strcpy(command, " discover >> homerun.log");
	system(command);

	file = fopen("homerun.log", "r");

	while (file == NULL)
	{
		printf("Cannot open file! Trying again.\n");
		file = fopen("homerun.log", "r");
#if UNIX
		nanosleep(&time, NULL);
#else
		_sleep(10);
#endif
		wait++;
		if (wait >= MAX_WAIT)
		{
			extract_args.homerun_thread->threadPopup = nk_true;
			return 0;
		}
	}

	while (1)
	{
		fgets(line, sizeof(line), file);
		device_success = sscanf(line, "hdhomerun device %[^\n]", device);
		if (feof(file))
			break;
		if (device_success == 1)
		{
			if (extract_args.homerun_thread->device_num == 0)
			{
				extract_args.homerun_thread->devices = malloc(sizeof(char *));
				extract_args.homerun_thread->devices[extract_args.homerun_thread->device_num] = strdup(device);
				extract_args.homerun_thread->device_num++;
			}
			else
			{
				extract_args.homerun_thread->devices = realloc(extract_args.homerun_thread->devices,
									       (extract_args.homerun_thread->device_num + 1) * sizeof(char *));
				extract_args.homerun_thread->devices[extract_args.homerun_thread->device_num] = strdup(device);
				extract_args.homerun_thread->device_num++;
			}
		}
	}
	printf("Find device thread finished\n");
}

void *setup_hd_homerun_device(void *args)
{
	char device[20];
	extract_args.homerun_thread = (struct hd_homerun_tab *)args;
	char channel_command[300];
	char program_command[300];
	char target_command[300];

	sscanf(extract_args.homerun_thread->devices[extract_args.homerun_thread->selected], "%s", device);
#if HD_HOMERUN
	strcpy(channel_command, "hdhomerun_config");
	strcpy(program_command, "hdhomerun_config");
	strcpy(target_command, "hdhomerun_config");
#else
	strncpy(channel_command, extract_args.homerun_thread->location, extract_args.homerun_thread->location_len);
	strncpy(program_command, extract_args.homerun_thread->location, extract_args.homerun_thread->location_len);
	strncpy(target_command, extract_args.homerun_thread->location, extract_args.homerun_thread->location_len);
#endif
	strcat(channel_command, " ");
	strcat(program_command, " ");
	strcat(target_command, " ");

	strcat(channel_command, device);
	strcat(program_command, device);
	strcat(target_command, device);

	strcat(channel_command, " set /tuner");
	strcat(program_command, " set /tuner");
	strcat(target_command, " set /tuner");

	strncat(channel_command, extract_args.homerun_thread->tuner, extract_args.homerun_thread->tuner_len);
	strncat(program_command, extract_args.homerun_thread->tuner, extract_args.homerun_thread->tuner_len);
	strncat(target_command, extract_args.homerun_thread->tuner, extract_args.homerun_thread->tuner_len);

	strcat(channel_command, "/channel ");
	strcat(program_command, "/program ");
	strcat(target_command, "/target ");

	strncat(channel_command, extract_args.homerun_thread->channel, extract_args.homerun_thread->channel_len);
	strncat(program_command, extract_args.homerun_thread->program, extract_args.homerun_thread->program_len);
	strncat(target_command, extract_args.homerun_thread->ipv4_address, extract_args.homerun_thread->ipv4_address_len);

	system(channel_command);
	system(program_command);
	system(target_command);

	pthread_exit(0);
}
