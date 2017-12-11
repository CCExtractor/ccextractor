/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/address_family.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/errno/socket.h>
#include <libexplain/buffer/errno/socketpair.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/socket_protocol.h>
#include <libexplain/buffer/socket_type.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_socketpair_system_call(explain_string_buffer_t *sb, int
    errnum, int domain, int type, int protocol, int *sv)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "socketpair(domain = ");
    explain_buffer_address_family(sb, domain);
    explain_string_buffer_puts(sb, ", type = ");
    explain_buffer_socket_type(sb, type);
    explain_string_buffer_puts(sb, ", protocol = ");
    explain_buffer_socket_protocol(sb, protocol);
    explain_string_buffer_puts(sb, ", sv = ");
    explain_buffer_pointer(sb, sv);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_socketpair_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int domain, int type, int protocol,
    int *sv)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/socketpair.html
     */
    (void)sv;
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "sv");
        break;

    default:
        explain_buffer_errno_socket_explanation
        (
            sb,
            errnum,
            syscall_name,
            domain,
            type,
            protocol
        );
        break;
    }
}


void
explain_buffer_errno_socketpair(explain_string_buffer_t *sb, int errnum, int
    domain, int type, int protocol, int *sv)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_socketpair_system_call
    (
        &exp.system_call_sb,
        errnum,
        domain,
        type,
        protocol,
        sv
    );
    explain_buffer_errno_socketpair_explanation
    (
        &exp.explanation_sb,
        errnum,
        "socketpair",
        domain,
        type,
        protocol,
        sv
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
