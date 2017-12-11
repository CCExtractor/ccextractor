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

#ifndef LIBEXPLAIN_BUFFER_EAFNOSUPPORT_H
#define LIBEXPLAIN_BUFFER_EAFNOSUPPORT_H

#include <libexplain/string_buffer.h>

struct sockaddr; /* forward */

/**
  * The explain_buffer_eafnosupport function may be used to explan an
  * EAFNOSUPPORT error, which occurs when a socket attempts to bind or
  * connect to an address of a different address family.
  *
  * @param sb
  *    The string buffer to print into.
  * @param fildes
  *    The file descriptor in question.
  * @param fildes_caption
  *    The name of the system call argument that holds the file
  *    descriptor in question.
  * @param sock_addr
  *    The address that attempted to bind or connect
  * @param sock_addr_caption
  *    The name of the system call argument that holds the address that
  *    attempted to bind or connect
  */
void explain_buffer_eafnosupport(explain_string_buffer_t *sb, int fildes,
    const char *fildes_caption, const struct sockaddr *sock_addr,
    const char *sock_addr_caption);

#endif /* LIBEXPLAIN_BUFFER_EAFNOSUPPORT_H */
/* vim: set ts=8 sw=4 et : */
