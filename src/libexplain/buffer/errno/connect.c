/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
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
#include <libexplain/ac/sys/socket.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/sys/un.h>

#include <libexplain/buffer/address_family.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/eaddrinuse.h>
#include <libexplain/buffer/eafnosupport.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/enotsock.h>
#include <libexplain/buffer/errno/connect.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/software_error.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_connect_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, const struct sockaddr *serv_addr,
    int serv_addr_size)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "connect(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", serv_addr = ");
    explain_buffer_sockaddr(sb, serv_addr, serv_addr_size);
    explain_string_buffer_printf
    (
        sb,
        ", serv_addr_size = %d)",
        serv_addr_size
    );
}


static void
explain_buffer_errno_connect_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes, const struct sockaddr *serv_addr,
    int serv_addr_size)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/connect.html
     */
    (void)serv_addr_size;
    switch (errnum)
    {
    case EACCES:
        if (serv_addr->sa_family == AF_UNIX)
        {
            const struct sockaddr_un *saddr;
            explain_final_t final_component;

            saddr = (const struct sockaddr_un *)serv_addr;
            explain_final_init(&final_component);
            final_component.want_to_write = 1;
            final_component.must_be_a_st_mode = 1;
            final_component.st_mode = S_IFSOCK;
            final_component.path_max = sizeof(saddr->sun_path) - 1;
            explain_buffer_eacces
            (
                sb,
                saddr->sun_path,
                "sock_addr",
                &final_component
            );
            break;
        }
        /* fall through... */

    case EPERM:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an EPERM error
             * reported by the connect(2) system call, in the case where
             * the process tried to connect to a broadcast address
             * without having the socket broadcast flag enabled; or, the
             * connection request failed because of a local firewall
             * rule
             */
            i18n("the process tried to connect to a broadcast address "
            "without having the socket broadcast flag enabled; or, the "
            "connection request failed because of a local firewall "
            "rule")
        );
        break;

    case EADDRINUSE:
        explain_buffer_eaddrinuse(sb, fildes);
        break;

    case EAFNOSUPPORT:
        explain_buffer_eafnosupport
        (
            sb,
            fildes,
            "fildes",
            serv_addr,
            "serv_addr"
        );
        break;

    case EAGAIN:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an EAGAIN error
             * reported by the connect(2) system call, in the case where
             * no more free local ports or insufficient entries in the
             * routing cache.
             */
            i18n("no more free local ports or insufficient entries in the "
            "routing cache")
        );
#ifdef __linux__
        if
        (
            serv_addr->sa_family == PF_INET
        &&
            explain_option_dialect_specific()
        )
        {
            explain_string_buffer_puts(sb->footnotes, "; ");
            explain_buffer_gettext
            (
                sb->footnotes,
                /*
                 * xgettext: This message is used to supplement an
                 * EAGAIN explanation for the connect(2) system call, on
                 * Linux the number of local ports can be increased.
                 */
                i18n("see the net.ipv4.ip_local_port_range sysctl in ip(7) "
                "for how to increase the number of local ports")
            );
        }
#endif
        break;

    case EALREADY:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an EALREADY
             * error reported by the connect(2) system call, in the
             * case where the socket is non-blocking and a previous
             * connection attempt has not yet been completed.
             */
            i18n("the socket is non-blocking and a previous connection "
            "attempt has not yet been completed")
        );
        explain_buffer_software_error(sb);
        break;

    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case ECONNREFUSED:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an ECONNREFUSED
             * error reported by the connect(2) system call, in the
             * case where the remote server is accessible but is not
             * listening for connections to the given port; or, an
             * intervening firewall refused the connection.
             */
            i18n("the remote server is accessible but is not listening for "
            "connections to the given port; or, an intervening firewall "
            "refused the connection")
        );
        break;

    case EFAULT:
        explain_buffer_efault(sb, "sock_addr");
        break;

    case EINPROGRESS:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an EINPROGRESS
             * error reported by the connect(2) system call, in the
             * case where the socket is non-blocking and the connection
             * cannot be completed immediately.
             */
            i18n("the socket is non-blocking and the connection cannot "
            "be completed immediately")
        );
        explain_buffer_software_error(sb);
        break;

    case EINTR:
        explain_buffer_eintr(sb, "connect");
        break;

    case EISCONN:
        {
            struct sockaddr_storage sas;
            struct sockaddr *sa;
            socklen_t       siz;

            sa = (struct sockaddr *)&sas;
            siz = sizeof(sas);
            if (getsockname(fildes, sa, &siz) >= 0)
            {
                char            addr[500];
                explain_string_buffer_t addr_sb;

                explain_string_buffer_init(&addr_sb, addr, sizeof(addr));
                explain_buffer_sockaddr(&addr_sb, sa, siz);
                explain_string_buffer_printf_gettext
                (
                    sb,
                    /*
                     * xgettext: This message is used to explain an EISCONN
                     * error reported by the connect(2) system call, in the case
                     * where the socket is already connected.
                     *
                     * %1$s => The network address to which it is connected
                     */
                    i18n("the socket is already connected to %s"),
                    addr
                );
            }
            else
            {
                explain_buffer_gettext
                (
                    sb,
                    /*
                     * xgettext: This message is used to explain an EISCONN
                     * error reported by the connect(2) system call, in the case
                     * where the socket is already connected, but the exact
                     * address cannot be determined.
                     */
                    i18n("the socket is already connected to a network address")
                );
            }
            explain_buffer_software_error(sb);
        }
        break;

    case ENETUNREACH:
        /* FIXME: diagnose more routing table cases */
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an ENETUNREACH
             * error reported by the connect(2) system call, in the case
             * where network or host is unreachable.
             */
            i18n("network or host is unreachable; sometimes this is a "
            "routing issue, sometimes the network is physically "
            "disconnected, sometimes a router is turned off, sometimes "
            "the host is physically disconnected, sometimes the host is "
            "turned off")
        );
        break;

    case ENOTSOCK:
        explain_buffer_enotsock(sb, fildes, "fildes");
        break;

    case ETIMEDOUT:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an ETIMEDOUT
             * error reported by the connect(2) system call, in the case
             * where the connection attempt took to long.
             */
            i18n("the connection attempt took to long; the server may be too "
            "busy to accept new connections, or an intervening firewall "
            "may be discarding your packets")
        );
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "connect");
        break;
    }
}


void
explain_buffer_errno_connect(explain_string_buffer_t *sb, int errnum,
    int fildes, const struct sockaddr *serv_addr, int serv_addr_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_connect_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        serv_addr,
        serv_addr_size
    );
    explain_buffer_errno_connect_explanation
    (
        &exp.explanation_sb,
        errnum,
        fildes,
        serv_addr,
        serv_addr_size
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
