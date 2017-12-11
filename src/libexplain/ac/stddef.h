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

#ifndef LIBEXPLAIN_AC_STDDEF_H
#define LIBEXPLAIN_AC_STDDEF_H

/**
  * @file
  * @brief Insulate <stddef.h> differences
  */

#include <libexplain/config.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

/* for C89 compilance */
#ifndef offsetof
#define offsetof(a, b)  ((size_t)((char *)&((a *)0)->b - (char *)0))
#endif

/* always include this before <sys/types.h> */
#include <libexplain/ac/linux/types.h>

/*
 * Sun's Solaris 9 C compiler, and some other proprietary C compilers, are not
 * ANSI C 1989 conforming (let alone C99) and need the sys/types.h header as
 * well for size_t.  C'mon, people, it's been more than 20 years!
 */
#include <libexplain/ac/sys/types.h>

#endif /* LIBEXPLAIN_AC_STDDEF_H */
/* vim: set ts=8 sw=4 et : */
