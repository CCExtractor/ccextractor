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
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/socket.h>

#include <libexplain/buffer/sockopt_name.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static const explain_parse_bits_table_t sol_socket_table[] =
{
    { "SO_DEBUG", SO_DEBUG },
#ifdef SO_REUSEADDR
    { "SO_REUSEADDR", SO_REUSEADDR },
#endif
#ifdef SO_TYPE
    { "SO_TYPE", SO_TYPE },
#endif
#ifdef SO_ERROR
    { "SO_ERROR", SO_ERROR },
#endif
#ifdef SO_DONTROUTE
    { "SO_DONTROUTE", SO_DONTROUTE },
#endif
#ifdef SO_BROADCAST
    { "SO_BROADCAST", SO_BROADCAST },
#endif
#ifdef SO_SNDBUF
    { "SO_SNDBUF", SO_SNDBUF },
#endif
#ifdef SO_RCVBUF
    { "SO_RCVBUF", SO_RCVBUF },
#endif
#ifdef SO_SNDBUFFORCE
    { "SO_SNDBUFFORCE", SO_SNDBUFFORCE },
#endif
#ifdef SO_RCVBUFFORCE
    { "SO_RCVBUFFORCE", SO_RCVBUFFORCE },
#endif
#ifdef SO_KEEPALIVE
    { "SO_KEEPALIVE", SO_KEEPALIVE },
#endif
#ifdef SO_OOBINLINE
    { "SO_OOBINLINE", SO_OOBINLINE },
#endif
#ifdef SO_NO_CHECK
    { "SO_NO_CHECK", SO_NO_CHECK },
#endif
#ifdef SO_PRIORITY
    { "SO_PRIORITY", SO_PRIORITY },
#endif
#ifdef SO_LINGER
    { "SO_LINGER", SO_LINGER },
#endif
#ifdef SO_BSDCOMPAT
    { "SO_BSDCOMPAT", SO_BSDCOMPAT },
#endif
#ifdef SO_PASSCRED
    { "SO_PASSCRED", SO_PASSCRED },
#endif
#ifdef SO_PEERCRED
    { "SO_PEERCRED", SO_PEERCRED },
#endif
#ifdef SO_RCVLOWAT
    { "SO_RCVLOWAT", SO_RCVLOWAT },
#endif
#ifdef SO_SNDLOWAT
    { "SO_SNDLOWAT", SO_SNDLOWAT },
#endif
#ifdef SO_RCVTIMEO
    { "SO_RCVTIMEO", SO_RCVTIMEO },
#endif
#ifdef SO_SNDTIMEO
    { "SO_SNDTIMEO", SO_SNDTIMEO },
#endif
#ifdef SO_SECURITY_AUTHENTICATION
    { "SO_SECURITY_AUTHENTICATION", SO_SECURITY_AUTHENTICATION },
#endif
#ifdef SO_SECURITY_ENCRYPTION_TRANSPORT
    { "SO_SECURITY_ENCRYPTION_TRANSPORT", SO_SECURITY_ENCRYPTION_TRANSPORT },
#endif
#ifdef SO_SECURITY_ENCRYPTION_NETWORK
    { "SO_SECURITY_ENCRYPTION_NETWORK", SO_SECURITY_ENCRYPTION_NETWORK },
#endif
#ifdef SO_BINDTODEVICE
    { "SO_BINDTODEVICE", SO_BINDTODEVICE },
#endif
#ifdef SO_ATTACH_FILTER
    { "SO_ATTACH_FILTER", SO_ATTACH_FILTER },
#endif
#ifdef SO_DETACH_FILTER
    { "SO_DETACH_FILTER", SO_DETACH_FILTER },
#endif
#ifdef SO_PEERNAME
    { "SO_PEERNAME", SO_PEERNAME },
#endif
#ifdef SO_TIMESTAMP
    { "SO_TIMESTAMP", SO_TIMESTAMP },
#endif
#ifdef SO_ACCEPTCONN
    { "SO_ACCEPTCONN", SO_ACCEPTCONN },
#endif
#ifdef SO_PEERSEC
    { "SO_PEERSEC", SO_PEERSEC },
#endif
#ifdef SO_PASSSEC
    { "SO_PASSSEC", SO_PASSSEC },
#endif
#ifdef SO_TIMESTAMPNS
    { "SO_TIMESTAMPNS", SO_TIMESTAMPNS },
#endif
#ifdef SCM_TIMESTAMPNS
    { "SCM_TIMESTAMPNS", SCM_TIMESTAMPNS },
#endif
#ifdef SO_MARK
    { "SO_MARK", SO_MARK },
#endif

    /* <linux/atm.h> */
#ifdef SO_SETCLP
    { "SO_SETCLP", SO_SETCLP },
#endif
#ifdef SO_CIRANGE
    { "SO_CIRANGE", SO_CIRANGE },
#endif
#ifdef SO_ATMQOS
    { "SO_ATMQOS", SO_ATMQOS },
#endif
#ifdef SO_ATMSAP
    { "SO_ATMSAP", SO_ATMSAP },
#endif
#ifdef SO_ATMPVC
    { "SO_ATMPVC", SO_ATMPVC },
#endif
#ifdef SO_MULTIPOINT
    { "SO_MULTIPOINT", SO_MULTIPOINT },
#endif
};

