/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_AC_UNISTD_H
#define LIBEXPLAIN_AC_UNISTD_H

/**
  * @file
  * @brief insulate <unistd.h> differences
  */

#include <libexplain/config.h>

/*
 * Need to define _BSD_SOURCE on Linux to get prototypes for the symlink
 * and readlink functions.
 */
#ifdef __linux__
#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif
#ifndef __USE_BSD
#define __USE_BSD 1
#endif
#endif

#ifdef HAVE_UNISTD_H
#include <libexplain/ac/linux/types.h> /* Ubuntu Hardy needs this first */
#include <libexplain/ac/sys/types.h>
#include <unistd.h>
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef F_OK
#define F_OK 0
#endif

/*
 * This is supposed to be declared in <unistd.h> by modern POSIX
 * impelementations.  Sadly, some systems in common use get this wrong.
 */
extern char **environ;

#endif /* LIBEXPLAIN_AC_UNISTD_H */
/* vim: set ts=8 sw=4 et : */
