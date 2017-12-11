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

#include <libexplain/ac/linux/if_ppp.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/int64_t.h>
#include <libexplain/buffer/pppol2tp_ioc_stats.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef PPPIOCGL2TPSTATS

void
explain_buffer_pppol2tp_ioc_stats(explain_string_buffer_t *sb,
    const struct pppol2tp_ioc_stats *data, int reply)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    explain_string_buffer_puts(sb, ", tunnel_id = ");
    explain_buffer_int(sb, data->tunnel_id);
    explain_string_buffer_puts(sb, ", session_id = ");
    explain_buffer_int(sb, data->session_id);
    if (reply)
    {
        explain_string_buffer_puts(sb, ", using_ipsec = ");
        explain_buffer_int(sb, data->using_ipsec);
        explain_string_buffer_puts(sb, ", tx_packets = ");
        explain_buffer_int64_t(sb, data->tx_packets);
        explain_string_buffer_puts(sb, ", tx_bytes = ");
        explain_buffer_int64_t(sb, data->tx_bytes);
        explain_string_buffer_puts(sb, ", tx_errors = ");
        explain_buffer_int64_t(sb, data->tx_errors);
        explain_string_buffer_puts(sb, ", rx_packets = ");
        explain_buffer_int64_t(sb, data->rx_packets);
        explain_string_buffer_puts(sb, ", rx_bytes = ");
        explain_buffer_int64_t(sb, data->rx_bytes);
        explain_string_buffer_puts(sb, ", rx_seq_discards = ");
        explain_buffer_int64_t(sb, data->rx_seq_discards);
        explain_string_buffer_puts(sb, ", rx_oos_packets = ");
        explain_buffer_int64_t(sb, data->rx_oos_packets);
        explain_string_buffer_puts(sb, ", rx_errors = ");
        explain_buffer_int64_t(sb, data->rx_errors);
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_pppol2tp_ioc_stats(explain_string_buffer_t *sb,
    const struct pppol2tp_ioc_stats *data, int reply)
{
    (void)reply;
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
