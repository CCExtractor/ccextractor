/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/string.h>

#include <libexplain/fileinfo.h>
#include <libexplain/lsof.h>
#include <libexplain/string_buffer.h>


typedef struct adapter adapter;
struct adapter
{
    explain_lsof_t  inherited;
    int             found;
};


static void
n_callback(explain_lsof_t *context, const char *name)
{
    adapter         *a;

    /*
     * Sometimes lsof(1) is less than helpful, and says "n (readlink:
     * Permission denied)" which is effectively no answer at all.
     *
     * There is a very small chance of discarding a valid result.
     * Get fussier if it proves to be an actual problem.
     */
    a = (adapter *)context;
    if (!strstr(name, " (readlink: "))
        return;
    a->found++;
}


int
explain_fileinfo_dir_tree_in_use(const char *path)
{
    adapter         obj;
    char            options[1000];
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, options, sizeof(options));
    explain_string_buffer_puts(&sb, "-- ");
    explain_string_buffer_puts_shell_quoted(&sb, path);

    obj.inherited.n_callback = n_callback;
    obj.found = 0;
    explain_lsof(options, &obj.inherited);
    return obj.found;
}


/* vim: set ts=8 sw=4 et : */
