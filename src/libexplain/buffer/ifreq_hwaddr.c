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

#include <libexplain/buffer/ifreq_hwaddr.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/is_efault.h>


void
explain_buffer_ifreq_hwaddr(explain_string_buffer_t *sb,
    const struct ifreq *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        const struct ifreq *ifr;

        /*
         * This is actually a huge big sucky union.  This specific case is given
         * the interface name and the interface hardware address.
         */
        ifr = data;
        explain_string_buffer_puts(sb, "{ ifr_name = ");
        explain_string_buffer_puts_quoted_n
        (
            sb,
            ifr->ifr_name,
            sizeof(ifr->ifr_name)
        );
#ifdef ifr_hwaddr
        explain_string_buffer_puts(sb, ", ifr_hwaddr = ");
        explain_buffer_sockaddr
        (
            sb,
            &ifr->ifr_hwaddr,
            sizeof(ifr->ifr_hwaddr)
        );
#endif
        explain_string_buffer_puts(sb, " }");
    }
}


/* vim: set ts=8 sw=4 et : */
