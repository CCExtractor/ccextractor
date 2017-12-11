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
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/video_capability.h>
#include <libexplain/buffer/video_capability_type.h>
#include <libexplain/is_efault.h>


#ifdef VIDIOCGCAP

void
explain_buffer_video_capability(explain_string_buffer_t *sb,
    const struct video_capability *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ name = ");
    explain_string_buffer_puts_quoted_n(sb, data->name, sizeof(data->name));
    explain_string_buffer_puts(sb, ", type = ");
    explain_buffer_video_capability_type(sb, data->type);
    explain_string_buffer_puts(sb, ", channels = ");
    explain_buffer_int(sb, data->channels);
    explain_string_buffer_puts(sb, ", audios = ");
    explain_buffer_int(sb, data->audios);
    explain_string_buffer_puts(sb, ", maxwidth = ");
    explain_buffer_int(sb, data->maxwidth);
    explain_string_buffer_puts(sb, ", maxheight = ");
    explain_buffer_int(sb, data->maxheight);
    explain_string_buffer_puts(sb, ", minwidth = ");
    explain_buffer_int(sb, data->minwidth);
    explain_string_buffer_puts(sb, ", minheight = ");
    explain_buffer_int(sb, data->minheight);
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
