/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/ac/linux/videodev.h>

#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/vbi_format.h>
#include <libexplain/buffer/video_palette.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>


#ifdef VIDIOCGVBIFMT

static void
explain_buffer_vbi_format_flags(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "VBI_UNSYNC", VBI_UNSYNC },
        { "VBI_INTERLACED", VBI_INTERLACED },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_vbi_format(explain_string_buffer_t *sb,
    const struct vbi_format *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ sampling_rate = ");
    explain_buffer_uint32_t(sb, data->sampling_rate);
    explain_string_buffer_puts(sb, ", samples_per_line = ");
    explain_buffer_uint32_t(sb, data->samples_per_line);
    explain_string_buffer_puts(sb, ", sample_format = ");
    explain_buffer_video_palette(sb, data->sample_format);
    explain_string_buffer_puts(sb, ", start = ");
    explain_buffer_int32_array(sb, data->start, SIZEOF(data->start));
    explain_string_buffer_puts(sb, ", count = ");
    explain_buffer_uint32_array(sb, data->count, SIZEOF(data->count));
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_vbi_format_flags(sb, data->flags);
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
