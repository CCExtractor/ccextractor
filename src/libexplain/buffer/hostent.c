/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013, 2014 Peter Miller
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
#include <libexplain/ac/sys/socket.h> /* for FreeBSD, for AF_INET */
#include <libexplain/ac/string.h>

#include <libexplain/buffer/address_family.h>
#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/hostent.h>
#include <libexplain/buffer/in6_addr.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/is_efault.h>


void
explain_buffer_hostent(explain_string_buffer_t *sb,
    const struct hostent *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ h_name = ");
    explain_string_buffer_puts_quoted(sb, data->h_name);
    if (data->h_aliases && data->h_aliases[0])
    {
        char            **sp;

        explain_string_buffer_puts(sb, ", h_aliases = { ");
        for (sp = data->h_aliases; *sp; ++sp)
        {
            if (sp != data->h_aliases)
                explain_string_buffer_puts(sb, ", ");
            explain_string_buffer_puts_quoted(sb, *sp);
        }
        explain_string_buffer_puts(sb, " }");
    }
    explain_string_buffer_puts(sb, ", h_addrtype = ");
    explain_buffer_address_family(sb, data->h_addrtype);
    if (data->h_length)
    {
        explain_string_buffer_puts(sb, ", h_length = ");
        explain_buffer_int(sb, data->h_length);
    }
    if (data->h_length > 0 && data->h_addr_list && data->h_addr_list[0])
    {
        char            **ap;

        explain_string_buffer_puts(sb, ", h_addr_list = { ");
        for (ap = data->h_addr_list; *ap; ++ap)
        {
            if (ap != data->h_addr_list)
                explain_string_buffer_puts(sb, ", ");
            switch (data->h_addrtype)
            {
            case AF_INET:
                explain_buffer_in_addr(sb, (const struct in_addr *)*ap);
                break;

            case AF_INET6:
                explain_buffer_in6_addr(sb, (const struct in6_addr *)*ap);
                break;

            default:
                /* This is documented as impossible.  Yeah, right. */
                explain_buffer_hexdump(sb, *ap, data->h_length);
                break;
            }
        }
        explain_string_buffer_puts(sb, " }");
    }
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
