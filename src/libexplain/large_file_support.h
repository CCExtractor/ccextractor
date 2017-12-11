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

#ifndef LIBEXPLAIN_LARGE_FILE_SUPPORT_H
#define LIBEXPLAIN_LARGE_FILE_SUPPORT_H

#include <libexplain/public_config.h>

/**
  * @page lfs "Large File Support"
  *
  * You must enable large file support to be able to use libexplain.
  * If your project uses GNU Autoconf, it is simply a matter of adding
  * the AC_SYS_LARGEFILE macro to your configure.ac file.  If not, you
  * need to add -D_FILE_OFFSET_BITS=64 to your CFLAGS definition (this
  * is not completely portable, consider using GNU Autoconf instead).
  *
  * Large file support is needed in order to be able to access files
  * larger than 2GB in size.  Some fundamental data types, such as
  * off_t, are defined differently, and this has many follow-on effects.
  * A libexplain design decision was to support large files, and this in
  * turn means that clients of libexplain must also enable large file
  * support, so that libexplain and its client programs are using the
  * same data sizes.  (Failure to do so leads to hard-to-find memory
  * scribbles and segfaults.)
  */

#if ((defined(LIBEXPLAIN_FILE_OFFSET_BITS) && \
        (LIBEXPLAIN_FILE_OFFSET_BITS-0 != _FILE_OFFSET_BITS-0)) || \
     (defined(LIBEXPLAIN_LARGEFILE_SOURCE) && !defined(_LARGEFILE_SOURCE)) || \
     (defined(LIBEXPLAIN_LARGE_FILES) && !defined(_LARGE_FILES)))
#error "\
You must enable large file support to be able to use libexplain.  If \
your project uses GNU Autoconf, it is simply a matter of adding the \
AC_SYS_LARGEFILE macro to your configure.ac file.\
"
.large/file.support/error.
#endif

#endif /* LIBEXPLAIN_LARGE_FILE_SUPPORT_H */
/* vim: set ts=8 sw=4 et : */
