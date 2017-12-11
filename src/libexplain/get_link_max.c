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

#include <libexplain/ac/limits.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/get_link_max.h>


long
explain_get_link_max(const char *pathname)
{
    long            result;

    /* see aso MINIX_LINK_MAX in <linux/minix_fs.h> */
    /* see aso MINIX2_LINK_MAX in <linux/minix_fs.h> */
    /* see aso LINK_MAX in <linux/limits.h> */
    /* see aso EXT2_LINK_MAX in <linux/ext2_fs.h> */
    /* see aso _POSIX_LINK_MAX in <bits/posix_lim1.h> */
    result = pathconf(pathname, _PC_LINK_MAX);
    if (result <= 0)
    {
#ifdef LINK_MAX
        result = LINK_MAX;
#else
        result = _POSIX_LINK_MAX;
#endif
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
