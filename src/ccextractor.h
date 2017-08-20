#ifndef CCEXTRACTOR_H
#define CCEXTRACTOR_H

#ifdef ENABLE_OCR
#include <allheaders.h>
#endif
#include <stdio.h>
#include "lib_ccx/lib_ccx.h"
#include "lib_ccx/configuration.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "lib_ccx/ccx_common_option.h"
#include "lib_ccx/ccx_mp4.h"
#include "lib_ccx/hardsubx.h"
#include "lib_ccx/ccx_share.h"
#ifdef WITH_LIBCURL
CURL *curl;
CURLcode res;
#endif
#if defined(ENABLE_OCR) && defined(_WIN32)
#define LEPT_MSG_SEVERITY L_SEVERITY_NONE
#endif

#if defined(PYTHONAPI)
#include "Python.h"
#include "funcobject.h"
#endif

struct python_subs_modified{
        int buffer_count;
        int srt_counter;
        char *start_time;
        char* end_time;
        char** buffer;

        //g608 specific elements
        int g608_grid_color_count;
        char** g608_grid_color;
        int g608_grid_font_count;
        char** g608_grid_font;
};

struct python_subs_array{
#if defined(PYTHONAPI)
        PyObject* reporter;
#endif
        char* temporary_file;
        FILE *fp;
        char* output_filename;
        int has_api_start_exited;
        int old_sub_count;
        int sub_count;
        struct python_subs_modified* subs;
        int is_transcript;
};


struct ccx_s_options ccx_options;
struct lib_ccx_ctx *signal_ctx;
struct python_subs_array array; 
int signal_python_api;                                // 1 symbolises that python wrapper made the call.
//volatile int terminate_asap = 0;

struct ccx_s_options* api_init_options();
void check_configuration_file(struct ccx_s_options api_options);
int compile_params(struct ccx_s_options *api_options,int argc);
void api_add_param(struct ccx_s_options* api_options,char* arg);
int api_start(struct ccx_s_options api_options);
int api_param_count(struct ccx_s_options* api_options);
char * api_param(struct ccx_s_options* api_options, int count);

void sigterm_handler(int sig);
void sigint_handler(int sig);
void print_end_msg(void);

void show_extracted_captions_with_timings();
int main(int argc, char *argv[]);

int cc_to_python_get_old_count();
void cc_to_python_set_old_count();
int cc_to_python_get_number_of_subs();
struct python_subs_modified cc_to_python_get_modified_sub(int i);
int cc_to_python_get_modified_sub_buffer_size(int i);
char* cc_to_python_get_modified_sub_buffer(int i, int j);
char* cc_to_python_get_output_filename();

void call_from_python_api(struct ccx_s_options *api_options);
#if defined(PYTHONAPI)
void run(PyObject * reporter, char * line);
#endif
#endif //CCEXTRACTOR_H
