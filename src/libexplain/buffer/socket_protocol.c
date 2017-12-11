/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/socket.h>
#include <libexplain/ac/netdb.h>
#include <libexplain/ac/netinet/in.h>

#include <libexplain/parse_bits.h>
#include <libexplain/buffer/socket_protocol.h>
#include <libexplain/sizeof.h>
#include <libexplain/string_buffer.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef IPPROTO_IP
    { "IPPROTO_IP", IPPROTO_IP },
#endif
#ifdef IPPROTO_ICMP
    { "IPPROTO_ICMP", IPPROTO_ICMP },
#endif
#ifdef IPPROTO_IGMP
    { "IPPROTO_IGMP", IPPROTO_IGMP },
#endif
#ifdef IPPROTO_IPIP
    { "IPPROTO_IPIP", IPPROTO_IPIP },
#endif
#ifdef IPPROTO_TCP
    { "IPPROTO_TCP", IPPROTO_TCP },
#endif
#ifdef IPPROTO_EGP
    { "IPPROTO_EGP", IPPROTO_EGP },
#endif
#ifdef IPPROTO_PUP
    { "IPPROTO_PUP", IPPROTO_PUP },
#endif
#ifdef IPPROTO_UDP
    { "IPPROTO_UDP", IPPROTO_UDP },
#endif
#ifdef IPPROTO_IDP
    { "IPPROTO_IDP", IPPROTO_IDP },
#endif
#ifdef IPPROTO_TP
    { "IPPROTO_TP", IPPROTO_TP },
#endif
#ifdef IPPROTO_IPV6
    { "IPPROTO_IPV6", IPPROTO_IPV6 },
#endif
#ifdef IPPROTO_ROUTING
    { "IPPROTO_ROUTING", IPPROTO_ROUTING },
#endif
#ifdef IPPROTO_FRAGMENT
    { "IPPROTO_FRAGMENT", IPPROTO_FRAGMENT },
#endif
#ifdef IPPROTO_RSVP
    { "IPPROTO_RSVP", IPPROTO_RSVP },
#endif
#ifdef IPPROTO_GRE
    { "IPPROTO_GRE", IPPROTO_GRE },
#endif
#ifdef IPPROTO_ESP
    { "IPPROTO_ESP", IPPROTO_ESP },
#endif
#ifdef IPPROTO_AH
    { "IPPROTO_AH", IPPROTO_AH },
#endif
#ifdef IPPROTO_ICMPV6
    { "IPPROTO_ICMPV6", IPPROTO_ICMPV6 },
#endif
#ifdef IPPROTO_NONE
    { "IPPROTO_NONE", IPPROTO_NONE },
#endif
#ifdef IPPROTO_MTP
    { "IPPROTO_MTP", IPPROTO_MTP },
#endif
#ifdef IPPROTO_ENCAP
    { "IPPROTO_ENCAP", IPPROTO_ENCAP },
#endif
#ifdef IPPROTO_PIM
    { "IPPROTO_PIM", IPPROTO_PIM },
#endif
#ifdef IPPROTO_COMP
    { "IPPROTO_COMP", IPPROTO_COMP },
#endif
#ifdef IPPROTO_SCTP
    { "IPPROTO_SCTP", IPPROTO_SCTP },
#endif
#ifdef IPPROTO_RAW
    { "IPPROTO_RAW", IPPROTO_RAW },
#endif
};


void
explain_buffer_socket_protocol(explain_string_buffer_t *sb, int protocol)
{
    const explain_parse_bits_table_t *tp;
    struct protoent *pep;

    tp = explain_parse_bits_find_by_value(protocol, table, SIZEOF(table));
    if (tp)
    {
        explain_string_buffer_puts(sb, tp->name);
        return;
    }

    explain_string_buffer_printf(sb, "%d", protocol);

    pep = getprotobynumber(protocol);
    if (pep)
    {
        explain_string_buffer_putc(sb, ' ');
        explain_string_buffer_puts_quoted(sb, pep->p_name);
    }
}


int
explain_parse_socket_protocol_or_die(const char *text, const char *caption)
{
    struct protoent *pep;

    pep = getprotobyname(text);
    if (pep)
        return pep->p_proto;
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
