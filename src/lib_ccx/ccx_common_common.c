#include "ccx_common_common.h"

int cc608_parity_table[256];

/* printf() for fd instead of FILE*, since dprintf is not portable */
int fdprintf(int fd, const char *fmt, ...)
{
	FILE *file;

	if ((file = fdopen(dup(fd), "w")) == NULL)
	{
		return -1;
	}

	va_list ap;

	va_start(ap, fmt);
	int ret = vfprintf(file, fmt, ap);
	fclose(file);
	va_end(ap);

	return ret;
}

/* Converts the given milli to separate hours,minutes,seconds and ms variables */
void millis_to_time(LLONG milli, unsigned *hours, unsigned *minutes,
		    unsigned *seconds, unsigned *ms)
{
	// LLONG milli = (LLONG) ((ccblock*1000)/29.97);
	*ms = (unsigned)(milli % 1000); // milliseconds
	milli = (milli - *ms) / 1000;	// Remainder, in seconds
	*seconds = (int)milli % 60;
	milli = (milli - *seconds) / 60; // Remainder, in minutes
	*minutes = (int)(milli % 60);
	milli = (milli - *minutes) / 60; // Remainder, in hours
	*hours = (int)milli;
}

/* Frees the given pointer */
void freep(void *arg)
{
	void **ptr = arg;
	if (*ptr)
	{
		free(*ptr);
		*ptr = NULL;
	}
}

int add_cc_sub_text(struct cc_subtitle *sub, char *str, LLONG start_time,
		    LLONG end_time, char *info, char *mode, enum ccx_encoding_type e_type)
{
	if (str == NULL || strlen(str) == 0)
		return 0;
	if (sub->nb_data)
	{
		for (; sub->next; sub = sub->next)
			;
		sub->next = malloc(sizeof(struct cc_subtitle));
		if (!sub->next)
			return -1;
		sub->next->prev = sub;
		sub = sub->next;
	}

	sub->type = CC_TEXT;
	sub->enc_type = e_type;
	sub->data = strdup(str);
	sub->datatype = CC_DATATYPE_GENERIC;
	sub->nb_data = str ? strlen(str) : 0;
	sub->start_time = start_time;
	sub->end_time = end_time;
	if (info)
		strncpy(sub->info, info, 4);
	if (mode)
		strncpy(sub->mode, mode, 4);
	sub->got_output = 1;
	sub->next = NULL;

	return 0;
}

// returns 1 if odd parity and 0 if even parity
// Same api interface as GNU extension __builtin_parity
int cc608_parity(unsigned int byte)
{
	unsigned int ones = 0;

	for (int i = 0; i < 7; i++)
	{
		if (byte & (1 << i))
			ones++;
	}

	return ones & 1; // same as `ones % 2` for positive integers
}

void cc608_build_parity_table(int *parity_table)
{
	unsigned int byte;
	int parity_v;
	for (byte = 0; byte <= 127; byte++)
	{
		parity_v = cc608_parity(byte);
		/* CC uses odd parity (i.e., # of 1's in byte is odd.) */
		parity_table[byte] = parity_v;
		parity_table[byte | 0x80] = !parity_v;
	}
}

void build_parity_table(void)
{
	cc608_build_parity_table(cc608_parity_table);
}
