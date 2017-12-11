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

#include <libexplain/ac/net/route.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/rtentry.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>

#if defined(SIOCADDRT) || defined(SIOCDELRT) || defined(SIOCRTMSG)

static void
explain_buffer_rtentry_flags(explain_string_buffer_t *sb, int flags)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef RTF_UP
        { "RTF_UP", RTF_UP },
#endif
#ifdef RTF_GATEWAY
        { "RTF_GATEWAY", RTF_GATEWAY },
#endif
#ifdef RTF_HOST
        { "RTF_HOST", RTF_HOST },
#endif
#ifdef RTF_REINSTATE
        { "RTF_REINSTATE", RTF_REINSTATE },
#endif
#ifdef RTF_DYNAMIC
        { "RTF_DYNAMIC", RTF_DYNAMIC },
#endif
#ifdef RTF_MODIFIED
        { "RTF_MODIFIED", RTF_MODIFIED },
#endif
#ifdef RTF_MTU
        { "RTF_MTU", RTF_MTU },
#endif
#ifdef RTF_WINDOW
        { "RTF_WINDOW", RTF_WINDOW },
#endif
#ifdef RTF_IRTT
        { "RTF_IRTT", RTF_IRTT },
#endif
#ifdef RTF_REJECT
        { "RTF_REJECT", RTF_REJECT },
#endif
    };

    explain_parse_bits_print(sb, flags, table, SIZEOF(table));
}


void
explain_buffer_rtentry(explain_string_buffer_t *sb,
    const struct rtentry *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ rt_gateway = ");
#ifndef __linux__
    explain_buffer_sockaddr(sb, data->rt_gateway, sizeof(*data->rt_gateway));
#else
    explain_buffer_sockaddr(sb, &data->rt_gateway, sizeof(data->rt_gateway));
    explain_string_buffer_puts(sb, ", rt_dst = ");
    explain_buffer_sockaddr(sb, &data->rt_dst, sizeof(data->rt_dst));
    explain_string_buffer_puts(sb, ", rt_genmask = ");
    explain_buffer_sockaddr(sb, &data->rt_genmask, sizeof(data->rt_genmask));
#endif
    explain_string_buffer_puts(sb, ", rt_flags = ");
    explain_buffer_rtentry_flags(sb, data->rt_flags);
#ifdef __linux__
    explain_string_buffer_puts(sb, ", rt_metric = ");
    explain_buffer_int(sb, data->rt_metric);
    explain_string_buffer_puts(sb, ", rt_dev = ");
    explain_buffer_pathname(sb, data->rt_dev);
    explain_string_buffer_puts(sb, ", rt_mtu = ");
    explain_buffer_long(sb, data->rt_mtu);
    explain_string_buffer_puts(sb, ", rt_window = ");
    explain_buffer_long(sb, data->rt_window);
    explain_string_buffer_puts(sb, ", rt_irtt = ");
    explain_buffer_int(sb, data->rt_irtt);
#endif
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_rtentry(explain_string_buffer_t *sb,
    const struct rtentry *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
