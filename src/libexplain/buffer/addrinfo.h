/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_ADDRINFO_H
#define LIBEXPLAIN_BUFFER_ADDRINFO_H

#include <libexplain/string_buffer.h>

struct addrinfo; /* forward */

/**
  * The explain_buffer_addrinfo function may be used to
  * print a representation of a struct addrinfo.
  *
  * @param sb
  *    The string buffer to print into.
  * @param ai
  *    The struct addrinfo to print.
  */
void explain_buffer_addrinfo(explain_string_buffer_t *sb,
    const struct addrinfo *ai);

#endif /* LIBEXPLAIN_BUFFER_ADDRINFO_H */
/* vim: set ts=8 sw=4 et : */
