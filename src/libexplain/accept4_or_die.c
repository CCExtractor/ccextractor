/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2012, 2013 Peter Miller
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
#include <libexplain/output.h>


int
explain_accept4_on_error(int fildes, struct sockaddr *sock_addr,
    socklen_t *sock_addr_size, int flags)
{
    int             result;

#ifdef HAVE_ACCEPT4
    result = accept4(fildes, sock_addr, sock_addr_size, flags);
#else
    result = -1;
    errno = ENOSYS;
#endif
    if (result < 0)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_accept4
            (
                hold_errno,
                fildes,
                sock_addr,
                sock_addr_size,
                flags
            )
        );
        errno = hold_errno;
    }
    return result;
}


int
explain_accept4_or_die(int fildes, struct sockaddr *sock_addr, socklen_t
    *sock_addr_size, int flags)
{
    int             result;

    result = explain_accept4_on_error(fildes, sock_addr, sock_addr_size, flags);
    if (result < 0)
    {
        explain_output_exit_failure();
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
