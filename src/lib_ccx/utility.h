#ifndef CC_UTILITY_H
#define CC_UTILITY_H

#ifndef _WIN32
	#include <arpa/inet.h>
#endif

#define RL32(x) (*(unsigned int *)(x))
#define RB32(x) (ntohl(*(unsigned int *)(x)))
#define RL16(x) (*(unsigned short int*)(x))
#define RB16(x) (ntohs(*(unsigned short int*)(x)))

#define CCX_NOPTS	((int64_t)UINT64_C(0x8000000000000000))

extern int temp_debug;
void init_boundary_time (struct ccx_boundary_time *bt);
void print_error (int mode, const char *fmt, ...);
int stringztoms (const char *s, struct ccx_boundary_time *bt);
char *get_basename(char *filename);
char *get_file_extension(enum ccx_output_format write_format);
int verify_crc32(uint8_t *buf, int len);
size_t utf16_to_utf8(unsigned short utf16_char, unsigned char *out);
#endif
