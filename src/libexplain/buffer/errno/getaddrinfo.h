/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
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

#ifndef LIBEXPLAIN_BUFFER_ERRNO_GETADDRINFO_H
#define LIBEXPLAIN_BUFFER_ERRNO_GETADDRINFO_H

#include <libexplain/string_buffer.h>

struct addrinfo; /* forward */

/**
  * The explain_buffer_errcode_getaddrinfo function is used to
  * obtain an explanation of an error returned by the getaddrinfo(3)
  * system call.  The least the message will contain is the value of
  * gai_strerror(errcode), but usually it will do much better, and
  * indicate the underlying cause in more detail.
  *
  * @param sb
  *     The string buffer to print the message into.  If a safe buffer
  *     is specified, this function is thread safe.
  * @param errcode
  *     The error value to be decoded, as returned by the getaddrinfo
  *     system call.
  * @param node
  *     The original node, exactly as passed to the getaddrinfo(3)
  *     system call.
  * @param service
  *     The original service, exactly as passed to the getaddrinfo(3)
  *     system call.
  * @param hints
  *     The original hints, exactly as passed to the getaddrinfo(3)
  *     system call.
  * @param res
  *     The original res, exactly as passed to the getaddrinfo(3) system
  *     call.
  */
void explain_buffer_errcode_getaddrinfo(explain_string_buffer_t *sb,
    int errcode, const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **res);

#endif /* LIBEXPLAIN_BUFFER_ERRNO_GETADDRINFO_H */
/* vim: set ts=8 sw=4 et : */
