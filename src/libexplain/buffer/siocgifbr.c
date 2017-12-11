/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/ac/linux/if_bridge.h>

#include <libexplain/buffer/siocgifbr.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_IF_BRIDGE_H

static void
explain_buffer_brctl(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "BRCTL_GET_VERSION", BRCTL_GET_VERSION },
        { "BRCTL_GET_BRIDGES", BRCTL_GET_BRIDGES },
        { "BRCTL_ADD_BRIDGE", BRCTL_ADD_BRIDGE },
        { "BRCTL_DEL_BRIDGE", BRCTL_DEL_BRIDGE },
        { "BRCTL_ADD_IF", BRCTL_ADD_IF },
        { "BRCTL_DEL_IF", BRCTL_DEL_IF },
        { "BRCTL_GET_BRIDGE_INFO", BRCTL_GET_BRIDGE_INFO },
        { "BRCTL_GET_PORT_LIST", BRCTL_GET_PORT_LIST },
        { "BRCTL_SET_BRIDGE_FORWARD_DELAY", BRCTL_SET_BRIDGE_FORWARD_DELAY },
        { "BRCTL_SET_BRIDGE_HELLO_TIME", BRCTL_SET_BRIDGE_HELLO_TIME },
        { "BRCTL_SET_BRIDGE_MAX_AGE", BRCTL_SET_BRIDGE_MAX_AGE },
        { "BRCTL_SET_AGEING_TIME", BRCTL_SET_AGEING_TIME },
        { "BRCTL_SET_GC_INTERVAL", BRCTL_SET_GC_INTERVAL },
        { "BRCTL_GET_PORT_INFO", BRCTL_GET_PORT_INFO },
        { "BRCTL_SET_BRIDGE_STP_STATE", BRCTL_SET_BRIDGE_STP_STATE },
        { "BRCTL_SET_BRIDGE_PRIORITY", BRCTL_SET_BRIDGE_PRIORITY },
        { "BRCTL_SET_PORT_PRIORITY", BRCTL_SET_PORT_PRIORITY },
        { "BRCTL_SET_PATH_COST", BRCTL_SET_PATH_COST },
        { "BRCTL_GET_FDB_ENTRIES", BRCTL_GET_FDB_ENTRIES },
    };

    explain_parse_bits_print_single(sb, data, table, SIZEOF(table));
}


void
explain_buffer_siocgifbr(explain_string_buffer_t *sb,
    const unsigned long data[3])
{
    explain_string_buffer_puts(sb, "{ ");
    explain_buffer_brctl(sb, data[0]);
    explain_string_buffer_printf(sb, ", %lu, %lu }", data[1], data[2]);
}

#else

void
explain_buffer_siocgifbr(explain_string_buffer_t *sb,
    const unsigned long data[3])
{
    explain_string_buffer_printf
    (
        sb,
        "{ %lu, %lu, %lu }",
        data[0],
        data[1],
        data[2]
    );
}

#endif


/* vim: set ts=8 sw=4 et : */
