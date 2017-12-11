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

#include <libexplain/ac/bluetooth/bluetooth.h>
#include <libexplain/ac/netinet/in.h>
#include <libexplain/ac/sys/socket.h>

/* This one doesn't play nice with others on 64-bit machines?!? */
#include <libexplain/ac/linux/irda.h>

#include <libexplain/buffer/sockopt_level.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static const explain_parse_bits_table_t table[] =
{
    /* <sys/socket.h> */
#ifdef SOL_RAW
    { "SOL_RAW", SOL_RAW },
#endif
#ifdef SOL_DECNET
    { "SOL_DECNET", SOL_DECNET },
#endif
#ifdef SOL_X25
    { "SOL_X25", SOL_X25 },
#endif
#ifdef SOL_PACKET
    { "SOL_PACKET", SOL_PACKET },
#endif
#ifdef SOL_ATM
    { "SOL_ATM", SOL_ATM },
#endif
#ifdef SOL_AAL
    { "SOL_AAL", SOL_AAL },
#endif
#ifdef SOL_IRDA
    { "SOL_IRDA", SOL_IRDA },
#endif
#ifdef SOL_IP
    { "SOL_IP", SOL_IP },
#endif
#ifdef SOL_TCP
    { "SOL_TCP", SOL_TCP },
#endif
#ifdef SOL_UDP
    { "SOL_UDP", SOL_UDP },
#endif
#ifdef SOL_IPV6
    { "SOL_IPV6", SOL_IPV6 },
#endif
#ifdef SOL_ICMPV6
    { "SOL_ICMPV6", SOL_ICMPV6 },
#endif
#ifdef SOL_SCTP
    { "SOL_SCTP", SOL_SCTP },
#endif
#ifdef SOL_UDPLITE
    { "SOL_UDPLITE", SOL_UDPLITE },
#endif
#ifdef SOL_RAW
    { "SOL_RAW", SOL_RAW },
#endif
#ifdef SOL_IPX
    { "SOL_IPX", SOL_IPX },
#endif
#ifdef SOL_AX25
    { "SOL_AX25", SOL_AX25 },
#endif
#ifdef SOL_ATALK
    { "SOL_ATALK", SOL_ATALK },
#endif
#ifdef SOL_NETROM
    { "SOL_NETROM", SOL_NETROM },
#endif
#ifdef SOL_ROSE
    { "SOL_ROSE", SOL_ROSE },
#endif
#ifdef SOL_DECNET
    { "SOL_DECNET", SOL_DECNET },
#endif
#ifdef SOL_X25
    { "SOL_X25", SOL_X25 },
#endif
#ifdef SOL_PACKET
    { "SOL_PACKET", SOL_PACKET },
#endif
#ifdef SOL_ATM
    { "SOL_ATM", SOL_ATM },
#endif
#ifdef SOL_AAL
    { "SOL_AAL", SOL_AAL },
#endif
#ifdef SOL_IRDA
    { "SOL_IRDA", SOL_IRDA },
#endif
#ifdef SOL_NETBEUI
    { "SOL_NETBEUI", SOL_NETBEUI },
#endif
#ifdef SOL_LLC
    { "SOL_LLC", SOL_LLC },
#endif
#ifdef SOL_DCCP
    { "SOL_DCCP", SOL_DCCP },
#endif
#ifdef SOL_NETLINK
    { "SOL_NETLINK", SOL_NETLINK },
#endif
#ifdef SOL_TIPC
    { "SOL_TIPC", SOL_TIPC },
#endif
#ifdef SOL_RXRPC
    { "SOL_RXRPC", SOL_RXRPC },
#endif
#ifdef SOL_PPPOL2TP
    { "SOL_PPPOL2TP", SOL_PPPOL2TP },
#endif
#ifdef SOL_BLUETOOTH
    { "SOL_BLUETOOTH", SOL_BLUETOOTH },
#endif
#ifdef SOL_SOCKET
    { "SOL_SOCKET", SOL_SOCKET },
#endif

    /* <linux/irda.h> */
#ifdef SOL_IRLMP
    { "SOL_IRLMP", SOL_IRLMP },
#endif
#ifdef SOL_IRTTP
    { "SOL_IRTTP", SOL_IRTTP },
#endif

    /* <bluetooth/bluetooth.h> */
#ifdef SOL_HCI
    { "SOL_HCI", SOL_HCI },
#endif
#ifdef SOL_L2CAP
    { "SOL_L2CAP", SOL_L2CAP },
#endif
#ifdef SOL_SCO
    { "SOL_SCO", SOL_SCO },
#endif
#ifdef SOL_RFCOMM
    { "SOL_RFCOMM", SOL_RFCOMM },
#endif
};


void
explain_buffer_sockopt_level(explain_string_buffer_t *sb, int data)
{
    explain_parse_bits_print_single(sb, data, table, SIZEOF(table));
}


int
explain_parse_sockopt_level_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
