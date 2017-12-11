/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/ac/net/if_ppp.h>
#include <libexplain/ac/net/ppp_defs.h>

#include <libexplain/buffer/npioctl.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_NET_IF_PPP_H


static void
explain_buffer_npioctl_protocol(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "PPP_IP", PPP_IP },
        { "PPP_AT", PPP_AT },
        { "PPP_IPX", PPP_IPX },
        { "PPP_VJC_COMP", PPP_VJC_COMP },
        { "PPP_VJC_UNCOMP", PPP_VJC_UNCOMP },
        { "PPP_MP", PPP_MP },
        { "PPP_IPV6", PPP_IPV6 },
        { "PPP_COMPFRAG", PPP_COMPFRAG },
        { "PPP_COMP", PPP_COMP },
        { "PPP_MPLS_UC", PPP_MPLS_UC },
        { "PPP_MPLS_MC", PPP_MPLS_MC },
        { "PPP_IPCP", PPP_IPCP },
        { "PPP_ATCP", PPP_ATCP },
        { "PPP_IPXCP", PPP_IPXCP },
        { "PPP_IPV6CP", PPP_IPV6CP },
        { "PPP_CCPFRAG", PPP_CCPFRAG },
        { "PPP_CCP", PPP_CCP },
        { "PPP_MPLSCP", PPP_MPLSCP },
        { "PPP_LCP", PPP_LCP },
        { "PPP_PAP", PPP_PAP },
        { "PPP_LQR", PPP_LQR },
        { "PPP_CHAP", PPP_CHAP },
        { "PPP_CBCP", PPP_CBCP },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


static void
explain_buffer_npioctl_mode(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "NPMODE_PASS", NPMODE_PASS },
        { "NPMODE_DROP", NPMODE_DROP },
        { "NPMODE_ERROR", NPMODE_ERROR },
        { "NPMODE_QUEUE", NPMODE_QUEUE },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_npioctl(explain_string_buffer_t *sb, const struct npioctl *data,
    int complete)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ protocol = ");
    explain_buffer_npioctl_protocol(sb, data->protocol);
    if (complete)
    {
        explain_string_buffer_puts(sb, ", mode = ");
        explain_buffer_npioctl_mode(sb, data->mode);
    }
    explain_string_buffer_puts(sb, " }");
}


#else


void
explain_buffer_npioctl(explain_string_buffer_t *sb, const struct npioctl *data,
    int complete)
{
    (void)complete;
    explain_buffer_pointer(sb, data);
}


#endif


/* vim: set ts=8 sw=4 et : */
