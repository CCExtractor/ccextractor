/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/dirent.h>
#include <libexplain/ac/string.h>

#include <libexplain/count_directory_entries.h>


int
explain_count_directory_entries(const char *pathname)
{
    DIR             *dp;
    int             count;

    dp = opendir(pathname);
    if (!dp)
        return -1;
    count = 0;
    for (;;)
    {
        struct dirent   *dep;

        dep = readdir(dp);
        if (!dep)
            break;
        if (0 == strcmp(dep->d_name, "."))
            continue;
        if (0 == strcmp(dep->d_name, ".."))
            continue;
        ++count;
    }
    closedir(dp);
    return count;
}


/* vim: set ts=8 sw=4 et : */
