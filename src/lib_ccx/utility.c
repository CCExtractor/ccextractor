#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "activity.h"
#include "utility.h"

int temp_debug = 0; // This is a convenience variable used to enable/disable debug on variable conditions. Find references to understand.
volatile sig_atomic_t change_filename_requested = 0;

extern int ccxr_verify_crc32(uint8_t *buf, int len);
extern int ccxr_levenshtein_dist(const uint64_t *s1, const uint64_t *s2, unsigned s1len, unsigned s2len);
extern int ccxr_levenshtein_dist_char(const char *s1, const char *s2, unsigned s1len, unsigned s2len);
extern void ccxr_timestamp_to_srttime(uint64_t timestamp, char *buffer);
extern void ccxr_timestamp_to_vtttime(uint64_t timestamp, char *buffer);
extern void ccxr_millis_to_date(uint64_t timestamp, char *buffer, enum ccx_output_date_format date_format, char millis_separator);
extern int ccxr_stringztoms(const char *s, struct ccx_boundary_time *bt);

static uint32_t crc32_table[] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

int verify_crc32(uint8_t *buf, int len)
{
	return ccxr_verify_crc32(buf, len);
}

int stringztoms(const char *s, struct ccx_boundary_time *bt)
{
	return ccxr_stringztoms(s, bt);
}
void timestamp_to_srttime(uint64_t timestamp, char *buffer)
{
	return ccxr_timestamp_to_srttime(timestamp, buffer);
}
void timestamp_to_vtttime(uint64_t timestamp, char *buffer)
{
	return ccxr_timestamp_to_vtttime(timestamp, buffer);
}

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

int levenshtein_dist(const uint64_t *s1, const uint64_t *s2, unsigned s1len, unsigned s2len)
{
	return ccxr_levenshtein_dist(s1, s2, s1len, s2len);
}

int levenshtein_dist_char(const char *s1, const char *s2, unsigned s1len, unsigned s2len)
{
	return ccxr_levenshtein_dist_char(s1, s2, s1len, s2len);
}

void millis_to_date(uint64_t timestamp, char *buffer, enum ccx_output_date_format date_format, char millis_separator)
{
	return ccxr_millis_to_date(timestamp, buffer, date_format, millis_separator);
}

bool_t in_array(uint16_t *array, uint16_t length, uint16_t element)
{
	bool_t r = NO;
	for (uint16_t i = 0; i < length; i++)
		if (array[i] == element)
		{
			r = YES;
			break;
		}
	return r;
}