static const explain_parse_bits_table_t sol_ip_table[] =
{
    { "IP_TOS", IP_TOS },
#ifdef IP_TTL
    { "IP_TTL", IP_TTL },
#endif
#ifdef IP_HDRINCL
    { "IP_HDRINCL", IP_HDRINCL },
#endif
#ifdef IP_OPTIONS
    { "IP_OPTIONS", IP_OPTIONS },
#endif
#ifdef IP_ROUTER_ALERT
    { "IP_ROUTER_ALERT", IP_ROUTER_ALERT },
#endif
#ifdef IP_RECVOPTS
    { "IP_RECVOPTS", IP_RECVOPTS },
#endif
#ifdef IP_RETOPTS
    { "IP_RETOPTS", IP_RETOPTS },
#endif
#ifdef IP_PKTINFO
    { "IP_PKTINFO", IP_PKTINFO },
#endif
#ifdef IP_PKTOPTIONS
    { "IP_PKTOPTIONS", IP_PKTOPTIONS },
#endif
#ifdef IP_PMTUDISC
    { "IP_PMTUDISC", IP_PMTUDISC },
#endif
#ifdef IP_MTU_DISCOVER
    { "IP_MTU_DISCOVER", IP_MTU_DISCOVER },
#endif
#ifdef IP_RECVERR
    { "IP_RECVERR", IP_RECVERR },
#endif
#ifdef IP_RECVTTL
    { "IP_RECVTTL", IP_RECVTTL },
#endif
#ifdef IP_RECVTOS
    { "IP_RECVTOS", IP_RECVTOS },
#endif
#ifdef IP_MULTICAST_IF
    { "IP_MULTICAST_IF", IP_MULTICAST_IF },
#endif
#ifdef IP_MULTICAST_TTL
    { "IP_MULTICAST_TTL", IP_MULTICAST_TTL },
#endif
#ifdef IP_MULTICAST_LOOP
    { "IP_MULTICAST_LOOP", IP_MULTICAST_LOOP },
#endif
#ifdef IP_ADD_MEMBERSHIP
    { "IP_ADD_MEMBERSHIP", IP_ADD_MEMBERSHIP },
#endif
#ifdef IP_DROP_MEMBERSHIP
    { "IP_DROP_MEMBERSHIP", IP_DROP_MEMBERSHIP },
#endif
#ifdef IP_UNBLOCK_SOURCE
    { "IP_UNBLOCK_SOURCE", IP_UNBLOCK_SOURCE },
#endif
#ifdef IP_BLOCK_SOURCE
    { "IP_BLOCK_SOURCE", IP_BLOCK_SOURCE },
#endif
#ifdef IP_ADD_SOURCE_MEMBERSHIP
    { "IP_ADD_SOURCE_MEMBERSHIP", IP_ADD_SOURCE_MEMBERSHIP },
#endif
#ifdef IP_DROP_SOURCE_MEMBERSHIP
    { "IP_DROP_SOURCE_MEMBERSHIP", IP_DROP_SOURCE_MEMBERSHIP },
#endif
#ifdef IP_MSFILTER
    { "IP_MSFILTER", IP_MSFILTER },
#endif
#ifdef MCAST_JOIN_GROUP
    { "MCAST_JOIN_GROUP", MCAST_JOIN_GROUP },
#endif
#ifdef MCAST_BLOCK_SOURCE
    { "MCAST_BLOCK_SOURCE", MCAST_BLOCK_SOURCE },
#endif
#ifdef MCAST_UNBLOCK_SOURCE
    { "MCAST_UNBLOCK_SOURCE", MCAST_UNBLOCK_SOURCE },
#endif
#ifdef MCAST_LEAVE_GROUP
    { "MCAST_LEAVE_GROUP", MCAST_LEAVE_GROUP },
#endif
#ifdef MCAST_JOIN_SOURCE_GROUP
    { "MCAST_JOIN_SOURCE_GROUP", MCAST_JOIN_SOURCE_GROUP },
#endif
#ifdef MCAST_LEAVE_SOURCE_GROUP
    { "MCAST_LEAVE_SOURCE_GROUP", MCAST_LEAVE_SOURCE_GROUP },
#endif
#ifdef MCAST_MSFILTER
    { "MCAST_MSFILTER", MCAST_MSFILTER },
#endif
};


