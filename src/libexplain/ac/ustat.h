/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_AC_USTAT_H
#define LIBEXPLAIN_AC_USTAT_H

/**
  * @file
  * @brief Insulate <ustat.h> differences
  */

#include <libexplain/ac/sys/ustat.h>

#ifndef LINUX_TYPES_H_STRUCT_USTAT
#ifdef HAVE_USTAT_H
#include <ustat.h>
#endif
#endif

struct ustat; /* forward */

#if !HAVE_DECL_USTAT
int ustat(dev_t dev, struct ustat *data);
#endif

#endif /* LIBEXPLAIN_AC_USTAT_H */
/* vim: set ts=8 sw=4 et : */
