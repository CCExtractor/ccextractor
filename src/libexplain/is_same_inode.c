/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/stat.h>

#include <libexplain/is_same_inode.h>


int
explain_is_same_inode(const struct stat *st1, const struct stat *st2)
{
    int             t1;
    int             t2;

    t1 = st1->st_mode & S_IFMT;
    t2 = st2->st_mode & S_IFMT;
    if (t1 != t2)
        return 0;
    switch (t1)
    {
    case S_IFBLK:
    case S_IFCHR:
        return (st1->st_rdev == st2->st_rdev);

    default:
        break;
    }
    return (st1->st_dev == st2->st_dev && st1->st_ino == st2->st_ino);
}


/* vim: set ts=8 sw=4 et : */
