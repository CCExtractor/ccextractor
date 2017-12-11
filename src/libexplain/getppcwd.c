/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#include <libexplain/ac/unistd.h>

#include <libexplain/fileinfo.h>
#include <libexplain/getppcwd.h>


char *
explain_getppcwd(char *data, size_t data_size)
{
    pid_t           ppid;

    ppid = getppid();
    if (ppid < 0)
        return NULL;
    if (!explain_fileinfo_pid_cwd(ppid, data, data_size))
        return NULL;
    return data;
}


/* vim: set ts=8 sw=4 et : */
