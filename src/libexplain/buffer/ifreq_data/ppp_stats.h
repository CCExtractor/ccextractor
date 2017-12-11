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

#ifndef LIBEXPLAIN_BUFFER_IFREQ_DATA_PPP_STATS_H
#define LIBEXPLAIN_BUFFER_IFREQ_DATA_PPP_STATS_H

#include <libexplain/string_buffer.h>

struct ifreq; /* forward */

/**
  * The explain_buffer_ifreq_data_ppp_stats function may be used
  * to print a representation of a ppp_stats structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The ifr_data->ppp_stats structure to be printed.
  * @param complete
  *     whether opr not to print the complete ifr_data or just the pointer
  */
void explain_buffer_ifreq_data_ppp_stats(explain_string_buffer_t *sb,
    const struct ifreq *data, int complete);

#endif /* LIBEXPLAIN_BUFFER_IFREQ_DATA_PPP_STATS_H */
/* vim: set ts=8 sw=4 et : */
