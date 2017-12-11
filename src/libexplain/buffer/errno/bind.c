/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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
#include <libexplain/ac/net/if.h>
#include <libexplain/ac/sys/socket.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/sys/un.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/eaddrinuse.h>
#include <libexplain/buffer/eafnosupport.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/enotsock.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/bind.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/explanation.h>
#include <libexplain/fildes_to_address_family.h>


static void
explain_buffer_errno_bind_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, const struct sockaddr *sock_addr,
    int sock_addr_size)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "bind(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", sock_addr = ");
    explain_buffer_sockaddr(sb, sock_addr, sock_addr_size);
    explain_string_buffer_printf
    (
        sb,
        ", sock_addr_size = %d",
        sock_addr_size
    );
    explain_string_buffer_putc(sb, ')');
}


static void
explain_final_sock_init(explain_final_t *fp)
{
    explain_final_init(fp);
    fp->want_to_create = 1;
    fp->want_to_read = 1;
    fp->must_be_a_st_mode = 1;
    fp->st_mode = S_IFSOCK;
}


static void
the_socket_is_already_bound(explain_string_buffer_t *sb, int fildes)
{
    struct sockaddr_storage sas;
    struct sockaddr *sas_p;
    socklen_t       sa_len;

    sas_p = (struct sockaddr *)&sas;
    sa_len = sizeof(sas);
    if (getsockname(fildes, sas_p, &sa_len) >= 0)
    {
        char            addr[500];
        explain_string_buffer_t addr_sb;

        explain_string_buffer_init(&addr_sb, addr, sizeof(addr));
        explain_buffer_sockaddr(&addr_sb, sas_p, sa_len);
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an EINVAL error
             * returned by the bind(2) system call, in the case where
             * the socket is already bound to an address.
             *
             * %1$s => a representation of the struct sockaddr that the
             *         socket is already bound to.
             */
            i18n("the socket is already bound to %s"),
            addr
        );
    }
    else
    {
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an EINVAL
             * error returned by the bind(2) system call, in the case
             * where the socket is already bound to an address, but the
             * address cannot be determined.
             */
            i18n("the socket is already bound to an address")
        );
    }
}


static void
explain_buffer_errno_bind_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes, const struct sockaddr *sock_addr,
    int sock_addr_size)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/bind.html
     */
    (void)sock_addr_size;
    switch (errnum)
    {
    case EACCES:
        if (sock_addr->sa_family == AF_UNIX)
        {
            explain_final_t final_component;
            const struct sockaddr_un *saddr;

            saddr = (const struct sockaddr_un *)sock_addr;
            explain_final_sock_init(&final_component);
            final_component.want_to_write = 1;
            final_component.want_to_create = 1;
            final_component.must_be_a_st_mode = 1;
            final_component.st_mode = S_IFSOCK;
            final_component.path_max = sizeof(saddr->sun_path) - 1;
            explain_buffer_eacces
            (
                sb,
                saddr->sun_path,
                "sock_addr->sun_path",
                &final_component
            );
        }
        else
        {
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: This message is used to explain an EACCES
                 * error reported by a bind(2) system call, in the case
                 * where a privileged port is specific, and the process
                 * does not have permission.
                 */
                i18n("the network port address is protected")
            );
            explain_buffer_dac_net_bind_service(sb);
        }
        break;

    case EADDRINUSE:
        if (sock_addr->sa_family == AF_UNIX)
        {
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: This message is used when an AF_UNIX socket
                 * file already exists when it should not.  While the
                 * bind(2) call will create the entry in the file
                 * system, the correponding close(2) will not remove it
                 * again, the programmer must explicitly unlink(2) it.
                 */
                i18n("the socket file already exists, and it should "
                "not; when you are done with AF_UNIX sockets you must "
                "deliberately unlink(2) the socket file, it does not "
                "happen automatically")
            );
            break;
        }
        explain_buffer_eaddrinuse(sb, fildes);
        break;

    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EAFNOSUPPORT:
        explain_buffer_eafnosupport
        (
            sb,
            fildes,
            "fildes",
            sock_addr,
            "sock_addr"
        );
        break;

    case EINVAL:
        if (sock_addr->sa_family == AF_UNIX)
        {
            if (explain_fildes_to_address_family(fildes) == AF_UNIX)
            {
                explain_buffer_einval_too_small
                (
                    sb,
                    "sock_addr_size",
                    sock_addr_size
                );
            }
            else
            {
                explain_buffer_eafnosupport
                (
                    sb,
                    fildes,
                    "fildes",
                    sock_addr,
                    "sock_addr"
                );
            }
        }
        else
        {
            the_socket_is_already_bound(sb, fildes);
        }
        break;

    case ENOTSOCK:
        explain_buffer_enotsock(sb, fildes, "fildes");
        break;

    case EADDRNOTAVAIL:
        /* FIXME: which is it? differentiate between these two cases */
        explain_string_buffer_puts
        (
            sb,
            /*
             * xgettext: This message is used to explain an EADDRNOTAVAIL
             * error reported by a bind(2) system call, in the case where
             * the requested network address was not local or a nonexistent
             * interface was requested.
             */
            i18n("the requested network address was not local or a "
            "nonexistent interface was requested")
        );

