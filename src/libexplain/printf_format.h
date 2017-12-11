/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_PRINTF_FORMAT_H
#define LIBEXPLAIN_PRINTF_FORMAT_H

#include <libexplain/ac/stddef.h>

#define FLAG_ZERO (1 << 0)
#define FLAG_SPACE (1 << 1)
#define FLAG_HASH (1 << 2)
#define FLAG_PLUS (1 << 3)
#define FLAG_MINUS (1 << 4)
#define FLAG_THOUSANDS (1 << 5)
#define FLAG_I18N (1 << 6)

typedef struct explain_printf_format_t explain_printf_format_t;
struct explain_printf_format_t
{
    int             index;
    unsigned int    flags;
    int             width;
    int             precision;
    unsigned char   modifier;
    unsigned char   specifier;
};

typedef struct explain_printf_format_list_t explain_printf_format_list_t;
struct explain_printf_format_list_t
{
    size_t length;
    size_t maximum;
    explain_printf_format_t *list;
};

/**
  * The explain_printf_format_list_constructor function is used to
  * prepare a list for use.  Failure to call it will cause segfaults.
  *
  * @param lp
  *     The list to operate on.
  */
void explain_printf_format_list_constructor(explain_printf_format_list_t *lp);

/**
  * The explain_printf_format_list_destructor function is used to
  * release resources held by the list.  Failure to call it will cause
  * memory leaks.
  *
  * @param lp
  *     The list to operate on.
  */
void explain_printf_format_list_destructor(explain_printf_format_list_t *lp);

/**
  * The explain_printf_format_list_clear function is used to discard the
  * contents of a list.
  *
  * @param lp
  *     The list to operate on.
  */
void explain_printf_format_list_clear(explain_printf_format_list_t *lp);

/**
  * The explain_printf_format_list_append function is used to
  * add another formaat specifier to the list.
  *
  * @param lp
  *     The list to operate on.
  * @param datum
  *     The additional format spec.
  * @returns
  *     0 on success, -1 if malloc() fails.
  */
int explain_printf_format_list_append(explain_printf_format_list_t *lp,
    const explain_printf_format_t *datum);

/**
  * The explain_printf_format_list_sort function is used to sort the
  * format specifiers in the list, into ascending order of indexes.
  * This is how you figure out the order and types of the remaining
  * arguments.
  *
  * @param lp
  *     The list to operate on.
  */
void explain_printf_format_list_sort(explain_printf_format_list_t *lp);

/**
  * The explain_printf_format function may be used to parse an argument
  * format, and store the types of the arguments.  It understands
  * positional arguments, too.
  *
  * @param text
  *    The text (format specification) to be parsed.
  * @param result
  *    Where to place the discovered format specifiers.
  *    (Will be cleared first, and cleared again if there are errors.)
  * @returns
  *    0 if there were no errors,
  *    >0 if something was odd (number indicates position in string).
  */
size_t explain_printf_format(const char *text,
    explain_printf_format_list_t *result);

#include <libexplain/string_buffer.h>
#include <libexplain/ac/stdarg.h>

/**
  * The explain_print_format_representation function is used to parse a
  * format string, and the print representations of all the arguments.
  * This is used to print system call representations.
  * The format string has already been printed.
  *
  * @param sb
  *     When to print the representation
  * @param format
  *     The format string describing the rest of the arguments.
  * @param ap
  *     the rest of the arguments.
  */
void explain_printf_format_representation(explain_string_buffer_t *sb,
    const char *format, va_list ap);

#endif /* LIBEXPLAIN_PRINTF_FORMAT_H */
/* vim: set ts=8 sw=4 et : */
