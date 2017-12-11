/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_NAME_MAX_H
#define LIBEXPLAIN_NAME_MAX_H

/**
  * @file
  * @brief define NAME_MAX for paths
  *
  * Finding the definition of NAME_MAX can be a challenge.
  * This is defined to exist by POSIX.
  * It could be in <sys/param.h>
  * It could be in <limits.h>
  * And it could be called MAXNAMLEN in BSD land.
  */

#include <libexplain/ac/limits.h>
#include <libexplain/ac/sys/param.h>

#ifndef NAME_MAX
# if defined(MAXNAMLEN)
#  define NAME_MAX MAXNAMLEN
# elif defined(CONF_NAME_MAX) && (CONF_NAME_MAX > 0)
#  define NAME_MAX CONF_NAME_MAX
# else
#  define NAME_MAX 255
# endif
#endif

#endif /* LIBEXPLAIN_NAME_MAX_H */
/* vim: set ts=8 sw=4 et : */
