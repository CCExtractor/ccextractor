/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011-2013 Peter Miller
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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/linux/videodev2.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/timeval.h>
#include <libexplain/buffer/v4l2_buf_flags.h>
#include <libexplain/buffer/v4l2_buf_type.h>
#include <libexplain/buffer/v4l2_buffer.h>
#include <libexplain/buffer/v4l2_field.h>
#include <libexplain/buffer/v4l2_memory.h>
#include <libexplain/buffer/v4l2_timecode.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_buffer(explain_string_buffer_t *sb,
    const struct v4l2_buffer *data, int extra)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ index = ");
    explain_buffer_uint32_t(sb, data->index);
    explain_string_buffer_puts(sb, ", type = ");
    explain_buffer_v4l2_buf_type(sb, data->type);
    if (extra)
    {
        explain_string_buffer_puts(sb, ", bytesused = ");
        explain_buffer_uint32_t(sb, data->bytesused);
        explain_string_buffer_puts(sb, ", flags = ");
        explain_buffer_v4l2_buf_flags(sb, data->flags);
        explain_string_buffer_puts(sb, ", field = ");
        explain_buffer_v4l2_field(sb, data->field);
        explain_string_buffer_puts(sb, ", timestamp = ");
        explain_buffer_timeval(sb, &data->timestamp);
        explain_string_buffer_puts(sb, ", timecode = ");
        explain_buffer_v4l2_timecode(sb, &data->timecode);
        explain_string_buffer_puts(sb, ", sequence = ");
        explain_buffer_uint32_t(sb, data->sequence);
    }
    explain_string_buffer_puts(sb, ", memory = ");
    explain_buffer_v4l2_memory(sb, data->memory);
    if (extra)
    {
        switch (data->memory)
        {
        case V4L2_MEMORY_MMAP:
            explain_string_buffer_puts(sb, ", m.offset = ");
            explain_buffer_uint32_t(sb, data->m.offset);
            break;

        case V4L2_MEMORY_USERPTR:
            explain_string_buffer_puts(sb, ", m.userptr = ");
            explain_buffer_ulong(sb, data->m.userptr);
            break;

        case V4L2_MEMORY_OVERLAY:
        default:
            break;
        }
        explain_string_buffer_puts(sb, ", length = ");
        explain_buffer_uint32_t(sb, data->length);
#ifdef LINUX_VIDEODEV2_H_struct_v4l2_buffer_input
        explain_string_buffer_puts(sb, ", input = ");
        explain_buffer_uint32_t(sb, data->input);
#endif
        if (data->reserved)
        {
            explain_string_buffer_puts(sb, ", reserved = ");
            explain_buffer_uint32_t(sb, data->reserved);
        }
    }
    explain_string_buffer_puts(sb, " }");
}


int
explain_v4l2_buffer_get_nframes(int fildes, int type)
{
    int lo = 0;
    int hi = 200;
    for (;;)
    {
        int mid = (lo + hi) / 2;
        struct v4l2_buffer qry;
        memset(&qry, 0, sizeof(qry));
        qry.index = mid;
        qry.type = type; /* NOT optional */
        if (ioctl(fildes, VIDIOC_QUERYBUF, &qry) >= 0)
        {
            if (hi <= 0 && lo <= 0)
                return -1;
            /* mid < nframes */
            lo = mid + 1;
            if (lo > hi)
                return lo;
        }
        else if (errno != EINVAL)
        {
            return -1;
        }
        else
        {
            if (hi <= 0 && lo <= 0)
                return -1;
            /* mid >= nframes */
            hi = mid - 1;
            if (lo >= hi)
                return lo;
        }
    }
}

#else

void
explain_buffer_v4l2_buffer(explain_string_buffer_t *sb,
    const struct v4l2_buffer *data, int extra)
{
    (void)extra;
    explain_buffer_pointer(sb, data);
}


int
explain_v4l2_buffer_get_nframes(int fildes, int type)
{
    return -1;
}

#endif

/* vim: set ts=8 sw=4 et : */
