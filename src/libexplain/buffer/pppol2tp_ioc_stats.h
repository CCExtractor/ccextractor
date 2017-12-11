/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_PPPOL2TP_IOC_STATS_H
#define LIBEXPLAIN_BUFFER_PPPOL2TP_IOC_STATS_H

#include <libexplain/string_buffer.h>

struct pppol2tp_ioc_stats; /* forward */

/**
  * The explain_buffer_pppol2tp_ioc_stats function may be used
  * to print a representation of a pppol2tp_ioc_stats structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The pppol2tp_ioc_stats structure to be printed.
  * @param reply
  *     true if this is the reply, false if it is the request
  */
void explain_buffer_pppol2tp_ioc_stats(explain_string_buffer_t *sb,
    const struct pppol2tp_ioc_stats *data, int reply);

#endif /* LIBEXPLAIN_BUFFER_PPPOL2TP_IOC_STATS_H */
/* vim: set ts=8 sw=4 et : */
