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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/linux/videodev2.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_buf_type.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_buf_type(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "V4L2_BUF_TYPE_VIDEO_CAPTURE", V4L2_BUF_TYPE_VIDEO_CAPTURE },
        { "V4L2_BUF_TYPE_VIDEO_OUTPUT", V4L2_BUF_TYPE_VIDEO_OUTPUT },
        { "V4L2_BUF_TYPE_VIDEO_OVERLAY", V4L2_BUF_TYPE_VIDEO_OVERLAY },
        { "V4L2_BUF_TYPE_VBI_CAPTURE", V4L2_BUF_TYPE_VBI_CAPTURE },
        { "V4L2_BUF_TYPE_VBI_OUTPUT", V4L2_BUF_TYPE_VBI_OUTPUT },
        { "V4L2_BUF_TYPE_SLICED_VBI_CAPTURE",
            V4L2_BUF_TYPE_SLICED_VBI_CAPTURE },
        { "V4L2_BUF_TYPE_SLICED_VBI_OUTPUT", V4L2_BUF_TYPE_SLICED_VBI_OUTPUT },
        { "V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY",
            V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY },
#ifdef V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
        { "V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE",
            V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE },
#endif
#ifdef V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
        { "V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE",
            V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE },
#endif
        { "V4L2_BUF_TYPE_PRIVATE", V4L2_BUF_TYPE_PRIVATE },
    };
    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_v4l2_buf_type_ptr(explain_string_buffer_t *sb, const int *value)
{
    if (explain_is_efault_pointer(value, sizeof(*value)))
    {
        explain_buffer_pointer(sb, value);
    }
    else
    {
        explain_string_buffer_puts(sb, "{ ");
        explain_buffer_v4l2_buf_type(sb, *value);
        explain_string_buffer_puts(sb, " }");
    }
}


enum explain_v4l2_buf_type_check_t
explain_v4l2_buf_type_check(int fildes, int data)
{
    struct v4l2_format dummy;

    memset(&dummy, 0, sizeof(dummy));
    dummy.type = data;
    if (ioctl(fildes, VIDIOC_G_FMT, &dummy) >= 0)
        return explain_v4l2_buf_type_check_ok;
    switch (errno)
    {
    case EINVAL:
        switch ((enum v4l2_buf_type)data)
        {
        case V4L2_BUF_TYPE_VIDEO_CAPTURE:
        case V4L2_BUF_TYPE_VIDEO_OVERLAY:
        case V4L2_BUF_TYPE_VIDEO_OUTPUT:
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
        case V4L2_BUF_TYPE_VBI_CAPTURE:
        case V4L2_BUF_TYPE_VBI_OUTPUT:
        case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
        case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
        case V4L2_BUF_TYPE_PRIVATE:
#ifdef V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
        case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
#endif
#ifdef V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
#endif
            return explain_v4l2_buf_type_check_notsup;

        default:
            return explain_v4l2_buf_type_check_nosuch;
        }

    default:
        switch ((enum v4l2_buf_type)data)
        {
        case V4L2_BUF_TYPE_VIDEO_CAPTURE:
        case V4L2_BUF_TYPE_VIDEO_OVERLAY:
        case V4L2_BUF_TYPE_VIDEO_OUTPUT:
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
        case V4L2_BUF_TYPE_VBI_CAPTURE:
        case V4L2_BUF_TYPE_VBI_OUTPUT:
        case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
        case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
        case V4L2_BUF_TYPE_PRIVATE:
#ifdef V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
        case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
#endif
#ifdef V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
#endif
            return explain_v4l2_buf_type_check_no_idea;

        default:
            return explain_v4l2_buf_type_check_nosuch;
        }
        break;
    }
}


enum explain_v4l2_buf_type_check_t
explain_v4l2_buf_type_ptr_check(int fildes, const int *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        return explain_v4l2_buf_type_check_no_idea;
    return explain_v4l2_buf_type_check(fildes, *data);
}

#endif


/* vim: set ts=8 sw=4 et : */
