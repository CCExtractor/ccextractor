/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_STRING_BUFFER_H
#define LIBEXPLAIN_STRING_BUFFER_H

#include <libexplain/ac/stdarg.h>
#include <libexplain/ac/stddef.h>

#include <libexplain/gcc_attributes.h>

#ifndef i18n
#define i18n(x) x
#endif

/**
  * The explain_string_buffer_t struct is used to represent a piece
  * of memory which is building a C string.
  * This does not require the use of dynamic memory.
  */
typedef struct explain_string_buffer_t explain_string_buffer_t;
struct explain_string_buffer_t
{
    char *message;
    size_t position;
    size_t maximum;
    explain_string_buffer_t *footnotes;
};

/**
  * The explain_string_buffer_init function may be used to prespare a
  * string buffer for use in building a C string.
  *
  * @param sb
  *    The string buffer to be initialised.
  * @param message
  *    The array which is to contain the string being constructed by the
  *    string buffer.
  * @param message_size
  *    The size (in bytes) of the array which is to contain the string
  *    being constructed by the string buffer.
  */
void explain_string_buffer_init(explain_string_buffer_t *sb,
    char *message, int message_size);

void explain_string_buffer_putc(explain_string_buffer_t *sb, int c);

/**
  * The explain_string_buffer_putc_quoted function is used to print
  * a C character constant (i.e., including the single quotes), with
  * escaping if necessary.
  *
  * @param sb
  *    The string buffer to be initialised.
  * @param c
  *    The character to be printed as a C character constant.
  */
void explain_string_buffer_putc_quoted(explain_string_buffer_t *sb,
    int c);

/**
  * The explain_string_buffer_putc_escaped function is used to print
  * a character with C escaping.  This can be used for the contents of
  * "string" and 'character' constants.
  *
  * @param sb
  *    The string buffer to be initialised.
  * @param c
  *    The character to be printed if possible, and C escaped if not.
  * @param delimiter
  *    The delimiter character; single quote for character constants,
  *    double quote for string constants
  */
void explain_string_buffer_putc_escaped(explain_string_buffer_t *sb,
    int c, int delimiter);

/*
 * The explain_string_buffer_puts function is used to print a string
 * into the string buffer.
 *
 * @param sb
 *     The string buffer to print into.
 * @param s
 *     The string to print into the string buffer.
 */
void explain_string_buffer_puts(explain_string_buffer_t *sb,
    const char *s);

/*
 * The explain_string_buffer_putsu function is used to print a string
 * into the string buffer.
 *
 * @param sb
 *     The string buffer to print into.
 * @param s
 *     The string to print into the string buffer.
 */
void explain_string_buffer_putsu(explain_string_buffer_t *sb,
    const unsigned char *s);

/*
 * The explain_string_buffer_puts_quoted function is used to print
 * a C string into the string buffer, complete with double quotes and
 * escape sequences if necessary.
 *
 * @param sb
 *     The string buffer to print into.
 * @param s
 *     The string to print into the string buffer.
 */
void explain_string_buffer_puts_quoted(explain_string_buffer_t *sb,
    const char *s);

/*
 * The explain_string_buffer_puts_shell_quoted function is used to print
 * a shell string into the string buffer, complete with double quotes and
 * sdingle quotes, and escape sequences if necessary.
 *
 * @param sb
 *     The string buffer to print into.
 * @param s
 *     The string to print into the string buffer.
 */
void explain_string_buffer_puts_shell_quoted(explain_string_buffer_t *sb,
    const char *s);

/*
 * The explain_string_buffer_putsu_quoted function is used to print
 * a C string into the string buffer, complete with double quotes and
 * escape sequences if necessary.
 *
 * @param sb
 *     The string buffer to print into.
 * @param s
 *     The string to print into the string buffer.
 */
void explain_string_buffer_putsu_quoted(explain_string_buffer_t *sb,
    const unsigned char *s);

