#include "ccx_common_common.h"

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
void mstotime(LLONG milli, unsigned *hours, unsigned *minutes,
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
