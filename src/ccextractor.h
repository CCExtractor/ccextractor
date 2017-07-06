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

struct ccx_s_options ccx_options;
struct lib_ccx_ctx *signal_ctx;
//volatile int terminate_asap = 0;

struct ccx_s_options* api_init_options();
void checking_configuration_file(struct ccx_s_options api_options);
int compile_params(struct ccx_s_options *api_options,int argc);
void api_add_param(struct ccx_s_options* api_options,char* arg);
int api_start(struct ccx_s_options api_options);
int api_param_count(struct ccx_s_options* api_options);
char * api_param(struct ccx_s_options* api_options, int count);

void sigterm_handler(int sig);
void sigint_handler(int sig);
void print_end_msg();
int main(int argc, char *argv[]);
#endif //CCEXTRACTOR_H