/*
 * The explain_string_buffer_puts_quoted_n function is used to print
 * a C string into the string buffer, complete with double quotes and
 * escape sequences if necessary.  The length is limited to that given,
 * unless a NUL is seen earlier.
 *
 * @param sb
 *     The string buffer to print into.
 * @param s
 *     The string to print into the string buffer.
 * @param n
 *     The maximum length of the string.
 */
void explain_string_buffer_puts_quoted_n(explain_string_buffer_t *sb,
    const char *s, size_t n);

/*
 * The explain_string_buffer_putsu_quoted_n function is used to print
 * a C string into the string buffer, complete with double quotes and
 * escape sequences if necessary.  The length is limited to that given,
 * unless a NUL is seen earlier.
 *
 * @param sb
 *     The string buffer to print into.
 * @param s
 *     The string to print into the string buffer.
 * @param n
 *     The maximum length of the string.
 */
void explain_string_buffer_putsu_quoted_n(explain_string_buffer_t *sb,
    const unsigned char *s, size_t n);

/**
  * The explain_string_buffer_printf function is used to print a
  * formatted string into a string buffer.
  *
  * @param sb
  *     The string buffer to print into
  * @param fmt
  *     The format string controling the types of the remaining arguments.
  *     See printf(3) for more information.
  */
void explain_string_buffer_printf(explain_string_buffer_t *sb,
    const char *fmt, ...)
                                                 LIBEXPLAIN_FORMAT_PRINTF(2, 3);

/**
  * The explain_string_buffer_printf function is used to print a
  * formatted string into a string buffer, translating the formatting text.
  *
  * @param sb
  *     The string buffer to print into
  * @param fmt
  *     The format string controling the types of the remaining
  *     arguments.  See printf(3) for more information on the content of
  *     this string.  It will be passed through explain_gettext for
  *     translation before it is used.
  */
void explain_string_buffer_printf_gettext(explain_string_buffer_t *sb,
    const char *fmt, ...)
                                                 LIBEXPLAIN_FORMAT_PRINTF(2, 3);

void explain_string_buffer_vprintf(explain_string_buffer_t *sb,
    const char *fmt, va_list ap);

void explain_string_buffer_copy(explain_string_buffer_t *dst,
    const explain_string_buffer_t *src);

void explain_string_buffer_path_join(explain_string_buffer_t *sb,
    const char *s);

/**
  * The explain_string_buffer_full functionmay be used to determine
  * whether or not a string buffer is full (has no more room for
  * additional characters).
  *
  * @param sb
  *     The string buffer to test.
  */
int explain_string_buffer_full(const explain_string_buffer_t *sb);

/**
  * The explain_string_buffer_write function may be used to append
  * a run of bytes to the end of a string buffer.  They do not need to
  * be NUL terminated; indeed, they shall <b>not</b> contain any NUL
  * characters.
  *
  * @param sb
  *     The string buffer to print into
  * @param data
  *     Pointer to the base of the array of characters to be written
  *     into the string buffer.
  * @param data_size
  *     The size in bytes (printable characters could be multi-byte
  *     sequences) the size in bytes of the array of bytes to write into
  *     the string buffer.
  */
void explain_string_buffer_write(explain_string_buffer_t *sb,
    const char *data, size_t data_size);

/**
  * The explain_string_buffer_truncate function is used to trim a
  * string buffer back to the given size.
  *
  * @param sb
  *     The string buffer to shorten
  * @param new_position
  *     The newer shorter position (ignored of longer)
  */
void explain_string_buffer_truncate(explain_string_buffer_t *sb,
    long new_position);

/**
  * The explain_string_buffer_rewind function is used to trim a
  * string buffer back to empty.
  *
  * @param sb
  *     The string buffer to shorten
  */
void explain_string_buffer_rewind(explain_string_buffer_t *sb);

#endif /* LIBEXPLAIN_STRING_BUFFER_H */
/* vim: set ts=8 sw=4 et : */
