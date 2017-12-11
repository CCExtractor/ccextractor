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

#include <libexplain/buffer/floppy_struct.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_FD_H

void
explain_buffer_floppy_struct(explain_string_buffer_t *sb,
    const struct floppy_struct *data)
{
    unsigned int    n;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ size = ");
    explain_buffer_uint(sb, data->size);
    explain_string_buffer_puts(sb, ", sect = ");
    explain_buffer_uint(sb, data->sect);
    explain_string_buffer_puts(sb, ", head = ");
    explain_buffer_uint(sb, data->head);
    explain_string_buffer_puts(sb, ", track = ");
    explain_buffer_uint(sb, data->track);
    explain_string_buffer_puts(sb, ", stretch = ");

    /*
     * bit 0 !=0 means double track steps
     * bit 1 != 0 means swap sides
     * bits 2..9 give the first sector
     *   number (the LSB is flipped)
     */
    n = data->stretch;
    if (n & FD_STRETCH)
    {
        explain_string_buffer_puts(sb, "FD_STRETCH | ");
        n &= ~FD_STRETCH;
    }
    if (n & FD_SWAPSIDES)
    {
        explain_string_buffer_puts(sb, "FD_SWAPSIDES | ");
        n &= ~FD_SWAPSIDES;
    }
#ifdef FD_SECTBASEMASK
    explain_string_buffer_printf(sb, "FD_MKSECTBASE(%d)", FD_SECTBASE(data));
    n &= ~FD_SECTBASEMASK;
#endif
    if (n)
        explain_string_buffer_printf(sb, " | 0x%X", n);

    explain_string_buffer_puts(sb, ", gap = ");
    explain_buffer_int(sb, data->gap);
    explain_string_buffer_puts(sb, ", rate = ");

    n = data->rate;
    if (n & FD_PERP)
    {
        explain_string_buffer_puts(sb, "FD_PERP | ");
        n &= ~FD_PERP;
    }
    if (n & FD_2M)
    {
        explain_string_buffer_puts(sb, "FD_2M | ");
        n &= ~FD_2M;
    }
    explain_buffer_int(sb, n & FD_SIZECODEMASK);
    n &= ~FD_SIZECODEMASK;
    if (n)
        explain_string_buffer_printf(sb, " | 0x%X", n);

    explain_string_buffer_puts(sb, ", spec1 = ");
    explain_buffer_int(sb, data->spec1);
    explain_string_buffer_puts(sb, ", fmt_gap = ");
    explain_buffer_int(sb, data->fmt_gap);
    explain_string_buffer_puts(sb, ", name = ");
    explain_buffer_pathname(sb, data->name);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_floppy_struct(explain_string_buffer_t *sb,
    const struct floppy_struct *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
