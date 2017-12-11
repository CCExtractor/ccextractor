/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/ac/limits.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/host_name_max.h>


long
explain_get_host_name_max(void)
{
#ifdef _SC_HOST_NAME_MAX
    /*
     * SUSv2 guarantees that "Host names are limited to 255 bytes".
     * POSIX.1-2001 guarantees that "Host names (not including the
     * terminating null byte) are limited to HOST_NAME_MAX bytes". On
     * Linux 1.* and 2.*, HOST_NAME_MAX is defined as 64.  Linux 0.x
     * kernels imposed a limit of 8 bytes.
     */
    {
        long            result;

        result = sysconf(_SC_HOST_NAME_MAX);
        if (result > 0)
            return result;
    }
#endif
#if HOST_NAME_MAX > 0
    return HOST_NAME_MAX;
#elif _POSIX_HOST_NAME_MAX > 0
    return _POSIX_HOST_NAME_MAX;
#else
    return 64;
#endif
}


/* vim: set ts=8 sw=4 et : */
