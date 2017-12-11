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

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_clip.h>
#include <libexplain/buffer/v4l2_field.h>
#include <libexplain/buffer/v4l2_rect.h>
#include <libexplain/buffer/v4l2_window.h>
#include <libexplain/is_efault.h>

#ifdef HAVE_LINUX_VIDEODEV2_H


void
explain_buffer_v4l2_window(explain_string_buffer_t *sb,
    const struct v4l2_window *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ w = ");
    explain_buffer_v4l2_rect(sb, &data->w);
    explain_string_buffer_puts(sb, ", field = ");
    explain_buffer_v4l2_field(sb, data->field);
    explain_string_buffer_puts(sb, ", chromakey = ");
    explain_buffer_uint32_t(sb, data->chromakey);
    explain_string_buffer_puts(sb, ", clips = ");
    explain_buffer_v4l2_clip_list(sb, data->clips, data->clipcount);
    explain_string_buffer_puts(sb, ", clipcount = ");
    explain_buffer_uint32_t(sb, data->clipcount);
    explain_string_buffer_puts(sb, ", bitmap = ");
    explain_buffer_pointer(sb, data->bitmap);
    explain_string_buffer_puts(sb, ", global_alpha = ");
    explain_buffer_int(sb, data->global_alpha);
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
