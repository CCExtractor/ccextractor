#ifndef CCX_CLI_THREAD_H
#define CCX_CLI_THREAD_H
#define HAVE_STRUCT_TIMESPEC
#include "ccextractorGUI.h"
#include "popups.h"
#include "tabs.h"
#include "command_builder.h"
#include "pthread.h"
struct args_extract {
	struct main_tab *main_threadsettings;
	struct built_string *threadcommand;
	struct hd_homerun_tab *homerun_thread;
	char *file_string;
	char *command_string;
};


static struct args_extract extract_args;

//FOR EXTRACT BUTTON TRIGGER ---- MAIN_TAB
pthread_t tid_launch;
pthread_attr_t attr_launch;

//FOR FIND DEVICES BUTTON TRIGGER ----- HD_HOMERUN_TAB
pthread_t tid_find;
pthread_attr_t attr_find;

//FOR SETUP DEVICE BUTTON TRIGGER ------ HD_HOMERUN_TAB
pthread_t tid_setup;
pthread_attr_t attr_setup;

void* read_activity_data(void *read_args);
void* read_data_from_thread(void* read_args);
void* extract_thread(void* extract_args);
void* feed_files_for_extraction(void* file_args);
void setup_and_create_thread(struct main_tab *main_settings, struct built_string *command);
void* find_hd_homerun_devices(void *args);
void* setup_hd_homerun_device(void *args);

#endif //!CCX_CLI_THREAD_H
