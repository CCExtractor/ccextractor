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

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/video_mmap.h>
#include <libexplain/buffer/video_palette.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOCMCAPTURE


void
explain_buffer_video_mmap(explain_string_buffer_t *sb,
    const struct video_mmap *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ frame = ");
    explain_buffer_uint(sb, data->frame);
    explain_string_buffer_puts(sb, ", width = ");
    explain_buffer_int(sb, data->width);
    explain_string_buffer_puts(sb, ", height = ");
    explain_buffer_int(sb, data->height);
    explain_string_buffer_puts(sb, ", format = ");
    explain_buffer_video_palette(sb, data->format);
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
