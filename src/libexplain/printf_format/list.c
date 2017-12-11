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

#include <libexplain/ac/stdlib.h>

#include <libexplain/printf_format.h>


void
explain_printf_format_list_constructor(explain_printf_format_list_t *lp)
{
    lp->length = 0;
    lp->maximum = 0;
    lp->list = 0;
}

void
explain_printf_format_list_destructor(explain_printf_format_list_t *lp)
{
    if (lp->list)
        free(lp->list);
    lp->length = 0;
    lp->maximum = 0;
    lp->list = 0;
}

void
explain_printf_format_list_clear(explain_printf_format_list_t *lp)
{
    lp->length = 0;
}


int
explain_printf_format_list_append(explain_printf_format_list_t *lp,
    const explain_printf_format_t *datum)
{
    if (lp->length >= lp->maximum)
    {
        size_t          j;
        size_t          new_maximum;
        size_t          nbytes;
        explain_printf_format_t *new_list;

        new_maximum = lp->maximum * 2 + 8;
        nbytes = new_maximum * sizeof(lp->list[0]);
        new_list = malloc(nbytes);
        if (!new_list)
            return -1;
        for (j = 0; j < lp->length; ++j)
            new_list[j] = lp->list[j];
        if (lp->list)
            free(lp->list);
        lp->list = new_list;
        lp->maximum = new_maximum;
    }
    lp->list[lp->length++] = *datum;
    return 0;
}


static int
cmp(const void *va, const void *vb)
{
    const explain_printf_format_t *a = va;
    const explain_printf_format_t *b = vb;
    return (a->index - b->index);
}


void
explain_printf_format_list_sort(explain_printf_format_list_t *lp)
{
    if (lp->length >= 2)
        qsort(lp->list, lp->length, sizeof(lp->list[0]), cmp);
}


/* vim: set ts=8 sw=4 et : */
