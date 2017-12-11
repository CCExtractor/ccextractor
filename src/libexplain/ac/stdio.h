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

#ifndef LIBEXPLAIN_AC_STDIO_H
#define LIBEXPLAIN_AC_STDIO_H

/**
  * @file
  * @brief Insulate <stdio.h> differences
  */

#include <libexplain/config.h>

#include <stdio.h>

/*
 * Ancient pre-ANSI-C systems (e.g. SunOS 4.1.2) fail to define this.
 */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef L_tmpnam
#define L_tmpnam 100
#endif

#define sprintf sprintf_is_dangerous__use_snprintf_instead@
#define vsprintf sprintf_is_dangerous__use_vsnprintf_instead@

#ifndef HAVE_STDIO_OFF_T
# ifndef HAVE_OFF_T
#  ifndef off_t
#   define off_t long
#  endif
# endif
#endif

#endif /* LIBEXPLAIN_AC_STDIO_H */
/* vim: set ts=8 sw=4 et : */
