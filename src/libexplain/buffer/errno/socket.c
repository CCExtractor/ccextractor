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
#include <libexplain/ac/netdb.h>

#include <libexplain/buffer/address_family.h>
#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/emfile.h>
#include <libexplain/buffer/enfile.h>
#include <libexplain/buffer/enobufs.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/socket.h>
#include <libexplain/buffer/socket_protocol.h>
#include <libexplain/buffer/socket_type.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_socket_system_call(explain_string_buffer_t *sb,
    int errnum, int family, int type, int protocol)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "socket(family = ");
    explain_buffer_address_family(sb, family);
    explain_string_buffer_puts(sb, ", type = ");
    explain_buffer_socket_type(sb, type);
    explain_string_buffer_puts(sb, ", protocol = ");
    explain_buffer_socket_protocol(sb, protocol);
    explain_string_buffer_putc(sb, ')');

    /*
     * Afterwards,
     *     getsockname(2);sa.sa_family
     *         can be used to retrieve the 'family'
     *     getsockopt(fildes, SOL_SOCKET, SO_TYPE, ...)
     *         can be used to retrieve the 'type'
     *     lsof(1) ... or the techniques it uses, on Linux
     *         can be used to retrieve the 'protocol'
     */
}


void
explain_buffer_errno_socket_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int family, int type, int protocol)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/socket.html
     *
     * Domain     Types...                                  See Also
     * AF_UNIX    SOCK_STREAM, SOCK_DGRAM, SOCK_SEQPACKET   unix(7)
     * AF_INET                                              ip(7)
     * AF_NETLINK                                           netlink(7)
     * AF_X25     SOCK_SEQPACKET                            x25(7)
     * AF_AX25                                              ax25(4)
     * AF_APPLETALK                                         ddp(7), atalk(4)
     * AF_PACKET                                            packet(7)
     * AF_NETROM                                            netrom(4)
     * AF_ROSE                                              rose(4)
     * AF_RAW                                               raw(7)
     */
    (void)family;
    (void)type;
    (void)protocol;
    switch (errnum)
    {
    case EACCES:
    case EPERM:
        explain_string_buffer_puts
        (
            sb,
            "the process does not have permission to create a socket of "
            "the specified type and/or protocol"
        );
#if defined(SOCK_RAW) || defined(SOCK_PACKET)
        switch (type)
        {
#ifdef SOCK_RAW
        case SOCK_RAW:
#endif
#ifdef SOCK_PACKET
        case SOCK_PACKET:
#endif
            explain_buffer_dac_net_raw(sb);
            break;

        default:
            break;
        }
#endif
        break;

    case EAFNOSUPPORT:
        explain_string_buffer_puts
        (
            sb,
            "the operating system does not support the specified "
            "address family"
        );
        break;

    case EINVAL:
        explain_string_buffer_puts
        (
            sb,
            "unknown protocol, or protocol family not available"
        );
        break;

    case EMFILE:
        explain_buffer_emfile(sb);
        break;

    case ENFILE:
        explain_buffer_enfile(sb);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOBUFS:
        explain_buffer_enobufs(sb);
        break;

    case EPROTONOSUPPORT:
        explain_string_buffer_puts
        (
            sb,
            "the protocol type or the specified protocol is not "
            "supported within this address family"
        );
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_socket(explain_string_buffer_t *sb, int errnum,
    int family, int type, int protocol)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_socket_system_call
    (
        &exp.system_call_sb,
        errnum,
        family,
        type,
        protocol
    );
    explain_buffer_errno_socket_explanation
    (
        &exp.explanation_sb,
        errnum,
        "socket",
        family,
        type,
        protocol
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
