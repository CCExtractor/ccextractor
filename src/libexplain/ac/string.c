/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/stddef.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/gcc_attributes.h>


/*
 *  NAME
 *      strerror - string for error number
 *
 *  SYNOPSIS
 *      char *strerror(int errnum);
 *
 *  DESCRIPTION
 *      The strerror function maps the error number in errnum to an error
 *      message string.
 *
 *  RETURNS
 *      The strerror function returns a pointer to the string, the contents of
 *      which are implementation-defined.  The array pointed to shall not be
 *      modified by the program, but may be overwritten by a subsequent call to
 *      the strerror function.
 *
 *  CAVEAT
 *      Unknown errors will be rendered in the form "Error %d", where %d will
 *      be replaced by a decimal representation of the error number.
 */

#ifndef HAVE_STRERROR

LINKAGE_HIDDEN
char *
strerror(int n)
{
    extern int      sys_nerr;
    extern char     *sys_errlist[];
    static char     buffer[16];

    if (n < 1 || n > sys_nerr)
    {
            sprintf(buffer, "Error %d", n);
            return buffer;
    }
    return sys_errlist[n];
}

#endif /* !HAVE_STRERROR */


#ifndef HAVE_STRCASECMP

LINKAGE_HIDDEN
int
strcasecmp(const char *s1, const char *s2)
{
    int             c1;
    int             c2;

    for (;;)
    {
        c1 = (unsigned char)*s1++;
        if (islower(c1))
            c1 = toupper(c1);
        c2 = (unsigned char)*s2++;
        if (islower(c2))
            c2 = toupper(c2);
        if (c1 != c2)
        {
            /*
             * if s1 is a leading substring of s2, must
             * return -1, even if the next character of s2
             * is negative.
             */
            if (!c1)
                return -1;
            if (c1 < c2)
                return -1;
            return 1;
        }
        if (!c1)
            return 0;
    }
}

#endif /* !HAVE_STRCASECMP */


#ifndef HAVE_STRSIGNAL

LINKAGE_HIDDEN
char *
strsignal(int n)
{
    switch (n)
    {
#ifdef SIGHUP
    case SIGHUP:
            return "hang up [SIGHUP]";
#endif /* SIGHUP */

#ifdef SIGINT
    case SIGINT:
            return "user interrupt [SIGINT]";
#endif /* SIGINT */

#ifdef SIGQUIT
    case SIGQUIT:
            return "user quit [SIGQUIT]";
#endif /* SIGQUIT */

#ifdef SIGILL
    case SIGILL:
            return "illegal instruction [SIGILL]";
#endif /* SIGILL */

#ifdef SIGTRAP
    case SIGTRAP:
            return "trace trap [SIGTRAP]";
#endif /* SIGTRAP */

#ifdef SIGIOT
    case SIGIOT:
            return "abort [SIGIOT]";
#endif /* SIGIOT */

#ifdef SIGEMT
    case SIGEMT:
            return "EMT instruction [SIGEMT]";
#endif /* SIGEMT */

#ifdef SIGFPE
    case SIGFPE:
            return "floating point exception [SIGFPE]";
#endif /* SIGFPE */

#ifdef SIGKILL
    case SIGKILL:
            return "kill [SIGKILL]";
#endif /* SIGKILL */

#ifdef SIGBUS
    case SIGBUS:
            return "bus error [SIGBUS]";
#endif /* SIGBUS */

#ifdef SIGSEGV
    case SIGSEGV:
            return "segmentation violation [SIGSEGV]";
#endif /* SIGSEGV */

#ifdef SIGSYS
    case SIGSYS:
            return "bad argument to system call [SIGSYS]";
#endif /* SIGSYS */

#ifdef SIGPIPE
    case SIGPIPE:
            return "write on a pipe with no one to read it [SIGPIPE]";
#endif /* SIGPIPE */

#ifdef SIGALRM
    case SIGALRM:
            return "alarm clock [SIGALRM]";
#endif /* SIGALRM */

#ifdef SIGTERM
    case SIGTERM:
            return "software termination [SIGTERM]";
#endif /* SIGTERM */

#ifdef SIGUSR1
    case SIGUSR1:
            return "user defined signal one [SIGUSR1]";
#endif /* SIGUSR1 */

#ifdef SIGUSR2
    case SIGUSR2:
            return "user defined signal two [SIGUSR2]";
#endif /* SIGUSR2 */

#ifdef SIGCLD
    case SIGCLD:
            return "death of child [SIGCLD]";
#endif /* SIGCLD */

#ifdef SIGPWR
    case SIGPWR:
            return "power failure [SIGPWR]";
#endif /* SIGPWR */
    }
    return 0;
}

#endif /* !HAVE_STRSIGNAL */


const char *
explain_strsignal(int n)
{
    static char buffer[16];
    const char *s = strsignal(n);
    if (s && *s)
        return s;
    snprintf(buffer, sizeof(buffer), "signal %d", n);
    return buffer;
}


char *
explain_strendcpy(char *dst, const char *src, const char *end)
{
    if (dst < end)
    {
        /* leave room for terminating NUL */
        end--;
        while (dst < end && *src)
            *dst++ = *src++;
        *dst = '\0';
        if (*src)
        {
            /* return end parameter if truncated */
            dst++;
        }
    }
    return dst;
}

#ifndef HAVE_STRNSTR

/* this should have LINKAGE_HIDDEN except that codegen needs it */

char *
strnstr(const char *haystack, const char *needle, size_t haystack_size)
{
    size_t          needle_size;
    size_t          j;

    needle_size = strlen(needle);
    if (haystack_size < needle_size)
        return 0;
    for (j = 0; needle_size + j <= haystack_size; ++j)
    {
        if (0 == memcmp(haystack + j, needle, needle_size))
            return (char *)(haystack + j);
    }
    return 0;
}

#endif /* !HAVE_STRNSTR */


/* vim: set ts=8 sw=4 et : */
