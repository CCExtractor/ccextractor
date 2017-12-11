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

#include <libexplain/ac/linux/fd.h>

#include <libexplain/buffer/char_data.h>
#include <libexplain/buffer/floppy_fdc_state.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_FD_H

void
explain_buffer_floppy_fdc_state(explain_string_buffer_t *sb,
    const struct floppy_fdc_state *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ spec1 = ");
    explain_buffer_int(sb, data->spec1);
    if (data->spec2)
    {
        explain_string_buffer_puts(sb, ", spec2 = ");
        explain_buffer_int(sb, data->spec2);
    }
    if (data->dtr)
    {
        explain_string_buffer_puts(sb, ", dtr = ");
        explain_buffer_int(sb, data->dtr);
    }
    if (data->version)
    {
        explain_string_buffer_puts(sb, ", version = ");
        explain_buffer_uint(sb, data->version);
    }
    if (data->dor)
    {
        explain_string_buffer_puts(sb, ", dor = ");
        explain_buffer_uint(sb, data->dor);
    }
    if (data->address)
    {
        explain_string_buffer_puts(sb, ", address = ");
        explain_buffer_ulong(sb, data->address);
    }
    if (data->rawcmd)
    {
        explain_string_buffer_puts(sb, ", rawcmd = ");
        explain_buffer_uint(sb, data->rawcmd);
    }
    if (data->reset)
    {
        explain_string_buffer_puts(sb, ", reset = ");
        explain_buffer_uint(sb, data->reset);
    }
    if (data->need_configure)
    {
        explain_string_buffer_puts(sb, ", need_configure = ");
        explain_buffer_uint(sb, data->need_configure);
    }
    if (data->perp_mode)
    {
        explain_string_buffer_puts(sb, ", perp_mode = ");
        explain_buffer_uint(sb, data->perp_mode);
    }
    if (data->has_fifo)
    {
        explain_string_buffer_puts(sb, ", has_fifo = ");
        explain_buffer_uint(sb, data->has_fifo);
    }
    if (data->driver_version)
    {
        explain_string_buffer_puts(sb, ", driver_version = ");
        explain_buffer_uint(sb, data->driver_version);
    }
    explain_string_buffer_puts(sb, ", track = ");
    explain_buffer_char_data(sb, data->track, sizeof(data->track));
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_floppy_fdc_state(explain_string_buffer_t *sb,
    const struct floppy_fdc_state *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
