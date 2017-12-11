
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

#include <libexplain/ac/arpa/inet.h>
#include <libexplain/ac/linux/atalk.h>
#include <libexplain/ac/linux/atm.h>
#include <libexplain/ac/linux/if_pppox.h>
#include <libexplain/ac/linux/irda.h>
#include <libexplain/ac/linux/netlink.h>
#include <libexplain/ac/linux/x25.h>
#include <libexplain/ac/netash/ash.h>
#include <libexplain/ac/netdb.h>
#include <libexplain/ac/neteconet/ec.h>
#include <libexplain/ac/netinet/in.h>
#include <libexplain/ac/netipx/ipx.h>
#include <libexplain/ac/netiucv/iucv.h>
#include <libexplain/ac/netpacket/packet.h>
#include <libexplain/ac/netrose/rose.h>
#include <libexplain/ac/sys/socket.h>
#include <libexplain/ac/sys/un.h>

#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/in6_addr.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/buffer/address_family.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>


/*
 * See unix(7) for more information.
 */
static void
explain_buffer_sockaddr_af_unix(explain_string_buffer_t *sb,
    const struct sockaddr_un *sa, size_t sa_len)
{
    if (sa_len > sizeof(sa->sun_family))
    {
        explain_string_buffer_puts(sb, ", sun_path = ");
        explain_string_buffer_puts_quoted(sb, sa->sun_path);
    }
}


void
explain_buffer_in_addr(explain_string_buffer_t *sb,
    const struct in_addr *addr)
{
    explain_string_buffer_puts(sb, inet_ntoa(*addr));
    if (ntohl(addr->s_addr) == INADDR_ANY)
    {
        explain_string_buffer_puts(sb, " INADDR_ANY");
    }
    else if (ntohl(addr->s_addr) == INADDR_BROADCAST)
    {
        explain_string_buffer_puts(sb, " INADDR_BROADCAST");
    }
    else if (explain_option_dialect_specific())
    {
        struct hostent  *hep;

        /*
         * We make this dialect specific, because different systems will
         * have different entries in their /etc/hosts file, or there
         * could be transient DNS failures, and these could cause false
         * negatives for automated testing.
         */
        /* FIXME: gethostbyaddr_r if available */
        hep = gethostbyaddr(addr, sizeof(addr), AF_INET);
        if (hep)
        {
            explain_string_buffer_putc(sb, ' ');
            explain_string_buffer_puts_quoted(sb, hep->h_name);
        }
    }
}


/*
 * See ip(7) and inet(3) for more information.
 */
static void
explain_buffer_sockaddr_af_inet(explain_string_buffer_t *sb,
    const struct sockaddr_in *sa, size_t sa_len)
{
    unsigned short  port;

    /*
     * print the port number, and name if we can
     */
    (void)sa_len;
    port = ntohs(sa->sin_port);
    explain_string_buffer_printf(sb, ", sin_port = %u", port);
    if (explain_option_dialect_specific())
    {
        struct servent  *sep;

        /*
         * We make this dialect specific, because different systems will
         * have different entries in their /etc/services file, and this
         * could cause false negatives for automated testing.
         */
        /* FIXME: use getservbyport_r if available */
        sep = getservbyport(sa->sin_port, "tcp");
        if (!sep)
            sep = getservbyport(sa->sin_port, "udp");
        if (sep)
        {
            explain_string_buffer_putc(sb, ' ');
            explain_string_buffer_puts_quoted(sb, sep->s_name);
        }
    }

    /*
     * print the IP address, and name if we can
     */
    explain_string_buffer_puts(sb, ", sin_addr = ");
    explain_buffer_in_addr(sb, &sa->sin_addr);
}


#ifdef AF_AX25
#ifdef HAVE_LINUX_X25_H

static void
explain_buffer_sockaddr_af_ax25(explain_string_buffer_t *sb,
    const struct sockaddr_ax25 *sa, size_t sa_len)
{
    /* amateur radio stuff */
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_ax25 */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
    /* file /usr/include/netax25/ax25.h */
}

#endif
#endif

#ifdef AF_IPX

