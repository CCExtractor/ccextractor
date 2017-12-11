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

#include <libexplain/buffer/int16_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/video_palette.h>
#include <libexplain/buffer/video_picture.h>
#include <libexplain/is_efault.h>

#ifdef VIDIOCGPICT


void
explain_buffer_video_picture(explain_string_buffer_t *sb,
    const struct video_picture *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ brightness = ");
    explain_buffer_uint16_t(sb, data->brightness);
    explain_string_buffer_puts(sb, ", hue = ");
    explain_buffer_uint16_t(sb, data->hue);
    explain_string_buffer_puts(sb, ", color = ");
    explain_buffer_uint16_t(sb, data->colour);
    explain_string_buffer_puts(sb, ", contrast = ");
    explain_buffer_uint16_t(sb, data->contrast);
    explain_string_buffer_puts(sb, ", whiteness = ");
    explain_buffer_uint16_t(sb, data->whiteness);
    explain_string_buffer_puts(sb, ", depth = ");
    explain_buffer_uint16_t(sb, data->depth);
    explain_string_buffer_puts(sb, ", palette = ");
    explain_buffer_video_palette(sb, data->palette);
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
