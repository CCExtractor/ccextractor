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

#include <libexplain/buffer/ifreq_ifmap.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef ifr_ifmap

static void
explain_buffer_ifmap(explain_string_buffer_t *sb,
    const struct ifmap *data)
{
    explain_string_buffer_printf
    (
        sb,
        "{ mem_start = %#lx, "
            "mem_end = %#lx, "
            "base_addr = %#x, "
            "irq = %u, "
            "dma = %u, "
            "port = %u }",
        data->mem_start,
        data->mem_end,
        data->base_addr,
        data->irq,
        data->dma,
        data->port
    );
}


void
explain_buffer_ifreq_ifmap(explain_string_buffer_t *sb,
    const struct ifreq *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        const struct ifreq *ifr;

        /*
         * This is actually a huge big sucky union.  This specific case
         * is given the interface name and the ifr_map member.
         */
        ifr = data;
        explain_string_buffer_puts(sb, "{ ifr_name = ");
        explain_string_buffer_puts_quoted_n
        (
            sb,
            ifr->ifr_name,
            sizeof(ifr->ifr_name)
        );
        explain_string_buffer_puts(sb, ", ifr_map = ");
        explain_buffer_ifmap(sb, &ifr->ifr_map);
        explain_string_buffer_puts(sb, " }");
    }
}

#else

void
explain_buffer_ifreq_ifmap(explain_string_buffer_t *sb,
    const struct ifreq *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
