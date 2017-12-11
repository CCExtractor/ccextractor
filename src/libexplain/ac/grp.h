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

#ifndef LIBEXPLAIN_AC_GRP_H
#define LIBEXPLAIN_AC_GRP_H

/**
  * @file
  * @brief Insulate <grp.h> differences
  */

#include <libexplain/ac/linux/types.h> /* Ubuntu Hardy needs this first */
#include <libexplain/ac/stddef.h>
#include <libexplain/ac/sys/types.h>
#include <grp.h>

#endif /* LIBEXPLAIN_AC_GRP_H */
/* vim: set ts=8 sw=4 et : */
