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

#ifndef LIBEXPLAIN_BUFFER_ERRNO_SELECT_H
#define LIBEXPLAIN_BUFFER_ERRNO_SELECT_H

#include <libexplain/ac/sys/select.h>

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_errno_select function
  * is used to obtain an explanation of an error returned
  * by the select(2) system call.
  * The least the message will contain is the value of
  * strerror(errnum), but usually it will do much better,
  * and indicate the underlying cause in more detail.
  *
  * @param sb
  *     The string buffer to print the message into.  If a
  *     safe buffer is specified, this function is thread
  *     safe.
  * @param errnum
  *     The error value to be decoded, usually obtained
  *     from the errno global variable just before this
  *     function is called.  This is necessary if you need
  *     to call <b>any</b> code between the system call to
  *     be explained and this function, because many libc
  *     functions will alter the value of errno.
  * @param nfds
  *     The original nfds, exactly as passed to the select(2) system call.
  * @param readfds
  *     The original readfds, exactly as passed to the select(2) system call.
  * @param writefds
  *     The original writefds, exactly as passed to the select(2) system call.
  * @param exceptfds
  *     The original exceptfds, exactly as passed to the select(2) system call.
  * @param timeout
  *     The original timeout, exactly as passed to the select(2) system call.
  */
void explain_buffer_errno_select(explain_string_buffer_t *sb, int errnum,
    int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
    struct timeval *timeout);

#endif /* LIBEXPLAIN_BUFFER_ERRNO_SELECT_H */
/* vim: set ts=8 sw=4 et : */
