/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enobufs.h>
#include <libexplain/buffer/enotconn.h>
#include <libexplain/buffer/enotsock.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/getsockname.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/socklen.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_getsockname_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, struct sockaddr *sock_addr,
    socklen_t *sock_addr_size)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "getsockname(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", sock_addr = ");
    explain_buffer_pointer(sb, sock_addr);
    explain_string_buffer_puts(sb, ", sock_addr_size = ");
    explain_buffer_socklen_star(sb, sock_addr_size);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_getsockname_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes, struct sockaddr *sock_addr,
    socklen_t *sock_addr_size)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/getsockname.html
     */
    (void)sock_addr;
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EFAULT:
        if (explain_is_efault_pointer(sock_addr_size, sizeof(*sock_addr_size)))
            explain_buffer_efault(sb, "sock_addr_size");
        else
            explain_buffer_efault(sb, "sock_addr");
        break;

    case EINVAL:
        if
        (
            explain_is_efault_pointer
            (
                sock_addr_size,
                sizeof(*sock_addr_size)
            )
        )
            goto dunno;
        explain_buffer_einval_too_small
        (
            sb,
            "sock_addr_size",
            (int)*sock_addr_size
        );
        break;

    case ENOBUFS:
        explain_buffer_enobufs(sb);
        break;

    case ENOTCONN:
        explain_buffer_enotconn(sb, "fildes");
        break;

    case ENOTSOCK:
        explain_buffer_enotsock(sb, fildes, "fildes");
        break;

    default:
        dunno:
        explain_buffer_errno_generic(sb, errnum, "getsockname");
        break;
    }
}


void
explain_buffer_errno_getsockname(explain_string_buffer_t *sb, int errnum,
    int fildes, struct sockaddr *sock_addr, socklen_t *sock_addr_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_getsockname_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        sock_addr,
        sock_addr_size
    );
    explain_buffer_errno_getsockname_explanation
    (
        &exp.explanation_sb,
        errnum,
        fildes,
        sock_addr,
        sock_addr_size
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
