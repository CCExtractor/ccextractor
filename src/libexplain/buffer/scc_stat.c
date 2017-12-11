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

#include <libexplain/ac/linux/scc.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/scc_stat.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_SCC_H

void
explain_buffer_scc_stat(explain_string_buffer_t *sb,
    const struct scc_stat *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ rxints = ");
    explain_buffer_long(sb, data->rxints);
    explain_string_buffer_puts(sb, ", txints = ");
    explain_buffer_long(sb, data->txints);
    explain_string_buffer_puts(sb, ", exints = ");
    explain_buffer_long(sb, data->exints);
    explain_string_buffer_puts(sb, ", spints = ");
    explain_buffer_long(sb, data->spints);
    explain_string_buffer_puts(sb, ", txframes = ");
    explain_buffer_long(sb, data->txframes);
    explain_string_buffer_puts(sb, ", rxframes = ");
    explain_buffer_long(sb, data->rxframes);
    explain_string_buffer_puts(sb, ", rxerrs = ");
    explain_buffer_long(sb, data->rxerrs);
    explain_string_buffer_puts(sb, ", txerrs = ");
    explain_buffer_long(sb, data->txerrs);
    explain_string_buffer_puts(sb, ", nospace = ");
    explain_buffer_uint(sb, data->nospace);
    explain_string_buffer_puts(sb, ", rx_over = ");
    explain_buffer_uint(sb, data->rx_over);
    explain_string_buffer_puts(sb, ", tx_under = ");
    explain_buffer_uint(sb, data->tx_under);
    explain_string_buffer_puts(sb, ", tx_state = ");
    explain_buffer_uint(sb, data->tx_state);
    explain_string_buffer_puts(sb, ", tx_queued = ");
    explain_buffer_int(sb, data->tx_queued);
    explain_string_buffer_puts(sb, ", maxqueue = ");
    explain_buffer_uint(sb, data->maxqueue);
    explain_string_buffer_puts(sb, ", bufsize = ");
    explain_buffer_uint(sb, data->bufsize);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_scc_stat(explain_string_buffer_t *sb,
    const struct scc_stat *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
