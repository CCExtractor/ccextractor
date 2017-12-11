/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2013 Peter Miller
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

#ifndef LIBEXPLAIN_AC_FCNTL_H
#define LIBEXPLAIN_AC_FCNTL_H

/**
  * @file
  * @brief Insulate <fcntl.h> differences
  */

#include <libexplain/config.h>

#ifdef HAVE_FCNTL_H
#if defined(__alpha__) && defined(HAVE_LINUX_FCNTL_H)
#include <libexplain/ac/linux/types.h> /* Ubuntu Hardy needs this first */
/* This is very strange, but numerous tests fail if we use <fcntl.h>
   because it appears to have several incorrect macro definitions */
#include <libexplain/ac/linux/fcntl.h>
#else
#include <fcntl.h>
#endif
#else
#include <libexplain/ac/sys/file.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_TEXT
#define O_TEXT 0
#endif

/*
 * Needed for explain_buffer_errno_path_resolution
 */
#ifndef O_DIRECTORY
#define O_DIRECTORY 0x40000000  /* or O_RDONLY */
#endif

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

/*
 * Even when O_LARGEFILE is not necessary, glibc adds one in for free.
 * The trouble is, this make things interesting when decoding
 * flags values returned by the kernel.
 */
#if defined(__linux__) && (O_LARGEFILE == 0)
#define O_LARGEFILE_HIDDEN 0100000
#endif


/*
 * From /usr/include/sharutils/fcntl.h ...
 *
 * Work around a bug in Solaris 9 and 10: AT_FDCWD is positive.
 * Its value exceeds INT_MAX, so its use as an int doesn't conform to
 * the C standard, and GCC and Sun C complain in some cases.  If the
 * bug is present, undef AT_FDCWD here, so it can be redefined.
 */
#if 0 < AT_FDCWD && AT_FDCWD == 0xffd19553
#undef AT_FDCWD

/*
 * Use the same bit pattern as Solaris 9, but with the proper signedness.
 * The bit pattern is important, in case this actually is Solaris with
 * the above workaround.
 */
#define AT_FDCWD (-3041965)
#endif


/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_AC_FCNTL_H */
