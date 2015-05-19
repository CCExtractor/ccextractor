#include <signal.h>
#include "lib_ccx.h"
#include "ccx_common_option.h"

int temp_debug = 0; // This is a convenience variable used to enable/disable debug on variable conditions. Find references to understand.

void timestamp_to_srttime(uint64_t timestamp, char *buffer)
{
	uint64_t p = timestamp;
	uint8_t h = (uint8_t) (p / 3600000);
	uint8_t m = (uint8_t) (p / 60000 - 60 * h);
	uint8_t s = (uint8_t) (p / 1000 - 3600 * h - 60 * m);
	uint16_t u = (uint16_t) (p - 3600000 * h - 60000 * m - 1000 * s);
	sprintf(buffer, "%02"PRIu8":%02"PRIu8":%02"PRIu8",%03"PRIu16, h, m, s, u);
}

void timestamp_to_smptetttime(uint64_t timestamp, char *buffer)
{
	uint64_t p = timestamp;
	uint8_t h = (uint8_t) (p / 3600000);
	uint8_t m = (uint8_t) (p / 60000 - 60 * h);
	uint8_t s = (uint8_t) (p / 1000 - 3600 * h - 60 * m);
	uint16_t u = (uint16_t) (p - 3600000 * h - 60000 * m - 1000 * s);
	sprintf(buffer, "%02"PRIu8":%02"PRIu8":%02"PRIu8".%03"PRIu16, h, m, s, u);
}

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

int levenshtein_dist (const uint64_t *s1, const uint64_t *s2, unsigned s1len, unsigned s2len)
{
	unsigned int x, y, v, lastdiag, olddiag;
	unsigned int *column = (unsigned *) malloc ((s1len+1)*sizeof (unsigned int));
	for (y = 1; y <= s1len; y++)
		column[y] = y;
	for (x = 1; x <= s2len; x++)
	{
		column[0] = x;
		for (y = 1, lastdiag = x-1; y <= s1len; y++)
		{
			olddiag = column[y];
			column[y] = MIN3(column[y] + 1, column[y-1] + 1, lastdiag + (s1[y-1] == s2[x-1] ? 0 : 1));
			lastdiag = olddiag;
		}
	}
	v = column[s1len];
	free (column);
	return v;
}

void millis_to_date (uint64_t timestamp, char *buffer, enum ccx_output_date_format date_format, char millis_separator)
{
	time_t secs;
	unsigned int millis;
	char c_temp[80];
	struct tm *time_struct=NULL;
	switch (date_format)
	{
		case ODF_NONE:
			buffer[0] = 0;
			break;
		case ODF_HHMMSS:
			timestamp_to_srttime (timestamp, buffer);
			buffer[8] = 0;
			break;
		case ODF_HHMMSSMS:
			timestamp_to_srttime (timestamp, buffer);
			break;
		case ODF_SECONDS:
			secs = (time_t) (timestamp/1000);
			millis = (time_t) (timestamp%1000);
			sprintf (buffer, "%lu%c%03u", (unsigned long) secs,
					millis_separator, (unsigned) millis);
			break;
		case ODF_DATE:
			secs = (time_t) (timestamp/1000);
			millis = (unsigned int) (timestamp%1000);
			time_struct = gmtime(&secs);
			strftime(c_temp, sizeof(c_temp), "%Y%m%d%H%M%S", time_struct);
			sprintf (buffer, "%s%c%03u", c_temp, millis_separator, millis);
			break;

		default:
			fatal(CCX_COMMON_EXIT_BUG_BUG, "Invalid value for date_format in millis_to_date()\n");
	}
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

/* Write formatted message to stderr and then exit. */
void fatal(int exit_code, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	if (ccx_options.gui_mode_reports)
		fprintf(stderr,"###MESSAGE#");
	else
		fprintf(stderr, "\rError: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(exit_code);
}

/* General output, replacement for printf so we can control globally where messages go.
   mprint => Message print */
void mprint (const char *fmt, ...)
{
	va_list args;
	if (!ccx_options.messages_target)
		return;
	activity_header(); // Brag about writing it :-)
	va_start(args, fmt);
	if (ccx_options.messages_target==CCX_MESSAGES_STDOUT)
	{
		vfprintf(stdout, fmt, args);
		fflush (stdout);
	}
	else
	{
		vfprintf(stderr, fmt, args);
		fflush (stderr);
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

	if(mask & t)
	{
		va_start(args, fmt);
		if (ccx_options.messages_target==CCX_MESSAGES_STDOUT)
		{
			vfprintf(stdout, fmt, args);
			fflush (stdout);
		}
		else
		{
			vfprintf(stderr, fmt, args);
			fflush (stderr);
		}
		va_end(args);
	}
}

void dump (LLONG mask, unsigned char *start, int l, unsigned long abs_start, unsigned clear_high_bit)
{
	LLONG t=temp_debug ? (ccx_options.debug_mask_on_debug | ccx_options.debug_mask) : ccx_options.debug_mask; // Mask override?
	if(! (mask & t))
		return;

	for (int x=0; x<l; x=x+16)
	{
		mprint ("%08ld | ",x+abs_start);
		for (int j=0; j<16; j++)
		{
			if (x+j<l)
				mprint ("%02X ",start[x+j]);
			else
				mprint ("   ");
		}
		mprint (" | ");
		for (int j=0; j<16; j++)
		{
			if (x+j<l && start[x+j]>=' ')
				mprint ("%c", start[x+j] & (clear_high_bit ? 0x7F : 0xFF)); // 0x7F < remove high bit, convenient for visual CC inspection
			else
				mprint (" ");
		}
		mprint ("\n");
	}
}

void init_boundary_time (struct ccx_boundary_time *bt)
{
	bt->hh = 0;
	bt->mm = 0;
	bt->ss = 0;
	bt->set = 0;
	bt->time_in_ms = 0;
}


void sleep_secs (int secs)
{
#ifdef _WIN32
	Sleep(secs * 1000);
#else
	sleep(secs);
#endif
}

int hex2int (char high, char low)
{
	unsigned char h,l;
	if (high >= '0' && high <= '9')
		h = high-'0';
	else if (high >= 'a' && high <= 'f')
		h = high-'a'+10;
	else
		return -1;
	if (low >= '0' && low <= '9')
		l = low - '0';
	else if (low >= 'a' && low <= 'f')
		l = low - 'a'+10;
	else
		return -1;
	return h * 16+l;
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
