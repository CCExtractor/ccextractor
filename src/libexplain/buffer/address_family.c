/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/sys/socket.h>

#include <libexplain/buffer/address_family.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef PF_UNSPEC
    { "PF_UNSPEC", PF_UNSPEC },
#endif
#ifdef PF_UNIX
    { "PF_UNIX", PF_UNIX },
#endif
#ifdef PF_LOCAL
    { "PF_LOCAL", PF_LOCAL },
#endif
#ifdef PF_FILE
    { "PF_FILE", PF_FILE },
#endif
#ifdef PF_INET
    { "PF_INET", PF_INET },
#endif
#ifdef PF_AX25
    { "PF_AX25", PF_AX25 },
#endif
#ifdef PF_IPX
    { "PF_IPX", PF_IPX },
#endif
#ifdef PF_APPLETALK
    { "PF_APPLETALK", PF_APPLETALK },
#endif
#ifdef PF_NETROM
    { "PF_NETROM", PF_NETROM },
#endif
#ifdef PF_BRIDGE
    { "PF_BRIDGE", PF_BRIDGE },
#endif
#ifdef PF_ATMPVC
    { "PF_ATMPVC", PF_ATMPVC },
#endif
#ifdef PF_X25
    { "PF_X25", PF_X25 },
#endif
#ifdef PF_INET6
    { "PF_INET6", PF_INET6 },
#endif
#ifdef PF_ROSE
    { "PF_ROSE", PF_ROSE },
#endif
#ifdef PF_DECnet
    { "PF_DECnet", PF_DECnet },
#endif
#ifdef PF_NETBEUI
    { "PF_NETBEUI", PF_NETBEUI },
#endif
#ifdef PF_SECURITY
    { "PF_SECURITY", PF_SECURITY },
#endif
#ifdef PF_KEY
    { "PF_KEY", PF_KEY },
#endif
#ifdef PF_NETLINK
    { "PF_NETLINK", PF_NETLINK },
#endif
#ifdef PF_ROUTE
    { "PF_ROUTE", PF_ROUTE },
#endif
#ifdef PF_PACKET
    { "PF_PACKET", PF_PACKET },
#endif
#ifdef PF_ASH
    { "PF_ASH", PF_ASH },
#endif
#ifdef PF_ECONET
    { "PF_ECONET", PF_ECONET },
#endif
#ifdef PF_ATMSVC
    { "PF_ATMSVC", PF_ATMSVC },
#endif
#ifdef PF_SNA
    { "PF_SNA", PF_SNA },
#endif
#ifdef PF_IRDA
    { "PF_IRDA", PF_IRDA },
#endif
#ifdef PF_PPPOX
    { "PF_PPPOX", PF_PPPOX },
#endif
#ifdef PF_WANPIPE
    { "PF_WANPIPE", PF_WANPIPE },
#endif
#ifdef PF_BLUETOOTH
    { "PF_BLUETOOTH", PF_BLUETOOTH },
#endif
#ifdef PF_IUCV
    { "PF_IUCV", PF_IUCV },
#endif
#ifdef PF_RXRPC
    { "PF_RXRPC", PF_RXRPC },
#endif
#ifdef AF_UNSPEC
    { "AF_UNSPEC", AF_UNSPEC },
#endif
#ifdef AF_LOCAL
    { "AF_LOCAL", AF_LOCAL },
#endif
#ifdef AF_UNIX
    { "AF_UNIX", AF_UNIX },
#endif
#ifdef AF_FILE
    { "AF_FILE", AF_FILE },
#endif
#ifdef AF_INET
    { "AF_INET", AF_INET },
#endif
#ifdef AF_AX25
    { "AF_AX25", AF_AX25 },
#endif
#ifdef AF_IPX
    { "AF_IPX", AF_IPX },
#endif
#ifdef AF_APPLETALK
    { "AF_APPLETALK", AF_APPLETALK },
#endif
#ifdef AF_NETROM
    { "AF_NETROM", AF_NETROM },
#endif
#ifdef AF_BRIDGE
    { "AF_BRIDGE", AF_BRIDGE },
#endif
#ifdef AF_ATMPVC
    { "AF_ATMPVC", AF_ATMPVC },
#endif
#ifdef AF_X25
    { "AF_X25", AF_X25 },
#endif
#ifdef AF_INET6
    { "AF_INET6", AF_INET6 },
#endif
#ifdef AF_ROSE
    { "AF_ROSE", AF_ROSE },
#endif
#ifdef AF_DECnet
    { "AF_DECnet", AF_DECnet },
#endif
#ifdef AF_NETBEUI
    { "AF_NETBEUI", AF_NETBEUI },
#endif
#ifdef AF_SECURITY
    { "AF_SECURITY", AF_SECURITY },
#endif
#ifdef AF_KEY
    { "AF_KEY", AF_KEY },
#endif
#ifdef AF_NETLINK
    { "AF_NETLINK", AF_NETLINK },
#endif
#ifdef AF_ROUTE
    { "AF_ROUTE", AF_ROUTE },
#endif
#ifdef AF_PACKET
    { "AF_PACKET", AF_PACKET },
#endif
#ifdef AF_ASH
    { "AF_ASH", AF_ASH },
#endif
#ifdef AF_ECONET
    { "AF_ECONET", AF_ECONET },
#endif
#ifdef AF_ATMSVC
    { "AF_ATMSVC", AF_ATMSVC },
#endif
#ifdef AF_SNA
    { "AF_SNA", AF_SNA },
#endif
#ifdef AF_IRDA
    { "AF_IRDA", AF_IRDA },
#endif
#ifdef AF_PPPOX
    { "AF_PPPOX", AF_PPPOX },
#endif
#ifdef AF_WANPIPE
    { "AF_WANPIPE", AF_WANPIPE },
#endif
#ifdef AF_BLUETOOTH
    { "AF_BLUETOOTH", AF_BLUETOOTH },
#endif
#ifdef AF_IUCV
    { "AF_IUCV", AF_IUCV },
#endif
#ifdef AF_RXRPC
    { "AF_RXRPC", AF_RXRPC },
#endif
};


void
explain_buffer_address_family(explain_string_buffer_t *sb, int domain)
{
    const explain_parse_bits_table_t *tp;

    tp = explain_parse_bits_find_by_value(domain, table, SIZEOF(table));
    if (tp)
        explain_string_buffer_puts(sb, tp->name);
    else
        explain_string_buffer_printf(sb, "%d", domain);
}


int
explain_parse_address_family_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
