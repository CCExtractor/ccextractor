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

#include <libexplain/ac/linux/hayesesp.h>

#include <libexplain/buffer/hayes_esp_config.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_HAYESESP_H

void
explain_buffer_hayes_esp_config(explain_string_buffer_t *sb,
    const struct hayes_esp_config *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ flow_on = ");
    explain_buffer_int(sb, data->flow_on);
    explain_string_buffer_puts(sb, ", flow_off = ");
    explain_buffer_int(sb, data->flow_off);
    explain_string_buffer_puts(sb, ", rx_trigger = ");
    explain_buffer_int(sb, data->rx_trigger);
    explain_string_buffer_puts(sb, ", tx_trigger = ");
    explain_buffer_int(sb, data->tx_trigger);
    explain_string_buffer_puts(sb, ", pio_threshold = ");
    explain_buffer_int(sb, data->pio_threshold);
    explain_string_buffer_puts(sb, ", rx_timeout");
    explain_buffer_int(sb, data->rx_timeout);
    explain_string_buffer_puts(sb, ", dma_channel = ");
    explain_buffer_int(sb, data->dma_channel);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_hayes_esp_config(explain_string_buffer_t *sb,
    const struct hayes_esp_config *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
