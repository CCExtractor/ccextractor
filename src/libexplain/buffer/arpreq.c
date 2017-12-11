/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#include <libexplain/ac/net/if_arp.h>

#include <libexplain/buffer/arpreq.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


static void
explain_buffer_arpreq_flags(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef ATF_COM
        { "ATF_COM", ATF_COM },
#endif
#ifdef ATF_PERM
    { "ATF_PERM", ATF_PERM },
#endif
#ifdef ATF_PUBL
    { "ATF_PUBL", ATF_PUBL },
#endif
#ifdef ATF_USETRAILERS
    { "ATF_USETRAILERS", ATF_USETRAILERS },
#endif
#ifdef ATF_NETMASK
        { "ATF_NETMASK", ATF_NETMASK },
#endif
#ifdef ATF_DONTPUB
        { "ATF_DONTPUB", ATF_DONTPUB },
#endif
#ifdef ATF_MAGIC
        { "ATF_MAGIC", ATF_MAGIC },
#endif
    };

    explain_parse_bits_print(sb, data, table, SIZEOF(table));
}


void
explain_buffer_arpreq(explain_string_buffer_t *sb,
    const struct arpreq *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        explain_string_buffer_puts(sb, "{ arp_pa = ");
        explain_buffer_sockaddr(sb, &data->arp_pa, sizeof(data->arp_pa));
        explain_string_buffer_puts(sb, ", arp_ha = ");
        explain_buffer_sockaddr(sb, &data->arp_ha, sizeof(data->arp_ha));
        explain_string_buffer_puts(sb, ", arp_flags = ");
        explain_buffer_arpreq_flags(sb, data->arp_flags);
#ifdef ATF_NETMASK
        explain_string_buffer_puts(sb, ", arp_netmask = ");
        explain_buffer_sockaddr
        (
            sb,
            &data->arp_netmask,
            sizeof(data->arp_netmask)
        );
#endif
        explain_string_buffer_puts(sb, " }");
    }
}


/* vim: set ts=8 sw=4 et : */
