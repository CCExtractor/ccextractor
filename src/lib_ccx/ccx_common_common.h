#ifndef _CC_COMMON_COMMON
#define _CC_COMMON_COMMON

#include "ccx_common_platform.h"

// Define possible exit codes that will be passed on to the fatal function
#define CCX_COMMON_EXIT_FILE_CREATION_FAILED   5
#define CCX_COMMON_EXIT_UNSUPPORTED            9
#define EXIT_NOT_ENOUGH_MEMORY                 500
#define CCX_COMMON_EXIT_BUG_BUG                1000

// Declarations
void fdprintf(int fd, const char *fmt, ...);
void mstotime(LLONG milli, unsigned *hours, unsigned *minutes,unsigned *seconds, unsigned *ms);
void freep(void *arg);
void dbg_print(LLONG mask, const char *fmt, ...);
unsigned char *debug_608toASC(unsigned char *ccdata, int channel);

extern int cc608_parity_table[256]; // From myth
#endif
