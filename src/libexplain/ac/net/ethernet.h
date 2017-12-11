/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2012, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_AC_NET_ETHERNET_H
#define LIBEXPLAIN_AC_NET_ETHERNET_H

/**
  * @file
  * @brief Insulate <net/ethernet.h> differences
  */

#include <libexplain/ac/sys/types.h>

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#endif /* LIBEXPLAIN_AC_NET_ETHERNET_H */
/* vim: set ts=8 sw=4 et : */
