#ifndef CC_UTILITY_H
#define CC_UTILITY_H
#include <signal.h>
#ifndef _WIN32
	#include <arpa/inet.h>
#endif

#define RL32(x) (*(unsigned int *)(x))
#define RB32(x) (ntohl(*(unsigned int *)(x)))
#define RL16(x) (*(unsigned short int*)(x))
#define RB16(x) (ntohs(*(unsigned short int*)(x)))

#define RB24(x) (  ((unsigned char*)(x))[0] << 16 | ((unsigned char*)(x))[1] << 8 | ((unsigned char*)(x))[2]  )

#define CCX_NOPTS	((int64_t)UINT64_C(0x8000000000000000))

struct ccx_rational
{
	int num;
	int den;
};
extern int temp_debug;
volatile extern sig_atomic_t change_filename_requested;

int levenshtein_dist_char (const char *s1, const char *s2, unsigned s1len, unsigned s2len);
void init_boundary_time (struct ccx_boundary_time *bt);
void print_error (int mode, const char *fmt, ...);
int stringztoms (const char *s, struct ccx_boundary_time *bt);
char *get_basename(char *filename);
char *get_file_extension(enum ccx_output_format write_format);
char *create_outfilename(const char *basename, const char *suffix, const char *extension);
int verify_crc32(uint8_t *buf, int len);
size_t utf16_to_utf8(unsigned short utf16_char, unsigned char *out);
LLONG change_timebase(LLONG val, struct ccx_rational cur_tb, struct ccx_rational dest_tb);
char *str_reallocncat(char *dst, char *src);

void dump (LLONG mask, unsigned char *start, int l, unsigned long abs_start, unsigned clear_high_bit);
LLONG change_timebase(LLONG val, struct ccx_rational cur_tb, struct ccx_rational dest_tb);
void timestamp_to_vtttime(uint64_t timestamp, char *buffer);
int vasprintf(char **strp, const char *fmt, va_list ap);
int asprintf(char **strp, const char *fmt, ...);
#ifdef _WIN32
char *strndup(const char *s, size_t n);
char *strtok_r(char *str, const char *delim, char **saveptr);
#endif //_WIN32

#endif //CC_UTILITY_H
