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

#include <libexplain/ac/fcntl.h> /* for AT_FDCWD */
#include <libexplain/ac/sys/stat.h>

#include <libexplain/fildes_is_dot.h>


int
explain_fildes_is_dot(int fildes)
{
    struct stat st1;
    struct stat st2;

    if (fildes == AT_FDCWD)
        return 1;
    if (stat(".", &st1) < 0)
        return 0;
    if (fstat(fildes, &st2) < 0)
        return 0;
    return (st1.st_ino == st2.st_ino && st1.st_dev == st2.st_dev);
}



/* vim: set ts=8 sw=4 et : */
