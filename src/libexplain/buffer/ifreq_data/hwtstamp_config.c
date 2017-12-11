/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/linux/net_tstamp.h>
#include <libexplain/ac/net/if.h>

#include <libexplain/buffer/ifreq_data/hwtstamp_config.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>


void
explain_buffer_ifreq_data_hwtstamp_config(explain_string_buffer_t *sb,
    const struct ifreq *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ifr_data = ");
    explain_buffer_hwtstamp_config
    (
        sb,
        (const struct hwtstamp_config *)data->ifr_data
    );
    explain_string_buffer_puts(sb, " }");
}

#ifdef HAVE_LINUX_NET_TSTAMP_H

static void
explain_buffer_hwtstamp_config_flags(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "SOF_TIMESTAMPING_TX_HARDWARE", SOF_TIMESTAMPING_TX_HARDWARE },
        { "SOF_TIMESTAMPING_TX_SOFTWARE", SOF_TIMESTAMPING_TX_SOFTWARE },
        { "SOF_TIMESTAMPING_RX_HARDWARE", SOF_TIMESTAMPING_RX_HARDWARE },
        { "SOF_TIMESTAMPING_RX_SOFTWARE", SOF_TIMESTAMPING_RX_SOFTWARE },
        { "SOF_TIMESTAMPING_SOFTWARE", SOF_TIMESTAMPING_SOFTWARE },
        { "SOF_TIMESTAMPING_SYS_HARDWARE", SOF_TIMESTAMPING_SYS_HARDWARE },
        { "SOF_TIMESTAMPING_RAW_HARDWARE", SOF_TIMESTAMPING_RAW_HARDWARE },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


static void
explain_buffer_hwtstamp_config_tx_type(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "HWTSTAMP_TX_OFF", HWTSTAMP_TX_OFF },
        { "HWTSTAMP_TX_ON", HWTSTAMP_TX_ON },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


static void
explain_buffer_hwtstamp_config_rx_filter(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "HWTSTAMP_FILTER_NONE", HWTSTAMP_FILTER_NONE },
        { "HWTSTAMP_FILTER_ALL", HWTSTAMP_FILTER_ALL },
        { "HWTSTAMP_FILTER_SOME", HWTSTAMP_FILTER_SOME },
        { "HWTSTAMP_FILTER_PTP_V1_L4_EVENT", HWTSTAMP_FILTER_PTP_V1_L4_EVENT },
        { "HWTSTAMP_FILTER_PTP_V1_L4_SYNC", HWTSTAMP_FILTER_PTP_V1_L4_SYNC },
        { "HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ",
            HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ },
        { "HWTSTAMP_FILTER_PTP_V2_L4_EVENT", HWTSTAMP_FILTER_PTP_V2_L4_EVENT },
        { "HWTSTAMP_FILTER_PTP_V2_L4_SYNC", HWTSTAMP_FILTER_PTP_V2_L4_SYNC },
        { "HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ",
            HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ },
        { "HWTSTAMP_FILTER_PTP_V2_L2_EVENT", HWTSTAMP_FILTER_PTP_V2_L2_EVENT },
        { "HWTSTAMP_FILTER_PTP_V2_L2_SYNC", HWTSTAMP_FILTER_PTP_V2_L2_SYNC },
        { "HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ",
            HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ },
        { "HWTSTAMP_FILTER_PTP_V2_EVENT", HWTSTAMP_FILTER_PTP_V2_EVENT },
        { "HWTSTAMP_FILTER_PTP_V2_SYNC", HWTSTAMP_FILTER_PTP_V2_SYNC },
        { "HWTSTAMP_FILTER_PTP_V2_DELAY_REQ",
            HWTSTAMP_FILTER_PTP_V2_DELAY_REQ },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_hwtstamp_config(explain_string_buffer_t *sb,
    const struct hwtstamp_config *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ flags = ");
    explain_buffer_hwtstamp_config_flags(sb, data->flags);
    explain_string_buffer_puts(sb, ", tx_type = ");
    explain_buffer_hwtstamp_config_tx_type(sb, data->tx_type);
    explain_string_buffer_puts(sb, ", rx_filter = ");
    explain_buffer_hwtstamp_config_rx_filter(sb, data->rx_filter);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_hwtstamp_config(explain_string_buffer_t *sb,
    const struct hwtstamp_config *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
