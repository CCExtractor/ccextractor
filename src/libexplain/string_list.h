/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_STRING_LIST_H
#define LIBEXPLAIN_STRING_LIST_H

#include <libexplain/ac/stddef.h>

typedef struct explain_string_list_t explain_string_list_t;
struct explain_string_list_t
{
    size_t          length;
    size_t          maximum;
    char            **string;
};


/**
  * The explain_string_list_split function may be used to break a string
  * to to several string in a string_list_t.  The parts are separated by
  * white space.
  *
  * @param result
  *    Where to put the strings.
  * @param text
  *    The text to be split into words.
  */
void explain_string_list_split(explain_string_list_t *result, const char *text);

/**
  * The explain_string_list_constructor function is used to prepare a
  * string list for use.
  *
  * @param slp
  *     The string list to operate on.
  */
void explain_string_list_constructor(explain_string_list_t *slp);

/**
  * The explain_string_list_destruct function is used to release the
  * resources held by a string list, when you are done with it.
  *
  * @param slp
  *     The string list to operate on.
  */
void explain_string_list_destructor(explain_string_list_t *slp);

/**
  * The explain_string_list_append function is used to add another
  * string to the end of the list.  The string is *always* copied.
  *
  * @param slp
  *     The string list to operate on.
  * @param text
  *     The string to be appended.
  */
void explain_string_list_append(explain_string_list_t *slp, const char *text);

/**
  * The explain_string_list_append_unique function is used to add
  * another string to the end of the list, provided that the string is
  * not already in the list.  The string is *always* copied if it is used.
  *
  * @param slp
  *     The string list to operate on.
  * @param text
  *     The string to be appended.
  */
void explain_string_list_append_unique(explain_string_list_t *slp,
    const char *text);

/**
  * The explain_string_list_append_n function is used to add another
  * string to the end of the list.  The string is *always* copied.
  *
  * @param slp
  *     The string list to operate on.
  * @param text
  *     The string to be appended.
  * @param tsize
  *     The length of the string to be appended.
  */
void explain_string_list_append_n(explain_string_list_t *slp, const char *text,
    size_t tsize);

/**
  * The explain_string_list_sort function is used to sort a string list
  * into ascending lexicographical order.
  *
  * @param slp
  *     The string list to operate on.
  */
void explain_string_list_sort(explain_string_list_t *slp);

#endif /* LIBEXPLAIN_STRING_LIST_H */
/* vim: set ts=8 sw=4 et : */
