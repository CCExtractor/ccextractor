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

#include <libexplain/ac/sys/param.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/symloopmax.h>


int
explain_symloopmax(void)
{
    int             result;

    result = -1;
#ifdef _SC_SYMLOOP_MAX
    /* <unistd.h> */
    result = sysconf(_SC_SYMLOOP_MAX);
#endif
#ifdef SYMLOOP_MAX
    /* sys/param.h> */
    if (result <= 0)
        result = SYMLOOP_MAX;
#endif
#ifdef _POSIX_SYMLOOP_MAX
    /* sys/param.h> */
    if (result <= 0)
        result = _POSIX_SYMLOOP_MAX;
#endif

    /* if all else fails, fake it */
    if (result <= 0)
        result = 8;
    return result;
}


/* vim: set ts=8 sw=4 et : */
