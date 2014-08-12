/*
This file contains includes for common stuff and platform interoperability. It is required for using other CCX libraries.
*/
#ifndef _CC_COMMON_COMMON
#define _CC_COMMON_COMMON

#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_common_structs.h"
#include "ccx_common_timing.h"

// Define possible exit codes that will be passed on to the fatal function
#define CCX_COMMON_EXIT_UNSUPPORTED							  9
#define CCX_COMMON_EXIT_BUG_BUG                            1000

// Declarations
void fdprintf(int fd, const char *fmt, ...);
void mstotime(LLONG milli, unsigned *hours, unsigned *minutes,unsigned *seconds, unsigned *ms);
void freep(void *arg);

#endif