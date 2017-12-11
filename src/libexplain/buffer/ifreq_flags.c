/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#include <libexplain/ac/net/if.h>

#include <libexplain/buffer/ifreq_flags.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>

static void
explain_buffer_ifflags(explain_string_buffer_t *sb, int flags)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "IFF_UP", IFF_UP },
        { "IFF_BROADCAST", IFF_BROADCAST },
        { "IFF_DEBUG", IFF_DEBUG },
        { "IFF_LOOPBACK", IFF_LOOPBACK },
        { "IFF_POINTOPOINT", IFF_POINTOPOINT },
#ifdef IFF_NOTRAILERS
        { "IFF_NOTRAILERS", IFF_NOTRAILERS },
#endif
        { "IFF_RUNNING", IFF_RUNNING },
        { "IFF_NOARP", IFF_NOARP },
        { "IFF_PROMISC", IFF_PROMISC },
        { "IFF_ALLMULTI", IFF_ALLMULTI },
#ifdef IFF_MASTER
        { "IFF_MASTER", IFF_MASTER },
#endif
#ifdef IFF_SLAVE
        { "IFF_SLAVE", IFF_SLAVE },
#endif
        { "IFF_MULTICAST", IFF_MULTICAST },
#ifdef IFF_PORTSEL
        { "IFF_PORTSEL", IFF_PORTSEL },
#endif
#ifdef IFF_AUTOMEDIA
        { "IFF_AUTOMEDIA", IFF_AUTOMEDIA },
#endif
#ifdef IFF_DYNAMIC
        { "IFF_DYNAMIC", IFF_DYNAMIC },
#endif
    };

    explain_parse_bits_print(sb, flags, table, SIZEOF(table));
}


void
explain_buffer_ifreq_flags(explain_string_buffer_t *sb,
    const struct ifreq *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        const struct ifreq *ifr;

        /*
         * This is actually a huge big sucky union.  This specific
         * case is given the interface name and the interface flags.
         */
        ifr = data;
        explain_string_buffer_puts(sb, "{ ifr_name = ");
        explain_string_buffer_puts_quoted_n
        (
            sb,
            ifr->ifr_name,
            sizeof(ifr->ifr_name)
        );
        explain_string_buffer_puts(sb, ", ifr_flags = ");
        explain_buffer_ifflags(sb, ifr->ifr_flags);
        explain_string_buffer_puts(sb, " }");
    }
}


/* vim: set ts=8 sw=4 et : */
