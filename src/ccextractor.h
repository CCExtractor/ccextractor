#ifndef CCEXTRACTOR_H
#define CCEXTRACTOR_H

// Needed to avoid warning implicit declaration of function ‘asprintf’ on Linux
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#ifdef ENABLE_OCR
#include <leptonica/allheaders.h>
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

extern struct ccx_s_options ccx_options;
extern struct lib_ccx_ctx *signal_ctx;
//volatile int terminate_asap = 0;

struct ccx_s_options* api_init_options();

int api_start(struct ccx_s_options api_options);


void sigterm_handler(int sig);
void sigint_handler(int sig);
void print_end_msg(void);

int main(int argc, char *argv[]);

#endif //CCEXTRACTOR_H
