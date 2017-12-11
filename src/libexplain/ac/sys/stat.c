/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2012-2014 Peter Miller
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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/gcc_attributes.h>

#undef utimens

#ifndef HAVE_UTIMENS

/*
 * Set the access and modification time stamps of FILE to be
 * TIMESPEC[0] and TIMESPEC[1], respectively.
 */
int utimens(char const *path, const struct timespec timespec[2])
    LIBEXPLAIN_LINKAGE_HIDDEN;

int
utimens(char const *path, const struct timespec timespec[2])
{
#ifndef HAVE_UTIMENSAT
    (void)path;
    (void)timespec[0].tv_sec;
    errno = ENOSYS;
    return -1;
#else
    return utimensat(AT_FDCWD, path, timespec, 0);
#endif
}

#endif /* utimens */

#ifndef HAVE_LUTIMENS

int lutimens(char const *path, const struct timespec timespec[2])
    LIBEXPLAIN_LINKAGE_HIDDEN;

int
lutimens(char const *path, const struct timespec timespec[2])
{
#ifndef HAVE_UTIMENSAT
    (void)path;
    (void)timespec[0].tv_sec;
    errno = ENOSYS;
    return -1;
#else
    return utimensat(AT_FDCWD, path, timespec, AT_SYMLINK_NOFOLLOW);
#endif
}

#endif /* utimens */


/* vim: set ts=8 sw=4 et : */
