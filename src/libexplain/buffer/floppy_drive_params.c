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

#include <libexplain/buffer/floppy_drive_params.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_FD_H

static void
explain_buffer_floppy_drive_params_flags(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "FTD_MSG", FTD_MSG },
        { "FD_BROKEN_DCL", FD_BROKEN_DCL },
        { "FD_DEBUG", FD_DEBUG },
        { "FD_SILENT_DCL_CLEAR", FD_SILENT_DCL_CLEAR },
        { "FD_INVERTED_DCL", FD_INVERTED_DCL },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_floppy_drive_params(explain_string_buffer_t *sb,
    const struct floppy_drive_params *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ cmos = ");
    explain_buffer_int(sb, data->cmos);
    if (data->max_dtr)
    {
        explain_string_buffer_puts(sb, ", max_dtr = ");
        explain_buffer_ulong(sb, data->max_dtr);
    }
    if (data->hlt)
    {
        explain_string_buffer_puts(sb, ", hlt = ");
        explain_buffer_ulong(sb, data->hlt);
    }
    if (data->hut)
    {
        explain_string_buffer_puts(sb, ", hut = ");
        explain_buffer_ulong(sb, data->hut);
    }
    if (data->srt)
    {
        explain_string_buffer_puts(sb, ", srt = ");
        explain_buffer_ulong(sb, data->srt);
    }
    if (data->spinup)
    {
        explain_string_buffer_puts(sb, ", spinup = ");
        explain_buffer_ulong(sb, data->spinup);
    }
    if (data->spindown)
    {
        explain_string_buffer_puts(sb, ", spindown = ");
        explain_buffer_ulong(sb, data->spindown);
    }
    if (data->spindown_offset)
    {
        explain_string_buffer_puts(sb, ", spindown_offset = ");
        explain_buffer_ulong(sb, data->spindown_offset);
    }
    if (data->select_delay)
    {
        explain_string_buffer_puts(sb, ", select_delay = ");
        explain_buffer_int(sb, data->select_delay);
    }
    if (data->rps)
    {
        explain_string_buffer_puts(sb, ", rps = ");
        explain_buffer_int(sb, data->rps);
    }
    if (data->tracks)
    {
        explain_string_buffer_puts(sb, ", tracks = ");
        explain_buffer_int(sb, data->tracks);
    }
    if (data->timeout)
    {
        explain_string_buffer_puts(sb, ", timeout = ");
        explain_buffer_ulong(sb, data->timeout);
    }
    if (data->interleave_sect)
    {
        explain_string_buffer_puts(sb, ", interleave_sect = ");
        explain_buffer_int(sb, data->interleave_sect);
    }
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_floppy_drive_params_flags(sb, data->flags);
    if (data->read_track)
    {
        explain_string_buffer_puts(sb, ", read_track = ");
        explain_buffer_int(sb, data->read_track);
    }
#if 0
    short autodetect[8];
#endif
    if (data->checkfreq)
    {
        explain_string_buffer_puts(sb, ", checkfreq = ");
        explain_buffer_int(sb, data->checkfreq);
    }
    if (data->native_format)
    {
        explain_string_buffer_puts(sb, ", native_format = ");
        explain_buffer_int(sb, data->native_format);
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_floppy_drive_params(explain_string_buffer_t *sb,
    const struct floppy_drive_params *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
