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

#include <libexplain/ac/linux/serial.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/serial_icounter_struct.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_SERIAL_H

void
explain_buffer_serial_icounter_struct(explain_string_buffer_t *sb,
    const struct serial_icounter_struct *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ rx = ");
    explain_buffer_int(sb, data->rx);
    explain_string_buffer_puts(sb, ", tx = ");
    explain_buffer_int(sb, data->tx);
    if (data->cts)
    {
        explain_string_buffer_puts(sb, ", cts = ");
        explain_buffer_int(sb, data->cts);
    }
    if (data->dsr)
    {
        explain_string_buffer_puts(sb, ", dsr = ");
        explain_buffer_int(sb, data->dsr);
    }
    if (data->rng)
    {
        explain_string_buffer_puts(sb, ", rng = ");
        explain_buffer_int(sb, data->rng);
    }
    if (data->dcd)
    {
        explain_string_buffer_puts(sb, ", dcd = ");
        explain_buffer_int(sb, data->dcd);
    }
    if (data->frame)
    {
        explain_string_buffer_puts(sb, ", frame = ");
        explain_buffer_int(sb, data->frame);
    }
    if (data->overrun)
    {
        explain_string_buffer_puts(sb, ", overrun = ");
        explain_buffer_int(sb, data->overrun);
    }
    if (data->parity)
    {
        explain_string_buffer_puts(sb, ", parity = ");
        explain_buffer_int(sb, data->parity);
    }
    if (data->brk)
    {
        explain_string_buffer_puts(sb, ", brk = ");
        explain_buffer_int(sb, data->brk);
    }
    if (data->buf_overrun)
    {
        explain_string_buffer_puts(sb, ", buf_overrun = ");
        explain_buffer_int(sb, data->buf_overrun);
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_serial_icounter_struct(explain_string_buffer_t *sb,
    const struct serial_icounter_struct *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