#ifdef SO_BINDTODEVICE
        {
            char            name[IFNAMSIZ + 1];
            socklen_t       namsiz;

            /*
             * This is only useful for AF_INET and AF_INET6,
             * AF_PACKET is different.
             */
            namsiz = sizeof(name);
            if
            (
                !getsockopt(fildes, SOL_SOCKET, SO_BINDTODEVICE, name, &namsiz)
            &&
                namsiz
            &&
                name[0]
            )
            {
                explain_string_buffer_puts(sb, " (");
                explain_string_buffer_puts_quoted(sb, name);
                explain_string_buffer_putc(sb, ')');
            }
        }
#endif
        break;

    case EFAULT:
        explain_buffer_efault(sb, "sock_addr");
        break;

    case ELOOP:
        if (sock_addr->sa_family == AF_UNIX)
        {
            struct sockaddr_un *saddr;
            explain_final_t final_component;

            explain_final_sock_init(&final_component);
            saddr = (struct sockaddr_un *)sock_addr;

            explain_buffer_eloop
            (
                sb,
                saddr->sun_path,
                "sock_addr->sun_path",
                &final_component
            );
        }
        break;

    case ENAMETOOLONG:
        if (sock_addr->sa_family == AF_UNIX)
        {
            struct sockaddr_un *saddr;
            explain_final_t final_component;

            explain_final_sock_init(&final_component);
            saddr = (struct sockaddr_un *)sock_addr;

            explain_buffer_enametoolong
            (
                sb,
                saddr->sun_path,
                "sock_addr->sun_path",
                &final_component
            );
        }
        break;

    case ENOENT:
        if (sock_addr->sa_family == AF_UNIX)
        {
            struct sockaddr_un *saddr;
            explain_final_t final_component;

            explain_final_sock_init(&final_component);
            saddr = (struct sockaddr_un *)sock_addr;

            explain_buffer_enoent
            (
                sb,
                saddr->sun_path,
                "sock_addr->sun_path",
                &final_component
            );
        }
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOTDIR:
        if (sock_addr->sa_family == AF_UNIX)
        {
            struct sockaddr_un *saddr;
            explain_final_t final_component;

            explain_final_sock_init(&final_component);
            saddr = (struct sockaddr_un *)sock_addr;

            explain_buffer_enotdir
            (
                sb,
                saddr->sun_path,
                "sock_addr->sun_path",
                &final_component
            );
        }
        break;

    case EROFS:
        if (sock_addr->sa_family == AF_UNIX)
        {
            struct sockaddr_un *saddr;

            saddr = (struct sockaddr_un *)sock_addr;
            explain_buffer_erofs(sb, saddr->sun_path, "sock_addr->sun_path");
        }
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "bind");
        break;
    }
}


void
explain_buffer_errno_bind(explain_string_buffer_t *sb, int errnum,
    int fildes, const struct sockaddr *sock_addr, int sock_addr_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_bind_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        sock_addr,
        sock_addr_size
    );
    explain_buffer_errno_bind_explanation
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
