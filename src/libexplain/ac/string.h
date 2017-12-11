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

#ifndef LIBEXPLAIN_AC_STRING_H
#define LIBEXPLAIN_AC_STRING_H

/**
  * @file
  * @brief Insulate <string.h> differences
  */

#include <libexplain/config.h>

#if STDC_HEADERS || HAVE_STRING_H
#  include <string.h>
   /* An ANSI string.h and pre-ANSI memory.h might conflict. */
#  if !STDC_HEADERS && HAVE_MEMORY_H
#    include <memory.h>
#  endif
#else
   /* memory.h and strings.h conflict on some systems. */
#  include <strings.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !HAVE_STRCASECMP
int strcasecmp(const char *, const char *);
#endif

#if !HAVE_STRNCASECMP
int strncasecmp(const char *, const char *, size_t);
#endif

#if !HAVE_DECL_STRSIGNAL
const char *strsignal(int);
#endif

const char *explain_strsignal(int);

#if !HAVE_DECL_STRVERSCMP
int strverscmp(const char *, const char *);
#endif

/**
  * The explain_strendcpy function is a buffer-overrun-safe replacement for
  * strcpy, strcat, and a more efficient replacement for strlcpy and
  * strlcat.
  *
  * Unless there is no space left in the buffer (dst >= end), the result
  * will always be NUL terminated.
  *
  * @param dst
  *     The position within the destination string buffer to be copied into.
  * @param src
  *     The string to be copied into the buffer.
  * @param end
  *     The end of the string buffer being copied into.  In most cases
  *     this is of the form "buffer + sizeof(buffer)", a constant which
  *     may be calculated at compile time.
  * @returns
  *     A pointer into the buffer where at the NUL terminator of the
  *     string in the buffer.  EXCEPT when an overrun would occur, in
  *     which case the \a end parameter is returned.
  *
  * @note
  * The return value is where the next string would be written into the
  * buffer.  For example, un-safe code such as
  *
  *     strcat(strcpy(buffer, "Hello, "), "World\n");
  *
  * can be safely replaced by
  *
  *     strendcpy(strendcpy(buffer, "Hello, ", buffer + sizeof(buffer)),
  *         "World\n", buffer + sizeof(buffer));
  *
  * and overruns will be handled safely.  Similarly, more complex string
  * manipulations can be written
  *
  *     char buffer[100];
  *     char *bp = buffer;
  *     bp = strendcpy(bp, "Hello, ", buffer + sizeof(buffer));
  *     bp = strendcpy(bp, "World!\n", buffer + sizeof(buffer));
  *
  * all that is required to test for an overrun is
  *
  *     if (bp == buffer + sizeof(buffer))
  *         fprintf(stderr, "Overrun!\n");
  *
  * On the plus side, there is only one functionto remember, not two,
  * replacing both strcpy and strcat.
  *
  * There have been some quite viable replacements for strcpy and strcat
  * in the BSD strlcpy and strlcat functions.  These functions are
  * indeed buffer-ovrrun-safe but they suffer from doing too much work
  * (and touching too much memory) in the case of overruns.
  *
  * Code such as
  *
  *     strlcpy(buffer, "Hello, ", sizeof(buffer));
  *     strlcat(buffer, "World!\n", sizeof(buffer));
  *
  * suffers from O(n**2) problem, constantly re-tracing the initial
  * portions of the buffer.  In addition, in the case of overruns, the
  * BSD versions of these functions return how big the buffer should
  * have been.  This functionality is rarely used, but still requires
  * the \a src to be traversed all the way to the NUL (and it could be
  * megabytes away) before they can return.  The strendcpy function does
  * not suffer from either of these performance problems.
  */
char *explain_strendcpy(char *dst, const char *src, const char *end);

#define strendcpy you_mean_explain_strendcpy!^%

#undef strcat
#define strcat strcat_is_unsafe__use_strendcpy_instead@
#undef strcpy
#define strcpy strcpy_is_unsafe__use_strendcpy_instead@

#ifndef HAVE_STRNSTR
char *strnstr(const char *haystack, const char *needle, size_t haystack_size);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LIBEXPLAIN_AC_STRING_H */
/* vim: set ts=8 sw=4 et : */
