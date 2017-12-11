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
#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/video_clip.h>
#include <libexplain/buffer/video_window.h>
#include <libexplain/buffer/video_window_flags.h>
#include <libexplain/is_efault.h>


#if defined(VIDIOCGWIN) || defined(VIDIOCSWIN)

void
explain_buffer_video_window(explain_string_buffer_t *sb,
    const struct video_window *data, int extra)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    if (data->x || data->y)
    {
        explain_string_buffer_puts(sb, "x = ");
        explain_buffer_uint32_t(sb, data->x);
        explain_string_buffer_puts(sb, ", y = ");
        explain_buffer_uint32_t(sb, data->y);
        explain_string_buffer_puts(sb, ", ");
    }
    explain_string_buffer_puts(sb, "width = ");
    explain_buffer_uint32_t(sb, data->width);
    explain_string_buffer_puts(sb, ", height = ");
    explain_buffer_uint32_t(sb, data->height);
    if (data->flags & VIDEO_WINDOW_CHROMAKEY)
    {
        explain_string_buffer_puts(sb, ", chromakey = ");
        explain_buffer_uint32_t(sb, data->chromakey);
    }
    if (data->flags)
    {
        explain_string_buffer_puts(sb, ", flags = ");
        explain_buffer_video_window_flags(sb, data->flags);
    }
    if (extra)
    {
        int             len;

        /* VIDIOCSWIN only */
        len = data->clipcount;
        if (len < 0)
            len = 0;
        if (len > 0)
        {
            explain_string_buffer_puts(sb, ", clips = ");
            explain_buffer_video_clip_array(sb, data->clips, len);
            explain_string_buffer_puts(sb, ", clipcount = ");
            explain_buffer_int(sb, data->clipcount);
        }
    }
    explain_string_buffer_puts(sb, " }");
}

#endif


/* vim: set ts=8 sw=4 et : */
