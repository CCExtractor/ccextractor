/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/eagain.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/econnaborted.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/ehostdown.h>
#include <libexplain/buffer/ehostunreach.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/emfile.h>
#include <libexplain/buffer/enetdown.h>
#include <libexplain/buffer/enetunreach.h>
#include <libexplain/buffer/enfile.h>
#include <libexplain/buffer/enobufs.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enonet.h>
#include <libexplain/buffer/enoprotoopt.h>
#include <libexplain/buffer/enosr.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/enotsock.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/eproto.h>
#include <libexplain/buffer/eprotonosupport.h>
#include <libexplain/buffer/erestart.h>
#include <libexplain/buffer/errno/accept.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/esocktnosupport.h>
#include <libexplain/buffer/etimedout.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/socket_type.h>
#include <libexplain/buffer/socklen.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_accept_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, struct sockaddr *sock_addr,
    socklen_t *sock_addr_size)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "accept(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", sock_addr = ");
    explain_buffer_pointer(sb, sock_addr);
    explain_string_buffer_printf(sb, ", sock_addr_size = ");
    explain_buffer_socklen_star(sb, sock_addr_size);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_accept_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes, struct sockaddr *sock_addr,
    socklen_t *sock_addr_size)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/accept.html
     */
    (void)sock_addr;
    switch (errnum)
    {
    case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
#endif
        explain_buffer_eagain_accept(sb);
        break;

    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case ECONNABORTED:
        explain_buffer_econnaborted(sb);
        break;

    case EFAULT:
        if (explain_is_efault_pointer(sock_addr_size, sizeof(*sock_addr_size)))
            explain_buffer_efault(sb, "sock_addr_size");
        else
            explain_buffer_efault(sb, "sock_addr");
        break;

    case EHOSTDOWN:
        explain_buffer_ehostdown(sb);
        break;

    case EHOSTUNREACH:
        explain_buffer_ehostunreach(sb);
        break;

    case EINTR:
        explain_buffer_eintr(sb, "accept");
        break;

    case EINVAL:
        if
        (
            !explain_is_efault_pointer(sock_addr_size, sizeof(*sock_addr_size))
        &&
            *sock_addr_size <= 0
        )
        {
            explain_buffer_einval_too_small
            (
                sb,
                "sock_addr_size",
                *sock_addr_size
            );
            break;
        }
        explain_buffer_einval_not_listening(sb);
        break;

    case EMFILE:
        explain_buffer_emfile(sb);
        break;

    case ENETDOWN:
        explain_buffer_enetdown(sb);
        break;

    case ENETUNREACH:
        explain_buffer_enetunreach(sb);
        break;

    case ENFILE:
        explain_buffer_enfile(sb);
        break;

    case ENOBUFS:
        explain_buffer_enobufs(sb);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

#ifdef ENONET
    case ENONET:
        explain_buffer_enonet(sb);
        break;
#endif

    case ENOPROTOOPT:
        explain_buffer_enoprotoopt(sb, "fildes");
        break;

    case ENOTSOCK:
        explain_buffer_enotsock(sb, fildes, "fildes");
        break;

    case ENOSYS:
#if defined(EOPNOTSUPP) && EOPNOTSUPP != ENOSYS
    case EOPNOTSUPP:
#endif
        explain_buffer_enosys_socket(sb, "accept", fildes);
        break;

#ifdef ENOSR
    case ENOSR:
        explain_buffer_enosr(sb);
        break;
#endif

    case EPERM:
        explain_buffer_eperm_accept(sb);
        break;

#ifdef EPROTO
    case EPROTO:
        explain_buffer_eproto_accept(sb, fildes);
        break;
#endif

    case EPROTONOSUPPORT:
        explain_buffer_eprotonosupport(sb);
        break;

#ifdef ERESTART
    case ERESTART:
        explain_buffer_erestart(sb, "accept");
        break;
#endif

    case ESOCKTNOSUPPORT:
        explain_buffer_esocktnosupport(sb, "accept", fildes);
        break;

    case ETIMEDOUT:
        explain_buffer_etimedout(sb, "accept");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "accept");
        break;
    }
}


void
explain_buffer_errno_accept(explain_string_buffer_t *sb, int errnum,
    int fildes, struct sockaddr *sock_addr, socklen_t *sock_addr_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_accept_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        sock_addr,
        sock_addr_size
    );
    explain_buffer_errno_accept_explanation
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
