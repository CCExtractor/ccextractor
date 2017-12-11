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

#ifndef LIBEXPLAIN_BUFFER_SOCK_FPROG_H
#define LIBEXPLAIN_BUFFER_SOCK_FPROG_H

#include <libexplain/string_buffer.h>

struct sock_fprog; /* forward */

/**
  * The explain_buffer_sock_fprog function may be used
  * to print a representation of a sock_fprog structure.
  *
  * @param sb
  *     The string buffer to print into.
  * @param data
  *     The sock_fprog structure to be printed.
  */
void explain_buffer_sock_fprog(explain_string_buffer_t *sb,
    const struct sock_fprog *data);

#endif /* LIBEXPLAIN_BUFFER_SOCK_FPROG_H */
/* vim: set ts=8 sw=4 et : */
