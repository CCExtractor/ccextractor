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

#include <libexplain/ac/arpa/inet.h>
#include <libexplain/ac/linux/x25.h>
#include <libexplain/ac/net/route.h>
#include <libexplain/ac/netax25/ax25.h>
#include <libexplain/ac/netrom/netrom.h>
#include <libexplain/ac/netrose/rose.h>

#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/route_struct.h>
#include <libexplain/buffer/rtentry.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/fildes_to_address_family.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_NETAX25_AX25_H

static void
explain_buffer_ax25_address(explain_string_buffer_t *sb,
    const ax25_address *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ax25_call =");
    explain_buffer_hexdump(sb, data->ax25_call, sizeof(data->ax25_call));
    explain_string_buffer_puts(sb, " }");

}


static void
explain_buffer_ax25_routes_struct(explain_string_buffer_t *sb,
    const struct ax25_routes_struct *data)
{
    unsigned        j;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ port_addr = ");
    explain_buffer_ax25_address(sb, &data->port_addr);
    explain_string_buffer_puts(sb, ", dest_addr = ");
    explain_buffer_ax25_address(sb, &data->dest_addr);
    explain_string_buffer_printf
    (
        sb,
        ", digi_count = %u, digi_addr = {",
        data->digi_count
    );
    for (j = 0; j < data->digi_count && j < AX25_MAX_DIGIS; ++j)
    {
        explain_string_buffer_puts(sb, ", ");
        explain_buffer_ax25_address(sb, &data->digi_addr[j]);
    }
    explain_string_buffer_puts(sb, " } }");
}

#endif

#ifdef HAVE_NETROM_NETROM_H

static void
explain_buffer_netrom_route_type(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "NETROM_NEIGH", NETROM_NEIGH },
        { "NETROM_NODE", NETROM_NODE },
    };

    explain_parse_bits_print_single(sb, data, table, SIZEOF(table));
}

static void
explain_buffer_nr_route_struct(explain_string_buffer_t *sb,
    const struct nr_route_struct *data)
{
    unsigned        j;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_netrom_route_type(sb, data->type);
    explain_string_buffer_puts(sb, ", callsign = ");
    explain_buffer_ax25_address(sb, &data->callsign);
    explain_string_buffer_puts(sb, ", device = ");
    explain_string_buffer_puts_quoted(sb, data->device);
    explain_string_buffer_printf(sb, ", quality = %u, ", data->quality);
    explain_string_buffer_puts(sb, "mnemonic = ");
    explain_string_buffer_puts_quoted_n
    (
        sb,
        data->mnemonic,
        sizeof(data->mnemonic)
    );
    explain_string_buffer_puts(sb, ", neighbour = ");
    explain_buffer_ax25_address(sb, &data->neighbour);
    explain_string_buffer_printf(sb, ", obs_count = %u, ", data->obs_count);
    explain_string_buffer_printf(sb, "ndigis = %u, ", data->ndigis);
    explain_string_buffer_puts(sb, ", digipeaters = {");
    for (j = 0; j < data->ndigis && j < AX25_MAX_DIGIS; ++j)
    {
        if (j)
            explain_string_buffer_putc(sb, ',');
        explain_string_buffer_putc(sb, ' ');
        explain_buffer_ax25_address(sb, &data->digipeaters[j]);
    }
    explain_string_buffer_puts(sb, " } }");
}

#endif
#ifdef __linux__

static void
explain_buffer_in6_addr(explain_string_buffer_t *sb,
    const struct in6_addr *data)
{
    char            straddr[200];

    inet_ntop(AF_INET6, data, straddr, sizeof(straddr));
    explain_string_buffer_puts(sb, straddr);
}


