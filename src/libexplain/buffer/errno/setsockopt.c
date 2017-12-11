/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/netinet/in.h>
#include <libexplain/ac/sys/socket.h>

#include <libexplain/buffer/char_data.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enoprotoopt.h>
#include <libexplain/buffer/enotsock.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setsockopt.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/buffer/socklen.h>
#include <libexplain/buffer/sockopt_level.h>
#include <libexplain/buffer/sockopt_name.h>
#include <libexplain/buffer/timeval.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_sockopt(explain_string_buffer_t *sb, int level, int name,
    const void *data, socklen_t data_size)
{
    if (explain_is_efault_pointer(data, data_size))
    {
        explain_buffer_pointer(sb, data);
        return;
    }
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
        switch (name)
        {
        case IP_TOS:
        case IP_TTL:
        case IP_HDRINCL:
#ifdef IP_ROUTER_ALERT
        case IP_ROUTER_ALERT:
#endif
        case IP_RECVOPTS:
#ifdef IP_PKTINFO
        case IP_PKTINFO:
#endif
#ifdef IP_MTU_DISCOVER
        case IP_MTU_DISCOVER:
#endif
#ifdef IP_RECVERR
        case IP_RECVERR:
#endif
        case IP_RECVTTL:
#ifdef IP_RECVTOS
        case IP_RECVTOS:
#endif
        case IP_MULTICAST_TTL:
        case IP_MULTICAST_LOOP:
            if (data_size >= sizeof(int))
            {
                explain_buffer_int_star(sb, data);
                break;
            }
            goto dunno;

#ifndef __bsd__
        case IP_OPTIONS:
        case IP_RETOPTS:
            if (data_size >= sizeof(struct ip_opts))
            {
                const struct ip_opts *p;

                p = data;
                explain_string_buffer_puts(sb, "{ ip_dst = ");
                explain_buffer_in_addr(sb, &p->ip_dst);
                explain_string_buffer_puts(sb, ", ip_opts = ");
                explain_buffer_char_data(sb, p->ip_opts, sizeof(p->ip_opts));
                explain_string_buffer_putc(sb, '}');
                break;
            }
            goto dunno;
#endif

        case IP_PKTOPTIONS:
            goto dunno;

        case IP_MSFILTER:
        case MCAST_MSFILTER:
            if (data_size >= sizeof(struct ip_msfilter))
            {
                const struct ip_msfilter *p;
                size_t          j;

                p = data;
                explain_string_buffer_puts(sb, "{ imsf_multiaddr = ");
                explain_buffer_in_addr(sb, &p->imsf_multiaddr);
                explain_string_buffer_puts(sb, ", imsf_interface = ");
                explain_buffer_in_addr(sb, &p->imsf_interface);
                explain_string_buffer_printf
                (
                    sb,
                    ", imsf_fmode = 0x%08lX, imsf_numsrc = 0x%08lX, "
                        "imsf_slist = { ",
                    (long)p->imsf_fmode,
                    (long)p->imsf_numsrc
                );
                for (j = 0; j < p->imsf_numsrc; ++j)
                {
                    const struct in_addr *q;
                    const char      *end;

                    q = &p->imsf_slist[j];
                    end = (const char *)(q + 1);
                    if ((socklen_t)(end - (const char *)data) > data_size)
                        break;
                    if (j)
                        explain_string_buffer_puts(sb, ", ");
                    explain_buffer_in_addr(sb, q);
                }
                explain_string_buffer_puts(sb, " }}");
                break;
            }
            goto dunno;

        case IP_MULTICAST_IF:
            if (data_size >= sizeof(struct in_addr))
            {
                const struct in_addr *p;

                p = data;
                explain_string_buffer_puts(sb, "{ ");
                explain_buffer_in_addr(sb, p);
                explain_string_buffer_puts(sb, " }");
                break;
            }
            goto dunno;

        case IP_ADD_MEMBERSHIP:
        case IP_DROP_MEMBERSHIP:
            if (data_size == sizeof(struct ip_mreq))
            {
                const struct ip_mreq *p;

                p = data;
                explain_string_buffer_puts(sb, "{ imr_multiaddr = ");
                explain_buffer_in_addr(sb, &p->imr_multiaddr);
                explain_string_buffer_puts(sb, ", imr_interface = ");
                explain_buffer_in_addr(sb, &p->imr_interface);
                explain_string_buffer_puts(sb, " }");
                break;
            }
            if (data_size >= sizeof(struct ip_mreqn))
            {
                const struct ip_mreqn *p;

                p = data;
                explain_string_buffer_puts(sb, "{ imr_multiaddr = ");
                explain_buffer_in_addr(sb, &p->imr_multiaddr);
                explain_string_buffer_puts(sb, ", imr_address = ");
                explain_buffer_in_addr(sb, &p->imr_address);
                explain_string_buffer_printf
                (
                    sb,
                    ", imr_ifindex = %d }",
                    p->imr_ifindex
                );
                break;
            }
            goto dunno;

        case IP_UNBLOCK_SOURCE:
        case IP_BLOCK_SOURCE:
        case IP_ADD_SOURCE_MEMBERSHIP:
        case IP_DROP_SOURCE_MEMBERSHIP:
            if (data_size >= sizeof(struct ip_mreq_source))
            {
                const struct ip_mreq_source *p;

                p = data;
                explain_string_buffer_puts(sb, "{ imr_multiaddr = ");
                explain_buffer_in_addr(sb, &p->imr_multiaddr);
                explain_string_buffer_puts(sb, ", imr_interface = ");
                explain_buffer_in_addr(sb, &p->imr_interface);
                explain_string_buffer_puts(sb, ", imr_sourceaddr = ");
                explain_buffer_in_addr(sb, &p->imr_sourceaddr);
                explain_string_buffer_puts(sb, " }");
                break;
            }
            goto dunno;

        case MCAST_JOIN_GROUP:
        case MCAST_LEAVE_GROUP:
            if (data_size >= sizeof(struct group_req))
            {
                const struct group_req *p;

                p = data;
                explain_string_buffer_printf
                (
                    sb,
                    "{ gr_interface = %lu, gr_group = ",
                    (long)p->gr_interface
                );
                explain_buffer_sockaddr
                (
                    sb,
                    (const struct sockaddr *)&p->gr_group,
                    sizeof(p->gr_group)
                );
                explain_string_buffer_puts(sb, " }");
                break;
            }
            goto dunno;

        case MCAST_BLOCK_SOURCE:
        case MCAST_UNBLOCK_SOURCE:
        case MCAST_JOIN_SOURCE_GROUP:
        case MCAST_LEAVE_SOURCE_GROUP:
            if (data_size >= sizeof(struct group_source_req))
            {
                const struct group_source_req *p;

                p = data;
                explain_string_buffer_printf
                (
                    sb,
                    "{ gsr_interface = %lu, gsr_group = ",
                    (unsigned long)p->gsr_interface
                );
                explain_buffer_sockaddr
                (
                    sb,
                    (const struct sockaddr *)&p->gsr_group,
                    sizeof(p->gsr_group)
                );
                explain_string_buffer_puts(sb, ", gsr_source = ");
                explain_buffer_sockaddr
                (
                    sb,
                    (const struct sockaddr *)&p->gsr_source,
                    sizeof(p->gsr_source)
                );
                explain_string_buffer_puts(sb, " }");
                break;
            }
            goto dunno;

        default:
            goto dunno;
        }
        break;
#endif

    /* FIXME: add support for SOL_IPV6 */
    /* FIXME: add support for SOL_IPX */
    /* FIXME: add support for SOL_IRDA */
    /* FIXME: add support for SOL_LLC */
    /* FIXME: add support for SOL_NETBEUI */
    /* FIXME: add support for SOL_NETLINK */
    /* FIXME: add support for SOL_NETROM */
    /* FIXME: add support for SOL_PACKET */
    /* FIXME: add support for SOL_PPPOL2TP */
    /* FIXME: add support for SOL_RAW */
    /* FIXME: add support for SOL_ROSE */
    /* FIXME: add support for SOL_RXRPC */
    /* FIXME: add support for SOL_SCTP */

    case SOL_SOCKET:
        switch (name)
        {
#ifdef SO_BINDTODEVICE
        case SO_BINDTODEVICE:
            explain_string_buffer_puts_quoted_n(sb, data, data_size);
            break;
#endif

        case SO_LINGER:
            if (data_size >= sizeof(struct linger))
            {
                const struct linger *p;

                p = data;
                explain_string_buffer_printf
                (
                    sb,
                    "{ l_onoff = %d, l_linger = %d }",
                    p->l_onoff,
                    p->l_linger
                );
                break;
            }
            goto dunno;

        case SO_RCVTIMEO:
        case SO_SNDTIMEO:
            if (data_size >= sizeof(struct timeval))
            {
                explain_buffer_timeval(sb, data);
                break;
            }
            goto dunno;

        default:
            goto dunno;
        }
        break;

    /* FIXME: add supoirt for SOL_TCP */
    /* FIXME: add supoirt for SOL_TIPC */
    /* FIXME: add supoirt for SOL_UDP */
    /* FIXME: add supoirt for SOL_UDPLITE */
    /* FIXME: add supoirt for SOL_X25 */

    default:
        dunno:
        if (data_size == sizeof(int))
            explain_buffer_int_star(sb, data);
        else
            explain_buffer_char_data(sb, data, data_size);
        break;
    }
}


