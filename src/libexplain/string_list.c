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

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>

#include <libexplain/malloc.h>
#include <libexplain/string_list.h>
#include <libexplain/strndup.h>


void
explain_string_list_constructor(explain_string_list_t *slp)
{
    slp->length = 0;
    slp->maximum = 0;
    slp->string = 0;
}


void
explain_string_list_destructor(explain_string_list_t *slp)
{
    size_t          j;

    for (j = 0; j < slp->length; ++j)
    {
        free(slp->string[j]);
        slp->string[j] = 0;
    }
    slp->length = 0;
    slp->maximum = 0;
    if (slp->string)
    {
        free(slp->string);
        slp->string = 0;
    }
}


static int
explain_string_list_member(const explain_string_list_t *slp, const char *text)
{
    size_t          j;

    for (j = 0; j < slp->length; ++j)
        if (0 == strcmp(slp->string[j], text))
            return 1;
    return 0;
}


void
explain_string_list_append_unique(explain_string_list_t *slp, const char *text)
{
    if (!explain_string_list_member(slp, text))
        explain_string_list_append(slp, text);
}


void
explain_string_list_append(explain_string_list_t *slp, const char *text)
{
    explain_string_list_append_n(slp, text, strlen(text));
}


void
explain_string_list_append_n(explain_string_list_t *slp, const char *text,
    size_t text_size)
{
    if (slp->length >= slp->maximum)
    {
        char            **new_string;
        size_t          j;

        slp->maximum = slp->maximum * 2 + 16;
        new_string = explain_malloc_or_die(slp->maximum * sizeof(*new_string));
        for (j = 0; j < slp->length; ++j)
            new_string[j] = slp->string[j];
        if (slp->string)
            free(slp->string);
        slp->string = new_string;
    }
    slp->string[slp->length++] = explain_strndup_or_die(text, text_size);
}


void
explain_string_list_split(explain_string_list_t *result, const char *text)
{
    for (;;)
    {
        unsigned char   c;
        const char      *start;

        start = text;
        c = *text++;
        if (!c)
            return;
        if (isspace(c))
            continue;
        for (;;)
        {
            c = *text;
            if (!c || isspace(c))
                break;
            ++text;
        }
        explain_string_list_append_n(result, start, text - start);
    }
}


static int
cmp(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}


void
explain_string_list_sort(explain_string_list_t *slp)
{
    if (slp->length >= 2)
        qsort(slp->string, slp->length, sizeof(slp->string[0]), cmp);
}


/* vim: set ts=8 sw=4 et : */