void
explain_buffer_sockopt_name(explain_string_buffer_t *sb, int level,
    int name)
{
    switch (level)
    {
    /* FIXME: add support for SOL_AAL */
    /* FIXME: add support for SOL_ATALK */
    /* FIXME: add support for SOL_ATM */
    /* FIXME: add support for SOL_AX25 */
    /* FIXME: add support for SOL_BLUETOOTH */
    /* FIXME: add support for SOL_DCCP */
    /* FIXME: add support for SOL_DECNET */
    /* FIXME: add support for SOL_ICMP */
    /* FIXME: add support for SOL_ICMPV6 */

#ifdef SOL_IP
    case SOL_IP:
        explain_parse_bits_print_single
        (
            sb,
            name,
            sol_ip_table,
            SIZEOF(sol_ip_table)
        );
        break;
#endif

    /* FIXME: add support for SOL_IPV6 */
    /* FIXME: add support for SOL_IPX */
    /* FIXME: add support for SOL_IRDA */
    /* FIXME: add support for SOL_LLC */
    /* FIXME: add support for SOL_NETBUEI */
    /* FIXME: add support for SOL_NETLINK */
    /* FIXME: add support for SOL_NETROM */
    /* FIXME: add support for SOL_PACKET */
    /* FIXME: add support for SOL_PPPOL2TP */
    /* FIXME: add support for SOL_RAW */
    /* FIXME: add support for SOL_ROSE */
    /* FIXME: add support for SOL_RXRPC */
    /* FIXME: add support for SOL_SCTP */

    case SOL_SOCKET:
        explain_parse_bits_print_single
        (
            sb,
            name,
            sol_socket_table,
            SIZEOF(sol_socket_table)
        );
        break;

    /* FIXME: add support for SOL_TCP */
    /* FIXME: add support for SOL_TIPC */
    /* FIXME: add support for SOL_UDP */
    /* FIXME: add support for SOL_UDPLITE */
    /* FIXME: add support for SOL_X25 */

    default:
        explain_string_buffer_printf(sb, "%d", name);
        break;
    }
}


typedef struct list_t list_t;
struct list_t
{
    explain_parse_bits_table_t *item;
    size_t          size;
    size_t          max;
};


static void
list_constructor(list_t *lp)
{
    lp->item = 0;
    lp->size = 0;
    lp->max = 0;
}


static void
list_append(list_t *lp, const explain_parse_bits_table_t *table, size_t size)
{
    if (lp->size + size > lp->max)
    {
        size_t          new_max;
        explain_parse_bits_table_t *new_item;

        new_max = lp->max * 2 + 4;
        while (lp->size + size > new_max)
            new_max = new_max * 2 + 4;
        new_item = malloc(sizeof(*new_item) * new_max);
        if (!new_item)
            return;
        memcpy(new_item, lp->item, lp->size * sizeof(*new_item));
        if (lp->item)
            free(lp->item);
        lp->item = new_item;
        lp->max = new_max;
    }
    memcpy(lp->item + lp->size, table, size * sizeof(*table));
    lp->size += size;
}


static void
list_destructor(list_t *lp)
{
    if (lp->item)
        free(lp->item);
    lp->item = 0;
    lp->size = 0;
    lp->max = 0;
}


int
explain_parse_sockopt_name_or_die(const char *text, const char *caption)
{
    list_t          list;
    int             result;

    list_constructor(&list);
    list_append(&list, sol_ip_table, SIZEOF(sol_ip_table));
    list_append(&list, sol_socket_table, SIZEOF(sol_socket_table));
    if (!list.item)
    {
        return
            explain_parse_bits_or_die
            (
                text,
                sol_socket_table,
                SIZEOF(sol_socket_table),
                caption
            );
    }
    result = explain_parse_bits_or_die(text, list.item, list.size, caption);
    list_destructor(&list);
    return result;
}


/* vim: set ts=8 sw=4 et : */
