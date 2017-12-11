/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_AC_SYS_TYPES_H
#define LIBEXPLAIN_AC_SYS_TYPES_H

/**
  * @file
  * @brief Insulate <sys/types.h> differences
  */

#include <libexplain/ac/stddef.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef HAVE_SYS_TYPES_OFF_T
# ifndef HAVE_OFF_T
#  ifndef off_t
#   define off_t long
#  endif
# endif
#endif

#ifndef HAVE_LOFF_T
typedef off_t loff_t;
#endif

#endif /* LIBEXPLAIN_AC_SYS_TYPES_H */
/* vim: set ts=8 sw=4 et : */
