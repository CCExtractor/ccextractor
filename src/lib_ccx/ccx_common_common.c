#include "ccx_common_common.h"

int cc608_parity_table[256];

/* printf() for fd instead of FILE*, since dprintf is not portable */
void fdprintf(int fd, const char *fmt, ...)
{
	/* Guess we need no more than 100 bytes. */
	int n, size = 100;
	char *p, *np;
	va_list ap;

	if (fd < 0)
		return;
	if ((p = (char *)malloc(size)) == NULL)
		return;

	while (1)
	{
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf(p, size, fmt, ap);
		va_end(ap);
		/* If that worked, return the string. */
		if (n > -1 && n < size)
		{
			write(fd, p, n);
			free(p);
			return;
		}
		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			size = n + 1; /* precisely what is needed */
		else           /* glibc 2.0 */
			size *= 2;  /* twice the old size */
		if ((np = (char *)realloc(p, size)) == NULL)
		{
			free(p);
			return;
		}
		else
		{
			p = np;
		}
	}
}
/* Converts the given milli to separate hours,minutes,seconds and ms variables */
void millis_to_time(LLONG milli, unsigned *hours, unsigned *minutes,
	unsigned *seconds, unsigned *ms)
{
	// LLONG milli = (LLONG) ((ccblock*1000)/29.97);
	*ms = (unsigned)(milli % 1000); // milliseconds
	milli = (milli - *ms) / 1000;  // Remainder, in seconds
	*seconds = (int)(milli % 60);
	milli = (milli - *seconds) / 60; // Remainder, in minutes
	*minutes = (int)(milli % 60);
	milli = (milli - *minutes) / 60; // Remainder, in hours
	*hours = (int)milli;
}
/* Frees the given pointer */
void freep(void *arg)
{
	void **ptr = (void **)arg;
	if (*ptr)
		free(*ptr);
	*ptr = NULL;

}

int add_cc_sub_text(struct cc_subtitle *sub, char *str, LLONG start_time,
		LLONG end_time, char *info, char *mode, enum ccx_encoding_type e_type)
{
	if (sub->nb_data)
	{
		for(;sub->next;sub = sub->next);
		sub->next = malloc(sizeof(struct cc_subtitle));
		if(!sub->next)
			return -1;
		sub->next->prev = sub;
		sub = sub->next;
	}

	sub->type = CC_TEXT;
	sub->enc_type = e_type;
	sub->data = strdup(str);
	sub->nb_data = str? strlen(str): 0;
	sub->start_time = start_time;
	sub->end_time = end_time;
	if(info)
		strncpy(sub->info, info, 4);
	if(mode)
		strncpy(sub->mode, mode, 4);
	sub->got_output = 1;
	sub->next = NULL;

	return 0;
}

int cc608_parity(unsigned int byte)
{
	int ones = 0;

	for (int i = 0; i < 7; i++)
	{
		if (byte & (1 << i))
			ones++;
	}

	return ones & 1;
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

void build_parity_table (void)
{
	cc608_build_parity_table(cc608_parity_table);
}
