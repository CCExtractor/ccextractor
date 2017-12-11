/*
 * libexplain - a library of system-call-specific strerror replacements
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

#include <libexplain/ac/unistd.h>

#include <libexplain/getpagesize.h>


int
explain_getpagesize(void)
{
#ifdef HAVE_GETPAGESIZE
    return getpagesize();
#elif defined(HAVE_SYSCONF) && defined(_SC_PAGESIZE)
    return sysconf(_SC_PAGESIZE);
#else
    return -1;
#endif
}


/* vim: set ts=8 sw=4 et : */
