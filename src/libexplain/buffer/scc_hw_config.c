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
#include <libexplain/buffer/scc_hw_config.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_SCC_H


void
explain_buffer_scc_hw_config(explain_string_buffer_t *sb,
    const struct scc_hw_config *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ data_a = ");
    explain_buffer_ulong(sb, data->data_a);
    explain_string_buffer_puts(sb, ", ctrl_a = ");
    explain_buffer_ulong(sb, data->ctrl_a);
    explain_string_buffer_puts(sb, ", data_b = ");
    explain_buffer_ulong(sb, data->data_b);
    explain_string_buffer_puts(sb, ", ctrl_b = ");
    explain_buffer_ulong(sb, data->ctrl_b);
    explain_string_buffer_puts(sb, ", vector_latch = ");
    explain_buffer_ulong(sb, data->vector_latch);
    explain_string_buffer_puts(sb, ", special = ");
    explain_buffer_ulong(sb, data->special);
    explain_string_buffer_puts(sb, ", irq = ");
    explain_buffer_int(sb, data->irq);
    explain_string_buffer_puts(sb, ", clock = ");
    explain_buffer_long(sb, data->clock);
    explain_string_buffer_puts(sb, ", option = ");
    explain_buffer_int(sb, data->option);
    explain_string_buffer_puts(sb, ", brand = ");
    explain_buffer_int(sb, data->brand);
    explain_string_buffer_puts(sb, ", escc = ");
    explain_buffer_int(sb, data->escc);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_scc_hw_config(explain_string_buffer_t *sb,
    const struct scc_hw_config *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
