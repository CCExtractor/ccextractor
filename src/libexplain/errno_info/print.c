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

#include <libexplain/ac/stdio.h>
#include <libexplain/ac/string.h>

#include <libexplain/errno_info/table.h>
#include <libexplain/errno_info/print.h>


void
explain_errno_info_print(int all)
{
    size_t          j;

    for (j = 0; j < explain_errno_info_size; ++j)
    {
        const char      *s;
        const explain_errno_info_t *ep;

        ep = &explain_errno_info[j];
        if (!all && ep->description)
            continue;
        printf("#ifdef %s\n", ep->name);
        printf("    {\n");
        printf("        %s,\n", ep->name);
        printf("        \"%s\",\n", ep->name);
        s = strerror(ep->error_number);
        if (s && *s)
            printf("        \"%s\"\n", s);
        else
            printf("        0\n");
        printf("    },\n");
        printf("#endif\n");
    }
}


/* vim: set ts=8 sw=4 et : */
