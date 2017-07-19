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

struct cc_to_python_subs{
    char **inputfile;                // List of files to process
	int num_input_files;             // How many input files
    char* basefilename;
    char* extension;
    char** subs;
    int number_of_lines;
};

struct ccx_s_options ccx_options;
struct lib_ccx_ctx *signal_ctx;
struct cc_to_python_subs python_subs;
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
int main(int argc, char *argv[]);

int cc_to_python_get_subs_number_of_lines();
char* cc_to_python_get_sub(int i);

//char* cc_to_python_get_basefilename();
//char* cc_to_python_get_extension();

//int __real_write(int file_handle, char* buffer, int nbyte);
int __wrap_write(int file_handle, char* buffer, int nbyte);
#endif //CCEXTRACTOR_H
