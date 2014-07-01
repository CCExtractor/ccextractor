#include "ccextractor.h"

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

static char *text;
static int text_size=0;

//int temp_debug = 0; // This is a convenience variable used to enable/disable debug on variable conditions. Find references to understand.

void timestamp_to_srttime(uint64_t timestamp, char *buffer) {
	uint64_t p = timestamp;
	uint8_t h = (uint8_t) (p / 3600000);
	uint8_t m = (uint8_t) (p / 60000 - 60 * h);
	uint8_t s = (uint8_t) (p / 1000 - 3600 * h - 60 * m);
	uint16_t u = (uint16_t) (p - 3600000 * h - 60000 * m - 1000 * s);
	sprintf(buffer, "%02"PRIu8":%02"PRIu8":%02"PRIu8",%03"PRIu16, h, m, s, u);
}
#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

int levenshtein_dist (const uint64_t *s1, const uint64_t *s2, unsigned s1len, unsigned s2len) {
    unsigned int x, y, v, lastdiag, olddiag;
    unsigned int *column=(unsigned *) malloc ((s1len+1)*sizeof (unsigned int));
    for (y = 1; y <= s1len; y++)
        column[y] = y;
    for (x = 1; x <= s2len; x++) {
        column[0] = x;
        for (y = 1, lastdiag = x-1; y <= s1len; y++) {
            olddiag = column[y];
            column[y] = MIN3(column[y] + 1, column[y-1] + 1, lastdiag + (s1[y-1] == s2[x-1] ? 0 : 1));
            lastdiag = olddiag;
        }
    }
	v=column[s1len];
    free (column);	
	return v;
}

void millis_to_date (uint64_t timestamp, char *buffer) 
{
	time_t secs;
	unsigned int millis;
	char c_temp[80];
	struct tm *time_struct=NULL;
	switch (ccx_options.date_format)
	{
		case ODF_NONE:
			buffer[0]=0;
			break;
		case ODF_HHMMSS:
			timestamp_to_srttime (timestamp, buffer);
			buffer[8]=0;
			break;
		case ODF_HHMMSSMS:
			timestamp_to_srttime (timestamp, buffer);
			break;
		case ODF_SECONDS:
			secs=(time_t) (timestamp/1000);
			millis=(time_t) (timestamp%1000);
			sprintf (buffer, "%lu%c%03u", (unsigned long) secs,
				ccx_options.millis_separator,(unsigned) millis);
			break;
		case ODF_DATE:
			secs=(time_t) (timestamp/1000);
			millis=(unsigned int) (timestamp%1000);			
            time_struct = gmtime(&secs);
            strftime(c_temp, sizeof(c_temp), "%Y%m%d%H%M%S", time_struct);
			sprintf (buffer,"%s%c%03u",c_temp,
				ccx_options.millis_separator,millis);			
			break;

		default:
			fatal (EXIT_BUG_BUG, "Invalid value for date_format in millis_to_date()\n");
	}
}

void mstotime (LLONG milli, unsigned *hours, unsigned *minutes,
               unsigned *seconds, unsigned *ms)
{
    // LLONG milli = (LLONG) ((ccblock*1000)/29.97);
    *ms=(unsigned) (milli%1000); // milliseconds
    milli=(milli-*ms)/1000;  // Remainder, in seconds
    *seconds = (int) (milli%60);
    milli=(milli-*seconds)/60; // Remainder, in minutes
    *minutes = (int) (milli%60);
    milli=(milli-*minutes)/60; // Remainder, in hours
    *hours=(int) milli;
}

bool_t in_array(uint16_t *array, uint16_t length, uint16_t element) {
	bool_t r = NO;
	for (uint16_t i = 0; i < length; i++)
		if (array[i] == element) {
			r = YES;
			break;
		}
	return r;
}

/* Alloc text space */
void resize_text()
{
	text_size=(!text_size)?1024:text_size*2;
	if (text)
		free (text);
	text=(char *) malloc (text_size);
	if (!text)
		fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory for text buffer.");
	memset (text,0,text_size);
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

/* printf() for fd instead of FILE*, since dprintf is not portable */
void fdprintf (int fd, const char *fmt, ...)
{
     /* Guess we need no more than 100 bytes. */
     int n, size = 100;
     char *p, *np;
     va_list ap;

     if ((p = (char *) malloc (size)) == NULL)
        return;

     while (1) 
	 {
        /* Try to print in the allocated space. */
        va_start(ap, fmt);
        n = vsnprintf (p, size, fmt, ap);
        va_end(ap);
        /* If that worked, return the string. */
        if (n > -1 && n < size)
		{
			write (fd, p, n);			
			free (p);
            return;
		}
        /* Else try again with more space. */
        if (n > -1)    /* glibc 2.1 */
           size = n+1; /* precisely what is needed */
        else           /* glibc 2.0 */
           size *= 2;  /* twice the old size */
        if ((np = (char *) realloc (p, size)) == NULL) 
		{
           free(p);
           return ;
        } else {
           p = np;
        }
     }
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
	t=temp_debug ? (ccx_options.debug_mask_on_debug | ccx_options.debug_mask) : ccx_options.debug_mask; // Mask override?

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


/* Shorten some debug output code. */
void dvprint(const char *fmt, ...)
{
    va_list args;
	if (!ccx_options.messages_target)
		return;
	if(! (ccx_options.debug_mask & CCX_DMT_VIDES ))
		return;

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
            if (x+j<=l && start[x+j]>=' ')
				mprint ("%c",start[x+j] & (clear_high_bit?0x7F:0xFF)); // 0x7F < remove high bit, convenient for visual CC inspection
            else
                mprint (" ");
        }
        mprint ("\n");
    }
}

void init_boundary_time (struct ccx_boundary_time *bt)
{
    bt->hh=0;
    bt->mm=0;
    bt->ss=0;
    bt->set=0;
    bt->time_in_ms=0;
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
	if (high>='0' && high<='9')
		h=high-'0';
	else if (high>='a' && high<='f')
		h=high-'a'+10;
	else 
		return -1;
	if (low>='0' && low<='9')
		l=low-'0';
	else if (low>='a' && low<='f')
		l=low-'a'+10;
	else 
		return -1;
	return h*16+l;
}
/**
 * @param base points to the start of the array
 * @param nb   number of element in array
 * @param size size of each element
 * @param compar Comparison function, which is called with three argument
 *               that point to the objects being compared and arg.
 * @param arg argument passed as it is to compare function
 */
void shell_sort(void *base, int nb,size_t size,int (*compar)(const void*p1,const void *p2,void*arg),void *arg)
{
	unsigned char *lbase = (unsigned char*)base;
	unsigned char *tmp = (unsigned char*)malloc(size);
	for (int gap = nb / 2; gap > 0; gap = gap / 2)
	{
		int p, j;
		for (p = gap; p < nb; p++)
		{
			memcpy(tmp, lbase + (p *size), size);
			for (j = p; j >= gap && ( compar(tmp,lbase + ( (j - gap) * size),arg) < 0); j -= gap)
			{
				memcpy(lbase + (j*size),lbase + ( (j - gap) * size),size);
			}
			memcpy(lbase + (j *size),tmp, size);
		}
	}
    free(tmp);
}

int string_cmp2(const void *p1,const void *p2,void *arg)
{
	return strcasecmp(*(char**)p1,*(char**)p2);
}
int string_cmp(const void *p1,const void *p2)
{
	return string_cmp2(p1, p2, NULL);
}
