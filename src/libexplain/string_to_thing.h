/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2012, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_STRING_TO_THING_H
#define LIBEXPLAIN_STRING_TO_THING_H

#include <libexplain/ac/stddef.h>
#include <libexplain/ac/sys/socket.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/unistd.h>

/**
  * The explain_string_to_thing function may be used to convert text to
  * a size_t value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a size_t value.  On error, prints an error message
  *    and does not return.
  */
size_t explain_parse_size_t_or_die(const char *text);

/**
  * The explain_parse_ssize_t_or_die function may be used to convert text to
  * a ssize_t value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a ssize_t value.  On error, prints an error message
  *    and does not return.
  */
ssize_t explain_parse_ssize_t_or_die(const char *text);

/**
  * The explain_string_to_thing function may be used to convert text to
  * a ptrdiff_t value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a ptrdiff_t value.  On error, prints an error message
  *    and does not return.
  */
ptrdiff_t explain_parse_ptrdiff_t_or_die(const char *text);

/**
  * The explain_string_to_thing function may be used to convert text to
  * a off_t value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a off_t value.  On error, prints an error message
  *    and does not return.
  */
off_t explain_parse_off_t_or_die(const char *text);

/**
  * The explain_string_to_thing function may be used to convert text to
  * a pointer value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a pointer value.  On error, prints an error message
  *    and does not return.
  */
void *explain_parse_pointer_or_die(const char *text);

/**
  * The explain_parse_long_or_die function may be used to convert text to
  * a long value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a pointer value.  On error, prints an error message
  *    and does not return.
  */
long explain_parse_long_or_die(const char *text);

/**
  * The explain_string_to_ulong function may be used to convert text to
  * an unsigned long value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a pointer value.  On error, prints an error message
  *    and does not return.
  */
unsigned long explain_parse_ulong_or_die(const char *text);

/**
  * The explain_string_to_longlong function may be used to convert text
  * to a long long value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a pointer value.  On error, prints an error message
  *    and does not return.
  */
long long explain_parse_longlong_or_die(const char *text);

/**
  * The explain_string_to_ulonglong function may be used to convert text
  * to an unsigned long long value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a pointer value.  On error, prints an error message
  *    and does not return.
  */
unsigned long long explain_parse_ulonglong_or_die(const char *text);

/**
  * The explain_string_to_socklen_t function may be used to convert text
  * to a socklen_t value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a pointer value.  On error, prints an error message
  *    and does not return.
  */
socklen_t explain_parse_socklen_t_or_die(const char *text);

/**
  * The explain_parse_int_or_die function may be used to convert text
  * to a int value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a pointer value.  On error, prints an error message
  *    and does not return.
  */
int explain_parse_int_or_die(const char *text);

/**
  * The explain_string_to_uint function may be used to convert text to
  * an unsigned int value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a pointer value.  On error, prints an error message
  *    and does not return.
  */
unsigned explain_parse_uint_or_die(const char *text);

/**
  * The explain_string_to_double function may be used to convert text to
  * a double-precision floating-point value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a floating-point value.  On error, prints an error
  *    message and does not return.
  */
double explain_parse_double_or_die(const char *text);

/**
  * The explain_string_to_float function may be used to convert text to
  * a single-precision floating-point value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a floating-point value.  On error, prints an error
  *    message and does not return.
  */
float explain_parse_float_or_die(const char *text);

/**
  * The explain_string_to_long_double function may be used to convert
  * text to a triple-precision floating-point value.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a floating-point value.  On error, prints an error
  *    message and does not return.
  */
long double explain_parse_long_double_or_die(const char *text);

/**
  * The explain_parse_uid_t_or_die function may be used to convert text
  * to a user ID.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a uid.  On error, prints an error message and does
  *    not return.
  */
uid_t explain_parse_uid_t_or_die(const char *text);

/**
  * The explain_parse_gid_t_or_die function may be used to convert text
  * to a group ID.
  *
  * @param text
  *    The text to be converted.
  * @returns
  *    On success, a gid.  On error, prints an error message and does
  *    not return.
  */
gid_t explain_parse_gid_t_or_die(const char *text);

int explain_parse_bool_or_die(const char * text);

FILE *explain_parse_stream_or_die(const char *text, const char *mode);

#endif /* LIBEXPLAIN_STRING_TO_THING_H */
/* vim: set ts=8 sw=4 et : */