static void
explain_buffer_sockaddr_af_ipx(explain_string_buffer_t *sb,
    const struct sockaddr_ipx *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_ipx */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
    /* file /usr/include/netipx/ipx.h */
}

#endif

#ifdef AF_APPLETALK

static void
explain_buffer_sockaddr_af_appletalk(explain_string_buffer_t *sb,
    const struct sockaddr_at *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_at */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
    /*
     * file /usr/include/linux/atalk.h
     * sat_port
     * sat_addr
     */
}

#endif

#ifdef AF_NETROM

static void
explain_buffer_sockaddr_af_netrom(explain_string_buffer_t *sb,
    const struct sockaddr *sa, size_t sa_len)
{
    /* amateur radio stuff */
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_netrom */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
    /* file /usr/include/netrom/netrom.h */
}

#endif

#ifdef AF_BRIDGE

static void
explain_buffer_sockaddr_af_bridge(explain_string_buffer_t *sb,
    const struct sockaddr *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_bridge */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif

#ifdef AF_ATMPVC
#ifdef HAVE_LINUX_ATM_H

static void
explain_buffer_sockaddr_af_atmpvc(explain_string_buffer_t *sb,
    const struct sockaddr_atmpvc *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_atmpvc */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
    /* file /usr/include/linux/atm.h */
}

#endif
#endif

#ifdef AF_X25
#ifdef HAVE_LINUX_X25_H

/*
 * See x25(7) for more information.
 */
static void
explain_buffer_sockaddr_af_x25(explain_string_buffer_t *sb,
    const struct sockaddr_x25 *sa, size_t sa_len)
{
    (void)sa_len;
    explain_string_buffer_puts(sb, ", sx25_addr = ");
    explain_string_buffer_puts_quoted(sb, sa->sx25_addr.x25_addr);
}

#endif
#endif

#ifdef AF_INET6

/*
 * See man ipv6(7) for more information.
 */
static void
explain_buffer_sockaddr_af_inet6(explain_string_buffer_t *sb,
    const struct sockaddr_in6 *sa, size_t sa_len)
{
    unsigned short  port;

    /*
     * print the port number, and name if we can
     */
    (void)sa_len;
    port = ntohs(sa->sin6_port);
    explain_string_buffer_printf(sb, ", sin_port = %u", port);
    if (explain_option_dialect_specific())
    {
        struct servent  *sep;

        /*
         * We make this dialect specific, because different systems will
         * have different entries in their /etc/services file, and this
         * could cause false negatives for automated testing.
         */
        /* FIXME: use getservbyport_r if available */
        sep = getservbyport(port, "tcp");
        if (!sep)
            sep = getservbyport(port, "udp");
        if (sep)
        {
            explain_string_buffer_putc(sb, ' ');
            explain_string_buffer_puts_quoted(sb, sep->s_name);
        }
    }

    if (sa->sin6_flowinfo != 0)
    {
        explain_string_buffer_printf
        (
            sb,
            ", sin6_flowinfo = %ld",
            (long)sa->sin6_flowinfo
        );
    }

    /*
     * print the IP address, and name if we can
     */
    explain_string_buffer_puts(sb, ", sin6_addr = ");
    explain_buffer_in6_addr(sb, &sa->sin6_addr);
    if (explain_option_dialect_specific())
    {
        struct hostent  *hep;

        /*
         * We make this dialect specific, because different systems will
         * have different entries in their /etc/hosts file, or there
         * could be transient DNS failures, and these could cause false
         * negatives for automated testing.
         */
        /* FIXME: gethostbyaddr_r if available */
        hep = gethostbyaddr(&sa->sin6_addr, sizeof(sa->sin6_addr), AF_INET6);
        if (hep)
        {
            explain_string_buffer_putc(sb, ' ');
            explain_string_buffer_puts_quoted(sb, hep->h_name);
        }
    }

    if (sa->sin6_scope_id != 0)
    {
        /*
         * sin6_scope_id is an ID of depending of on the scope of the
         * address.  It is new in Linux 2.4.  Linux only supports it for
         * link scope addresses, in that case sin6_scope_id contains the
         * interface index (see netdevice(7))
         */
        explain_string_buffer_printf
        (
            sb,
            ", sin6_scope_id = %ld",
            (long)sa->sin6_scope_id
        );
    }
}

#endif

#ifdef AF_ROSE

static void
explain_buffer_sockaddr_af_rose(explain_string_buffer_t *sb,
    const struct sockaddr_rose *sa, size_t sa_len)
{
    /* amateur radio stuff */
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_rose */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
    /* file /usr/include/netrose/rose.h */
}

#endif

#ifdef AF_DECnet

static void
explain_buffer_sockaddr_af_decnet(explain_string_buffer_t *sb,
    const struct sockaddr *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_decnet */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif

#ifdef AF_NETBEUI

static void
explain_buffer_sockaddr_af_netbeui(explain_string_buffer_t *sb,
    const struct sockaddr *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    /* FIXME: decode sockaddr_netbeui */
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif

#ifdef AF_SECURITY

static void
explain_buffer_sockaddr_af_security(explain_string_buffer_t *sb,
    const struct sockaddr *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_security */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif

#ifdef AF_KEY

static void
explain_buffer_sockaddr_af_key(explain_string_buffer_t *sb,
    const struct sockaddr *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_key */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif

#ifdef AF_NETLINK
#ifdef HAVE_LINUX_NETLINK_H

static void
explain_buffer_sockaddr_af_netlink(explain_string_buffer_t *sb,
    const struct sockaddr_nl *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_nl */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
    /* file /usr/include/linux/netlink.h */
}

#endif
#endif

#ifdef AF_PACKET

static void
explain_buffer_sockaddr_af_packet(explain_string_buffer_t *sb,
    const struct sockaddr_ll *sa, size_t sa_len)
{
    unsigned        alen;

    explain_string_buffer_puts(sb, ", ");
    if (sa_len < sizeof(*sa))
    {
        explain_buffer_hexdump(sb, sa, sa_len);
        return;
    }
    explain_string_buffer_printf
    (
        sb,
        "sll_protocol = %u, ",
        sa->sll_protocol
    );
    explain_string_buffer_printf(sb, "sll_ifindex = %d, ", sa->sll_ifindex);
    explain_string_buffer_printf(sb, "sll_hatype = %u, ", sa->sll_hatype);
    explain_string_buffer_printf(sb, "sll_pkttype = %u, ", sa->sll_pkttype);
    explain_string_buffer_printf(sb, "sll_halen = %u, ", sa->sll_halen);
    explain_string_buffer_puts(sb, "sll_addr = { ");
    alen = sa->sll_halen;
    if (alen > sizeof(sa->sll_addr))
        alen = sizeof(sa->sll_addr);
    explain_buffer_hexdump(sb, sa->sll_addr, alen);
    explain_string_buffer_puts(sb, " }");
}

#endif

#ifdef AF_ASH

static void
explain_buffer_sockaddr_af_ash(explain_string_buffer_t *sb,
    const struct sockaddr_ash *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_ash */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
    /* file /usr/include/netash/ash.h */
}

#endif

#ifdef AF_ECONET

static void
explain_buffer_sockaddr_af_econet(explain_string_buffer_t *sb,
    const struct sockaddr_ec *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_ec */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
    /* file /usr/include/neteconet/ec.h */
}

#endif

#ifdef AF_ATMSVC
#ifdef HAVE_LINUX_ATM_H

static void
explain_buffer_sockaddr_af_atmsvc(explain_string_buffer_t *sb,
    const struct sockaddr_atmsvc *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_atmsvc */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif
#endif

#ifdef AF_SNA

static void
explain_buffer_sockaddr_af_sna(explain_string_buffer_t *sb,
    const struct sockaddr *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_sna */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif

#ifdef AF_IRDA
#ifdef HAVE_LINUX_IRDA_H

static void
explain_buffer_sockaddr_af_irda(explain_string_buffer_t *sb,
    const struct sockaddr_irda *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_irda */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif
#endif

#ifdef AF_PPPOX
#ifdef HAVE_LINUX_IF_PPPOX_H

static void
explain_buffer_sockaddr_af_pppox(explain_string_buffer_t *sb,
    const struct sockaddr_pppox *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_pppox */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
    /* file /usr/include/linux/if_pppox.h */
}

#endif
#endif

#ifdef AF_WANPIPE

static void
explain_buffer_sockaddr_af_wanpipe(explain_string_buffer_t *sb,
    const struct sockaddr *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_wanpipe */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif

#ifdef AF_BLUETOOTH

static void
explain_buffer_sockaddr_af_bluetooth(explain_string_buffer_t *sb,
    const struct sockaddr *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_(something) */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);

    /*
     * file /usr/include/bluetooth/hci.h:    struct sockaddr_hci
     * file /usr/include/bluetooth/l2cap.h:  struct sockaddr_l2
     * file /usr/include/bluetooth/rfcomm.h: struct sockaddr_rc
     * file /usr/include/bluetooth/sco.h:    struct sockaddr_sco
     */
}

#endif

#ifdef AF_IUCV
#ifdef HAVE_NETIUCV_IUCV_H

static void
explain_buffer_sockaddr_af_iucv(explain_string_buffer_t *sb,
    const struct sockaddr_iucv *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_iucv */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif
#endif

#ifdef AF_RXRPC

static void
explain_buffer_sockaddr_af_rxrpc(explain_string_buffer_t *sb,
    const struct sockaddr *sa, size_t sa_len)
{
    explain_string_buffer_putc(sb, ',');
    /* FIXME: decode sockaddr_rxrpc */
    if (sa_len > sizeof(*sa))
        sa_len = sizeof(*sa);
    explain_buffer_hexdump(sb, sa, sa_len);
}

#endif

void
explain_buffer_sockaddr(explain_string_buffer_t *sb,
    const struct sockaddr *sa, int sa_len)
{
    if (explain_is_efault_pointer(sa, sizeof(*sa)))
    {
        explain_buffer_pointer(sb, sa);
        return;
    }
    explain_string_buffer_puts(sb, "{ sa_family = ");
    explain_buffer_address_family(sb, sa->sa_family);
    switch (sa->sa_family)
    {
    case AF_UNSPEC:
        break;

    case AF_UNIX:
        explain_buffer_sockaddr_af_unix
        (
            sb,
            (const struct sockaddr_un *)sa,
            sa_len
        );
        break;

    case AF_INET:
        explain_buffer_sockaddr_af_inet
        (
            sb,
            (const struct sockaddr_in *)sa,
            sa_len
        );
        break;

#ifdef AF_AX25
#ifdef HAVE_LINUX_X25_H
    case AF_AX25:
        explain_buffer_sockaddr_af_ax25
        (
            sb,
            (const struct sockaddr_ax25 *)sa,
            sa_len
        );
        break;
#endif
#endif

#ifdef AF_IPX
    case AF_IPX:
        explain_buffer_sockaddr_af_ipx
        (
            sb,
            (const struct sockaddr_ipx *)sa,
            sa_len
        );
        break;
#endif

#ifdef AF_APPLETALK
    case AF_APPLETALK:
        explain_buffer_sockaddr_af_appletalk
        (
            sb,
            (const struct sockaddr_at *)sa,
            sa_len
        );
        break;
#endif

#ifdef AF_NETROM
    case AF_NETROM:
        explain_buffer_sockaddr_af_netrom(sb, sa, sa_len);
        break;
#endif

#ifdef AF_BRIDGE
    case AF_BRIDGE:
        explain_buffer_sockaddr_af_bridge(sb, sa, sa_len);
        break;
#endif

#ifdef AF_ATMPVC
#ifdef HAVE_LINUX_ATM_H
    case AF_ATMPVC:
        explain_buffer_sockaddr_af_atmpvc
        (
            sb,
            (const struct sockaddr_atmpvc *)sa,
            sa_len
        );
        break;
#endif
#endif

#ifdef AF_X25
#ifdef HAVE_LINUX_X25_H
    case AF_X25:
        explain_buffer_sockaddr_af_x25
        (
            sb,
            (const struct sockaddr_x25 *)sa,
            sa_len
        );
        break;
#endif
#endif

#ifdef AF_INET6
    case AF_INET6:
        explain_buffer_sockaddr_af_inet6
        (
            sb,
            (const struct sockaddr_in6 *)sa,
            sa_len
        );
        break;
#endif

#ifdef AF_ROSE
    case AF_ROSE:
        explain_buffer_sockaddr_af_rose
        (
            sb,
            (const struct sockaddr_rose *)sa,
            sa_len
        );
        break;
#endif

#ifdef AF_DECnet
    case AF_DECnet:
        explain_buffer_sockaddr_af_decnet(sb, sa, sa_len);
        break;
#endif

#ifdef AF_NETBEUI
    case AF_NETBEUI:
        explain_buffer_sockaddr_af_netbeui(sb, sa, sa_len);
        break;
#endif

#ifdef AF_SECURITY
    case AF_SECURITY:
        explain_buffer_sockaddr_af_security(sb, sa, sa_len);
        break;
#endif

#ifdef AF_KEY
    case AF_KEY:
        explain_buffer_sockaddr_af_key(sb, sa, sa_len);
        break;
#endif

#ifdef AF_NETLINK
#ifdef HAVE_LINUX_NETLINK_H
    case AF_NETLINK:
 /* case AF_ROUTE: */
        explain_buffer_sockaddr_af_netlink
        (
            sb,
            (const struct sockaddr_nl *)sa,
            sa_len
        );
        break;
#endif
#endif

#ifdef AF_PACKET
    case AF_PACKET:
        explain_buffer_sockaddr_af_packet
        (
            sb,
            (const struct sockaddr_ll *)sa,
            sa_len
        );
        break;
#endif

#ifdef AF_ASH
    case AF_ASH:
        explain_buffer_sockaddr_af_ash
        (
            sb,
            (const struct sockaddr_ash *)sa,
            sa_len
        );
        break;
#endif

#ifdef AF_ECONET
    case AF_ECONET:
        explain_buffer_sockaddr_af_econet
        (
            sb,
            (const struct sockaddr_ec *)sa,
            sa_len
        );
        break;
#endif

#ifdef AF_ATMSVC
#ifdef HAVE_LINUX_ATM_H
    case AF_ATMSVC:
        explain_buffer_sockaddr_af_atmsvc
        (
            sb,
            (const struct sockaddr_atmsvc *)sa,
            sa_len
        );
        break;
#endif
#endif

#ifdef AF_SNA
    case AF_SNA:
        explain_buffer_sockaddr_af_sna(sb, sa, sa_len);
        break;
#endif

#ifdef AF_IRDA
#ifdef HAVE_LINUX_IRDA_H
    case AF_IRDA:
        explain_buffer_sockaddr_af_irda
        (
            sb,
            (const struct sockaddr_irda *)sa,
            sa_len
        );
        break;
#endif
#endif

#ifdef AF_PPPOX
#ifdef HAVE_LINUX_IF_PPPOX_H
    case AF_PPPOX:
        explain_buffer_sockaddr_af_pppox
        (
            sb,
            (const struct sockaddr_pppox *)sa,
            sa_len
        );
        break;
#endif
#endif

#ifdef AF_WANPIPE
    case AF_WANPIPE:
        explain_buffer_sockaddr_af_wanpipe(sb, sa, sa_len);
        break;
#endif

#ifdef AF_BLUETOOTH
    case AF_BLUETOOTH:
        explain_buffer_sockaddr_af_bluetooth(sb, sa, sa_len);
        break;
#endif

#ifdef AF_IUCV
#ifdef HAVE_NETIUCV_IUCV_H
    case AF_IUCV:
        explain_buffer_sockaddr_af_iucv
        (
            sb,
            (const struct sockaddr_iucv *)sa,
            sa_len
        );
        break;
#endif
#endif

#ifdef AF_RXRPC
    case AF_RXRPC:
        explain_buffer_sockaddr_af_rxrpc(sb, sa, sa_len);
        break;
#endif

    default:
        /* no idea */
        explain_string_buffer_putc(sb, ',');
        explain_buffer_hexdump(sb, sa, sa_len);
        break;
    }
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