void print_error(int mode, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	if (mode)
		fprintf(stderr, "###MESSAGE#");
	else
		fprintf(stderr, "\rError: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void write_wrapped(int fd, const char *buf, size_t count)
{
	while (count)
	{
		ssize_t written = write(fd, buf, count);
		if (written == -1)
			fatal(1, "writing to file");
		buf += written;
		count -= written;
	}
}

/* Write formatted message to stderr and then exit. */
void fatal(int exit_code, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	if (ccx_options.gui_mode_reports)
		fprintf(stderr, "###MESSAGE#");
	else
		fprintf(stderr, "\rError: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	print_end_msg();
	va_end(args);
	exit(exit_code);
}

/* General output, replacement for printf so we can control globally where messages go.
   mprint => Message print */
void mprint(const char *fmt, ...)
{
	va_list args;
	if (!ccx_options.messages_target)
		return;
	va_start(args, fmt);
	if (ccx_options.messages_target == CCX_MESSAGES_STDOUT)
	{
		vfprintf(stdout, fmt, args);
		fflush(stdout);
	}
	else
	{
		vfprintf(stderr, fmt, args);
		fflush(stderr);
	}
	va_end(args);
}

/* Shorten some debug output code. */
void dbg_print(LLONG mask, const char *fmt, ...)
{
	va_list args;
	LLONG t;
	if (!ccx_options.messages_target)
		return;
	t = temp_debug ? (ccx_options.debug_mask_on_debug | ccx_options.debug_mask) : ccx_options.debug_mask; // Mask override?

	if (mask & t)
	{
		va_start(args, fmt);
		if (ccx_options.messages_target == CCX_MESSAGES_STDOUT)
		{
			vfprintf(stdout, fmt, args);
			fflush(stdout);
		}
		else
		{
			vfprintf(stderr, fmt, args);
			fflush(stderr);
		}
		va_end(args);
	}
}

void dump(LLONG mask, unsigned char *start, int l, unsigned long abs_start, unsigned clear_high_bit)
{
	LLONG t = temp_debug ? (ccx_options.debug_mask_on_debug | ccx_options.debug_mask) : ccx_options.debug_mask; // Mask override?
	if (!(mask & t))
		return;

	for (int x = 0; x < l; x = x + 16)
	{
		mprint("%08ld | ", x + abs_start);
		for (int j = 0; j < 16; j++)
		{
			if (x + j < l)
				mprint("%02X ", start[x + j]);
			else
				mprint("   ");
		}
		mprint(" | ");
		for (int j = 0; j < 16; j++)
		{
			if (x + j < l && start[x + j] >= ' ')
				mprint("%c", start[x + j] & (clear_high_bit ? 0x7F : 0xFF)); // 0x7F < remove high bit, convenient for visual CC inspection
			else
				mprint(" ");
		}
		mprint("\n");
	}
}

void init_boundary_time(struct ccx_boundary_time *bt)
{
	bt->hh = 0;
	bt->mm = 0;
	bt->ss = 0;
	bt->set = 0;
	bt->time_in_ms = 0;
}

void sleep_secs(int secs)
{
#ifdef _WIN32
	Sleep(secs * 1000);
#else
	sleep(secs);
#endif
}

int hex_to_int(char high, char low)
{
	unsigned char h, l;
	if (high >= '0' && high <= '9')
		h = high - '0';
	else if (high >= 'a' && high <= 'f')
		h = high - 'a' + 10;
	else
		return -1;
	if (low >= '0' && low <= '9')
		l = low - '0';
	else if (low >= 'a' && low <= 'f')
		l = low - 'a' + 10;
	else
		return -1;
	return h * 16 + l;
}
int hex_string_to_int(char *string, int len)
{
	int total_return = 0;
	while (*string && len-- > 0 && total_return >= 0)
	{
		unsigned char c = *string;
		if (c >= '0' && c <= '9')
			total_return = total_return * 16 + (c - '0');
		else if (*string >= 'a' && *string <= 'f')
			total_return = total_return * 16 + (c - 'a' + 10);
		else
			total_return = -1;
		string++;
	}
	return total_return;
}

#ifndef _WIN32
void m_signal(int sig, void (*func)(int))
{
	struct sigaction act;
	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if (sigaction(sig, &act, NULL))
		perror("sigaction() error");

	return;
}

#endif

struct encoder_ctx *change_filename(struct encoder_ctx *enc_ctx)
{
	if (change_filename_requested == 0)
	{
		return enc_ctx;
	}
	struct encoder_ctx *temp_encoder = malloc(sizeof(struct encoder_ctx));
	if (!temp_encoder)
	{
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In change_filename: Out of memory allocating temp_encoder.");
	}
	*temp_encoder = *enc_ctx;
	if (enc_ctx->out->fh != -1)
	{
		if (enc_ctx->out->fh > 0)
			close(enc_ctx->out->fh);
		enc_ctx->out->fh = -1;
		int iter;
		size_t filename_len = strlen(enc_ctx->out->filename);
		size_t current_name_size = filename_len + 16;
		char *current_name = malloc(current_name_size);
		if (!current_name)
		{
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In change_filename: Out of memory allocating current_name.");
		}
		strncpy(current_name, enc_ctx->out->filename, current_name_size - 1);
		current_name[current_name_size - 1] = '\0';
		mprint("Creating %s\n", enc_ctx->out->filename);
		if (enc_ctx->out->renaming_extension)
		{
			size_t cur_len = strlen(current_name);
			snprintf(current_name + cur_len, current_name_size - cur_len, ".%d", enc_ctx->out->renaming_extension);
		}
		enc_ctx->out->renaming_extension++;
		for (iter = enc_ctx->out->renaming_extension; iter >= 1; iter--)
		{
			int ret;
			char new_extension[16];
			snprintf(new_extension, sizeof(new_extension), ".%d", iter);
			size_t newname_size = filename_len + 16;
			char *newname = malloc(newname_size);
			if (!newname)
			{
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In change_filename: Out of memory allocating newname.");
			}
			snprintf(newname, newname_size, "%s%s", enc_ctx->out->filename, new_extension);
			ret = rename(current_name, newname);
			if (ret)
			{
				mprint("Failed to rename the file\n");
			}
			mprint("Creating %s\n", newname);
			strncpy(current_name, enc_ctx->out->filename, current_name_size - 1);
			current_name[current_name_size - 1] = '\0';

			if (iter - 2 > 0)
			{
				size_t cur_len = strlen(current_name);
				snprintf(current_name + cur_len, current_name_size - cur_len, ".%d", iter - 2);
			}
			free(newname);
		}

		enc_ctx->out->fh = open(enc_ctx->out->filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
		free(current_name);
		if (enc_ctx->out->fh == -1)
		{
			mprint("Failed to create a new rotation file\n");
			return temp_encoder;
		}
		free(temp_encoder);
		change_filename_requested = 0;
		return enc_ctx;
	}
	return temp_encoder;
}

/*
 * RETURN VALUE: the return value should be free(3)-ed
 */
char *get_basename(char *filename)
{
	char *c;
	int len;
	char *basefilename;

	if (!filename)
		return NULL;

	len = strlen(filename);
	basefilename = (char *)malloc(len + 1);
	if (basefilename == NULL)
	{
		return NULL;
	}

	memcpy(basefilename, filename, len + 1);

	for (c = basefilename + len; c > basefilename && *c != '.'; c--)
	{
		;
	} // Get last .
	if (*c == '.')
		*c = 0;

	return basefilename;
}

const char *get_file_extension(const enum ccx_output_format write_format)
{
	switch (write_format)
	{
		case CCX_OF_CCD:
			return ".ccd";
		case CCX_OF_RAW:
			return ".raw";
		case CCX_OF_SCC:
			return ".scc";
		case CCX_OF_SRT:
			return ".srt";
		case CCX_OF_SSA:
			return ".ass";
		case CCX_OF_WEBVTT:
			return ".vtt";
		case CCX_OF_SAMI:
			return ".smi";
		case CCX_OF_SMPTETT:
			return ".ttml";
		case CCX_OF_TRANSCRIPT:
			return ".txt";
		case CCX_OF_RCWT:
			return ".bin";
		case CCX_OF_SPUPNG:
			return ".xml";
		case CCX_OF_DVDRAW:
			return ".dvdraw";
		case CCX_OF_SIMPLE_XML:
			return ".xml";
		case CCX_OF_G608:
			return ".g608";
		case CCX_OF_MCC:
			return ".mcc";
		case CCX_OF_NULL:
			return NULL;
		case CCX_OF_CURL:
			return NULL;
		default:
			fatal(CCX_COMMON_EXIT_BUG_BUG, "write_format doesn't have any legal value. Please report this bug on Github.com\n");
			errno = EINVAL;
			return NULL;
	}
	return 0;
}

char *create_outfilename(const char *basename, const char *suffix, const char *extension)
{
	char *ptr = NULL;
	size_t blen, slen, elen;

	if (basename)
		blen = strlen(basename);
	else
		blen = 0;

	if (suffix)
		slen = strlen(suffix);
	else
		slen = 0;

	if (extension)
		elen = strlen(extension);
	else
		elen = 0;
	if ((elen + slen + blen) <= 0)
		return NULL;

	size_t total_len = elen + slen + blen + 1;
	ptr = malloc(total_len);
	if (!ptr)
		return NULL;

	snprintf(ptr, total_len, "%s%s%s",
		 basename ? basename : "",
		 suffix ? suffix : "",
		 extension ? extension : "");
	return ptr;
}

/**
 * Ensures the output filename has the expected extension.
 * If the filename already ends with the expected extension, returns a copy.
 * If the filename has a different extension, respects user's choice and returns a copy.
 * If no extension exists, appends the expected extension.
 *
 * @param filename The original filename
 * @param extension The expected extension (e.g., ".srt")
 * @return Newly allocated string with proper extension (caller must free), or NULL on error
 */
char *ensure_output_extension(const char *filename, const char *extension)
{
	if (!filename)
		return NULL;

	size_t fname_len = strlen(filename);
	size_t ext_len = extension ? strlen(extension) : 0;

	// If no extension needed, just return a copy
	if (ext_len == 0)
		return strdup(filename);

	// Check if filename already ends with the extension (case-insensitive)
	if (fname_len >= ext_len)
	{
		const char *fname_ext = filename + fname_len - ext_len;
		if (strcasecmp(fname_ext, extension) == 0)
		{
			// Already has the correct extension
			return strdup(filename);
		}
	}

	// Find the basename (part after last '/' or '\')
	const char *last_sep = strrchr(filename, '/');
	const char *last_sep_win = strrchr(filename, '\\');
	if (last_sep_win > last_sep)
		last_sep = last_sep_win;

	const char *basename_start = last_sep ? last_sep + 1 : filename;
	const char *existing_ext = strrchr(basename_start, '.');

	// If filename already has a different extension (not just ending with '.'), respect user's choice
	if (existing_ext && existing_ext != basename_start && strlen(existing_ext) > 1)
	{
		// Has an existing extension, return as-is
		return strdup(filename);
	}

	// No extension found (or only ends with '.'), append the expected extension
	size_t new_len = fname_len + ext_len + 1;
	char *result = malloc(new_len);
	if (!result)
		return NULL;

	snprintf(result, new_len, "%s%s", filename, extension);
	return result;
}

size_t utf16_to_utf8(unsigned short utf16_char, unsigned char *out)
{
	if (utf16_char < 0x80)
	{
		out[0] = (unsigned char)((utf16_char >> 0 & 0x7F) | 0x00);
		return 1;
	}
	else if (utf16_char < 0x0800)
	{
		out[0] = (unsigned char)((utf16_char >> 6 & 0x1F) | 0xC0);
		out[1] = (unsigned char)((utf16_char >> 0 & 0x3F) | 0x80);
		return 2;
	}
	else
	{
		out[0] = (unsigned char)((utf16_char >> 12 & 0x0F) | 0xE0);
		out[1] = (unsigned char)((utf16_char >> 6 & 0x3F) | 0x80);
		out[2] = (unsigned char)((utf16_char >> 0 & 0x3F) | 0x80);
		return 3;
	}
	return 0;
}

LLONG change_timebase(LLONG val, struct ccx_rational cur_tb, struct ccx_rational dest_tb)
{
	/* val = (value * current timebase) / destination timebase */
	val = val * cur_tb.num * dest_tb.den;
	val = val / (cur_tb.den * dest_tb.num);
	return val;
}

char *str_reallocncat(char *dst, char *src)
{
	size_t src_len = strlen(src);
	size_t dst_len = dst == NULL ? 0 : strlen(dst);
	size_t nl = dst_len + src_len + 1;
	char *tmp = (char *)realloc(dst, nl);
	if (!tmp)
	{
		free(dst);
		return NULL;
	}
	// Copy src after existing content (or at start if dst was NULL)
	memcpy(tmp + dst_len, src, src_len + 1);
	return tmp;
}

#ifdef _WIN32
char *strndup(const char *s, size_t n)
{
	char *p;

	p = (char *)malloc(n + 1);
	if (p == NULL)
		return NULL;
	memcpy(p, s, n);
	p[n] = '\0';
	return p;
}
char *strtok_r(char *str, const char *delim, char **saveptr)
{
	return strtok_s(str, delim, saveptr);
}
#endif //_WIN32

int vasprintf(char **strp, const char *fmt, va_list ap)
{
	int len, ret;
	va_list tmp_va;

	va_copy(tmp_va, ap);
#ifdef _WIN32
	len = _vscprintf(fmt, tmp_va);
#else
	len = vsnprintf(NULL, 0, fmt, tmp_va);
#endif
	va_end(tmp_va);
	if (len == -1)
		return -1;
	size_t size = (size_t)len + 1;
	*strp = malloc(size);
	if (*strp == NULL)
		return -1;

#ifdef _WIN32
	ret = vsprintf_s(*strp, size, fmt, ap);
#else
	ret = vsnprintf(*strp, size, fmt, ap);
#endif
	if (ret == -1)
	{
		free(*strp);
		return -1;
	}

	return ret;
}

int asprintf(char **strp, const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vasprintf(strp, fmt, ap);
	va_end(ap);

	return ret;
}