static void
explain_buffer_rtmsg_type(explain_string_buffer_t *sb,
    unsigned long data)
{
    static const explain_parse_bits_table_t table[] =
    {
#if 0
        /*
         * These are defined, but in terms of symbols that are are NOT
         * defined.  Sheesh.
         */
        { "RTMSG_ACK", RTMSG_ACK },
        { "RTMSG_OVERRUN", RTMSG_OVERRUN },
#endif
#ifdef RTMSG_NEWDEVICE
        { "RTMSG_NEWDEVICE", RTMSG_NEWDEVICE },
#endif
#ifdef RTMSG_DELDEVICE
        { "RTMSG_DELDEVICE", RTMSG_DELDEVICE },
#endif
#ifdef RTMSG_NEWROUTE
        { "RTMSG_NEWROUTE", RTMSG_NEWROUTE },
#endif
#ifdef RTMSG_DELROUTE
        { "RTMSG_DELROUTE", RTMSG_DELROUTE },
#endif
#ifdef RTMSG_NEWRULE
        { "RTMSG_NEWRULE", RTMSG_NEWRULE },
#endif
#ifdef RTMSG_DELRULE
        { "RTMSG_DELRULE", RTMSG_DELRULE },
#endif
#ifdef RTMSG_CONTROL
        { "RTMSG_CONTROL", RTMSG_CONTROL },
#endif
#ifdef RTMSG_AR_FAILED
        { "RTMSG_AR_FAILED", RTMSG_AR_FAILED },
#endif
    };

    explain_parse_bits_print(sb, data, table, SIZEOF(table));
}


static void
explain_buffer_rtmsg_flags(explain_string_buffer_t *sb,
    unsigned long data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "RTF_DEFAULT", RTF_DEFAULT },
        { "RTF_ALLONLINK", RTF_ALLONLINK },
        { "RTF_ADDRCONF", RTF_ADDRCONF },
        { "RTF_LINKRT", RTF_LINKRT },
        { "RTF_NONEXTHOP", RTF_NONEXTHOP },
        { "RTF_CACHE", RTF_CACHE },
        { "RTF_FLOW", RTF_FLOW },
        { "RTF_POLICY", RTF_POLICY },
        { "RTCF_VALVE", RTCF_VALVE },
        { "RTCF_MASQ", RTCF_MASQ },
        { "RTCF_NAT", RTCF_NAT },
        { "RTCF_DOREDIRECT", RTCF_DOREDIRECT },
        { "RTCF_LOG", RTCF_LOG },
        { "RTCF_DIRECTSRC", RTCF_DIRECTSRC },
        { "RTF_LOCAL", RTF_LOCAL },
        { "RTF_INTERFACE", RTF_INTERFACE },
        { "RTF_MULTICAST", RTF_MULTICAST },
        { "RTF_BROADCAST", RTF_BROADCAST },
        { "RTF_NAT", RTF_NAT },
    };

    explain_parse_bits_print(sb, data, table, SIZEOF(table));
}


static void
explain_buffer_in6_rtmsg(explain_string_buffer_t *sb,
    const struct in6_rtmsg *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ rtmsg_dst = ");
    explain_buffer_in6_addr(sb, &data->rtmsg_dst);
    explain_string_buffer_puts(sb, ", rtmsg_src = ");
    explain_buffer_in6_addr(sb, &data->rtmsg_src);
    explain_string_buffer_puts(sb, ", rtmsg_gateway = ");
    explain_buffer_in6_addr(sb, &data->rtmsg_gateway);
    explain_string_buffer_puts(sb, ", rtmsg_type = ");
    explain_buffer_rtmsg_type(sb, data->rtmsg_type);
    explain_string_buffer_printf
    (
        sb,
        ", rtmsg_dst_len = %u, ",
        data->rtmsg_dst_len
    );
    explain_string_buffer_printf
    (
        sb,
        "rtmsg_src_len = %u, ",
        data->rtmsg_src_len
    );
    explain_string_buffer_printf
    (
        sb,
        "rtmsg_metric = %lu, ",
        (unsigned long)data->rtmsg_metric
    );
    explain_string_buffer_printf
    (
        sb,
        "rtmsg_info = %lu, ",
        data->rtmsg_info
    );
    explain_string_buffer_puts(sb, "rtmsg_flags = ");
    explain_buffer_rtmsg_flags(sb, data->rtmsg_flags);
    explain_string_buffer_printf
    (
        sb,
        ", rtmsg_ifindex = %d }",
        data->rtmsg_ifindex
    );
}

