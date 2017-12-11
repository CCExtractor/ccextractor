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

#include <libexplain/ac/linux/fiemap.h>

#include <libexplain/buffer/fiemap.h>
#include <libexplain/buffer/int64_t.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>

#ifdef HAVE_LINUX_FIEMAP_H


static void
explain_buffer_fiemap_flags(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "FIEMAP_FLAG_SYNC", FIEMAP_FLAG_SYNC },
        { "FIEMAP_FLAG_XATTR", FIEMAP_FLAG_XATTR },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_fiemap(explain_string_buffer_t *sb,
    const struct fiemap *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    /*
     * We only print the (in) fields.
     */
    explain_string_buffer_puts(sb, "{ fm_start = ");
    explain_buffer_uint64_t(sb, data->fm_start);
    explain_string_buffer_puts(sb, ", fm_length = ");
    explain_buffer_uint64_t(sb, data->fm_length);
    explain_string_buffer_puts(sb, ", fm_flags = ");
    explain_buffer_fiemap_flags(sb, data->fm_flags);
    explain_string_buffer_puts(sb, ", fm_extent_count = ");
    explain_buffer_ulong(sb, data->fm_extent_count);
    explain_string_buffer_puts(sb, " }");
}


#else


void
explain_buffer_fiemap(explain_string_buffer_t *sb,
    const struct fiemap *data)
{
    explain_buffer_pointer(sb, data);
}


#endif


/* vim: set ts=8 sw=4 et : */
