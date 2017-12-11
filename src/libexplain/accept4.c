/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/sys/socket.h>

#include <libexplain/accept4.h>
#include <libexplain/buffer/errno/accept4.h>
#include <libexplain/common_message_buffer.h>


const char *
explain_accept4(int fildes, struct sockaddr *sock_addr, socklen_t
    *sock_addr_size, int flags)
{
    return explain_errno_accept4(errno, fildes, sock_addr, sock_addr_size,
        flags);
}


const char *
explain_errno_accept4(int errnum, int fildes, struct sockaddr *sock_addr,
    socklen_t *sock_addr_size, int flags)
{
    explain_message_errno_accept4(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, fildes, sock_addr,
        sock_addr_size, flags);
    return explain_common_message_buffer;
}


void
explain_message_accept4(char *message, int message_size, int fildes, struct
    sockaddr *sock_addr, socklen_t *sock_addr_size, int flags)
{
    explain_message_errno_accept4(message, message_size, errno, fildes,
        sock_addr, sock_addr_size, flags);
}


void
explain_message_errno_accept4(char *message, int message_size, int errnum, int
    fildes, struct sockaddr *sock_addr, socklen_t *sock_addr_size, int flags)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_accept4(&sb, errnum, fildes, sock_addr, sock_addr_size,
        flags);
}


/* vim: set ts=8 sw=4 et : */