#endif
#ifdef HAVE_NETROSE_ROSE_H

static void
explain_buffer_rose_address(explain_string_buffer_t *sb,
    const rose_address *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ rose_addr = ");
    explain_string_buffer_puts_quoted_n
    (
        sb,
        data->rose_addr,
        sizeof(data->rose_addr)
    );
    explain_string_buffer_puts(sb, " }");
}


static void
explain_buffer_rose_route_struct(explain_string_buffer_t *sb,
    const struct rose_route_struct *data)
{
    unsigned        j;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ address = ");
    explain_buffer_rose_address(sb, &data->address);
    explain_string_buffer_printf(sb, ", mask = %u, ", data->mask);
    explain_string_buffer_puts(sb, "neighbour = ");
    explain_buffer_ax25_address(sb, &data->neighbour);
    explain_string_buffer_puts(sb, ", device = ");
    explain_string_buffer_puts_quoted_n
    (
        sb,
        data->device,
        sizeof(data->device)
    );
    explain_string_buffer_printf(sb, ", ndigis = %u, ", data->ndigis);
    explain_string_buffer_puts(sb, "digipeaters = {");
    for (j = 0; j < data->ndigis && j < AX25_MAX_DIGIS; ++j)
    {
        if (j)
            explain_string_buffer_putc(sb, ',');
        explain_string_buffer_putc(sb, ' ');
        explain_buffer_ax25_address(sb, &data->digipeaters[j]);
    }
    explain_string_buffer_puts(sb, " } }");
}

#endif
#ifdef HAVE_LINUX_X25_H

static void
explain_buffer_x25_address(explain_string_buffer_t *sb,
    const struct x25_address *data)
{
    explain_string_buffer_puts(sb, "{ x25_addr = ");
    explain_string_buffer_puts_quoted_n
    (
        sb,
        data->x25_addr,
        sizeof(data->x25_addr)
    );
    explain_string_buffer_puts(sb, " }");
}

static void
explain_buffer_x25_route_struct(explain_string_buffer_t *sb,
    const struct x25_route_struct *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ address = ");
    explain_buffer_x25_address(sb, &data->address);
    explain_string_buffer_printf(sb, ", sigdigits = %u, ", data->sigdigits);
    explain_string_buffer_puts(sb, "device = ");
    explain_string_buffer_puts_quoted_n
    (
        sb,
        data->device,
        sizeof(data->device)
    );
    explain_string_buffer_puts(sb, " }");
}

#endif


void
explain_buffer_route_struct(explain_string_buffer_t *sb, int fildes,
    const void *data)
{
    if (!data)
    {
        print_pointer:
        explain_buffer_pointer(sb, data);
        return;
    }
    switch (explain_fildes_to_address_family(fildes))
    {
    case -1:
        goto print_pointer;

    default:
        explain_buffer_rtentry(sb, data);
        break;

#ifdef HAVE_NETAX25_AX25_H
    case AF_AX25:
        explain_buffer_ax25_routes_struct(sb, data);
        break;
#endif

#ifdef __linux__
    case AF_INET6:
        explain_buffer_in6_rtmsg(sb, data);
        break;
#endif

#ifdef HAVE_NETROM_NETROM_H
    case AF_NETROM:
        explain_buffer_nr_route_struct(sb, data);
        break;
#endif

#ifdef HAVE_NETROSE_ROSE_H
    case AF_ROSE:
        explain_buffer_rose_route_struct(sb, data);
        break;
#endif

#ifdef HAVE_LINUX_X25_H
    case AF_X25:
        explain_buffer_x25_route_struct(sb, data);
        break;
#endif
    }
}


/* vim: set ts=8 sw=4 et : */
