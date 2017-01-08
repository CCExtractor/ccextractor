#ifndef _CC_COMMON_COMMON
#define _CC_COMMON_COMMON

#include "ccx_common_platform.h"
#include "ccx_common_structs.h"

// Define possible exit codes that will be passed on to the fatal function
/* Exit codes. Take this seriously as the GUI depends on them.
   0 means OK as usual,
   <100 means display whatever was output to stderr as a warning
   >=100 means display whatever was output to stdout as an error
*/
#define EXIT_OK                                 0
#define EXIT_NO_INPUT_FILES                     2
#define EXIT_TOO_MANY_INPUT_FILES               3
#define EXIT_INCOMPATIBLE_PARAMETERS            4
#define EXIT_UNABLE_TO_DETERMINE_FILE_SIZE      6
#define EXIT_MALFORMED_PARAMETER                7
#define EXIT_READ_ERROR                         8
#define EXIT_WITH_HELP                          9
#define EXIT_NO_CAPTIONS                        10
#define EXIT_NOT_CLASSIFIED                     300
#define EXIT_ERROR_IN_CAPITALIZATION_FILE       501
#define EXIT_BUFFER_FULL                        502
#define EXIT_MISSING_ASF_HEADER                 1001
#define EXIT_MISSING_RCWT_HEADER                1002

#define CCX_COMMON_EXIT_FILE_CREATION_FAILED   5
#define CCX_COMMON_EXIT_UNSUPPORTED            9
#define EXIT_NOT_ENOUGH_MEMORY                 500
#define CCX_COMMON_EXIT_BUG_BUG                1000

#define CCX_OK       0
#define CCX_FALSE    0
#define CCX_TRUE     1
#define CCX_EAGAIN  -100
#define CCX_EOF     -101
#define CCX_EINVAL  -102
#define CCX_ENOSUPP -103
#define CCX_ENOMEM  -104

// Declarations
void fdprintf(int fd, const char *fmt, ...);
void millis_to_time(LLONG milli, unsigned *hours, unsigned *minutes,unsigned *seconds, unsigned *ms);
void freep(void *arg);
void dbg_print(LLONG mask, const char *fmt, ...);
unsigned char *debug_608_to_ASC(unsigned char *ccdata, int channel);
int add_cc_sub_text(struct cc_subtitle *sub, char *str, LLONG start_time,
		LLONG end_time, char *info, char *mode, enum ccx_encoding_type);

extern int cc608_parity_table[256]; // From myth
#endif
