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

#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/timespec.h>
#include <libexplain/buffer/v4l2_event.h>
#include <libexplain/buffer/v4l2_event_type.h>
#include <libexplain/buffer/v4l2_event_vsync.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_VIDEODEV2_H
#ifdef VIDIOC_DQEVENT


void
explain_buffer_v4l2_event(explain_string_buffer_t *sb,
    const struct v4l2_event *data, int extra)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_v4l2_event_type(sb, data->type);
    if (extra)
    {
        switch (data->type)
        {
        case V4L2_EVENT_VSYNC:
            explain_string_buffer_puts(sb, ", u.vsync = ");
            explain_buffer_v4l2_event_vsync(sb, &data->u.vsync);
            break;

        case V4L2_EVENT_PRIVATE_START:
            explain_string_buffer_puts(sb, ", u.data = ");
            explain_buffer_mostly_text(sb, data->u.data, sizeof(data->u.data));
            break;

        default:
            break;
        }
        explain_string_buffer_puts(sb, ", pending = ");
        explain_buffer_uint32_t(sb, data->pending);
        explain_string_buffer_puts(sb, ", sequence = ");
        explain_buffer_uint32_t(sb, data->sequence);
        explain_string_buffer_puts(sb, ", timestamp = ");
        explain_buffer_timespec(sb, &data->timestamp);
    }
    if (!explain_uint32_array_all_zero(data->reserved, SIZEOF(data->reserved)))
    {
        explain_string_buffer_puts(sb, ", reserved = ");
        explain_buffer_uint32_array(sb, data->reserved, SIZEOF(data->reserved));
    }
    explain_string_buffer_puts(sb, " }");
}

#endif
#endif

/* vim: set ts=8 sw=4 et : */
