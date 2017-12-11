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

#include <libexplain/ac/linux/videodev2.h>

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_sliced_vbi_service_set.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_sliced_vbi_service_set(explain_string_buffer_t *sb,
    int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "V4L2_SLICED_TELETEXT_B", V4L2_SLICED_TELETEXT_B },
        { "V4L2_SLICED_VPS", V4L2_SLICED_VPS },
        { "V4L2_SLICED_CAPTION_525", V4L2_SLICED_CAPTION_525 },
        { "V4L2_SLICED_WSS_625", V4L2_SLICED_WSS_625 },
        { "V4L2_SLICED_VBI_525", V4L2_SLICED_VBI_525 },
        { "V4L2_SLICED_VBI_625", V4L2_SLICED_VBI_625 },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_v4l2_sliced_vbi_service_set_array(explain_string_buffer_t *sb,
    const uint16_t *data, size_t data_size)
{
    size_t          j;

    if (explain_is_efault_pointer(data, data_size * sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    /* shorten the array on the right if there are zero values */
    while (data_size > 2 && data[data_size - 1] == 0)
        --data_size;

    explain_string_buffer_puts(sb, "{ ");
    for (j = 0; j < data_size; ++j)
    {
        if (j)
            explain_string_buffer_puts(sb, ", ");
        explain_buffer_v4l2_sliced_vbi_service_set(sb, data[j]);
    }
    explain_string_buffer_puts(sb, " }");
}


void
explain_buffer_v4l2_sliced_vbi_service_set_array2(explain_string_buffer_t *sb,
    const uint16_t *data, size_t dim1_size, size_t dim2_size)
{
    size_t          j;

    if (explain_is_efault_pointer(data, dim1_size * dim2_size * sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    for (j = 0; j < dim1_size; ++j)
    {
        if (j)
            explain_string_buffer_puts(sb, ", ");
        explain_buffer_v4l2_sliced_vbi_service_set_array(sb,
            data + j * dim2_size, dim2_size);
    }
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
