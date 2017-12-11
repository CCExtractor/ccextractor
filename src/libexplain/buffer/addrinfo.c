/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/netdb.h>

#include <libexplain/buffer/address_family.h>
#include <libexplain/buffer/addrinfo.h>
#include <libexplain/buffer/addrinfo_flags.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/buffer/socket_protocol.h>
#include <libexplain/buffer/socket_type.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


void
explain_buffer_addrinfo(explain_string_buffer_t *sb,
    const struct addrinfo *ai)
{
    if (explain_is_efault_pointer(ai, sizeof(*ai)))
    {
        explain_buffer_pointer(sb, ai);
        return;
    }
    explain_string_buffer_puts(sb, "{ ai_flags = ");
    explain_buffer_addrinfo_flags(sb, ai->ai_flags);
    explain_string_buffer_puts(sb, ", ai_family = ");
    explain_buffer_address_family(sb, ai->ai_family);
    explain_string_buffer_puts(sb, ", ai_socktype = ");
    explain_buffer_socket_type(sb, ai->ai_socktype);
    explain_string_buffer_puts(sb, ", ai_protocol = ");
    explain_buffer_socket_protocol(sb, ai->ai_protocol);
    explain_string_buffer_puts(sb, ", ai_addr = ");
    explain_buffer_sockaddr(sb, ai->ai_addr, ai->ai_addrlen);
    if (ai->ai_flags & AI_CANONNAME)
    {
        explain_string_buffer_puts(sb, "ai_canonname = ");
        explain_buffer_pathname(sb, ai->ai_canonname);
    }
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