static void
explain_buffer_errno_setsockopt_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, int level, int name, void *data,
    socklen_t data_size)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "setsockopt(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", level = ");
    explain_buffer_sockopt_level(sb, level);
    explain_string_buffer_puts(sb, ", name = ");
    explain_buffer_sockopt_name(sb, level, name);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_sockopt(sb, level, name, data, data_size);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_socklen(sb, data_size);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_setsockopt_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes, int level, int name, void *data,
    socklen_t data_size)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setsockopt.html
     */
    (void)level;
    (void)name;
    (void)data;
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINVAL:
        explain_buffer_einval_too_small(sb, "data_size", data_size);
        break;

    case ENOPROTOOPT:
        explain_buffer_enoprotoopt(sb, "name");
        break;

    case ENOTSOCK:
        explain_buffer_enotsock(sb, fildes, "fildes");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "setsockopt");
        break;
    }
}


void
explain_buffer_errno_setsockopt(explain_string_buffer_t *sb, int errnum,
    int fildes, int level, int name, void *data, socklen_t data_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setsockopt_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        level,
        name,
        data,
        data_size
    );
    explain_buffer_errno_setsockopt_explanation
    (
        &exp.explanation_sb,
        errnum,
        fildes,
        level,
        name,
        data,
        data_size
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
