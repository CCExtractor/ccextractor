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
#include <libexplain/buffer/v4l2_buf_type.h>
#include <libexplain/buffer/v4l2_format.h>
#include <libexplain/buffer/v4l2_pix_format.h>
#include <libexplain/buffer/v4l2_sliced_vbi_format.h>
#include <libexplain/buffer/v4l2_vbi_format.h>
#include <libexplain/buffer/v4l2_window.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_format(explain_string_buffer_t *sb,
    const struct v4l2_format *data, int extra)
{
#ifdef HAVE_LINUX_VIDEODEV2_H
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_v4l2_buf_type(sb, data->type);
    if (extra)
    {
        switch (data->type)
        {
        case V4L2_BUF_TYPE_VIDEO_CAPTURE:
            explain_string_buffer_puts(sb, ", fmt.pix = ");
            explain_buffer_v4l2_pix_format(sb, &data->fmt.pix);
            break;

        case V4L2_BUF_TYPE_VIDEO_OVERLAY:
            explain_string_buffer_puts(sb, ", fmt.win = ");
            explain_buffer_v4l2_window(sb, &data->fmt.win);
            break;

        case V4L2_BUF_TYPE_VBI_CAPTURE:
            explain_string_buffer_puts(sb, ", fmt.vbi = ");
            explain_buffer_v4l2_vbi_format(sb, &data->fmt.vbi);
            break;

        case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
            explain_string_buffer_puts(sb, ", fmt.sliced = ");
            explain_buffer_v4l2_sliced_vbi_format(sb, &data->fmt.sliced);
            break;

        case V4L2_BUF_TYPE_VIDEO_OUTPUT:
        case V4L2_BUF_TYPE_VBI_OUTPUT:
        case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
        case V4L2_BUF_TYPE_PRIVATE:
#ifdef V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
        case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
#endif
#ifdef V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
#endif
        default:
            break;
        }
    }
    explain_string_buffer_puts(sb, " }");
#else
    explain_buffer_pointer(sb, data);
#endif
}


enum explain_v4l2_format_check_type_t
explain_v4l2_format_check_type(int fildes, const struct v4l2_format *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        return explain_v4l2_format_check_type_no_idea;
    switch (explain_v4l2_buf_type_check(fildes, data->type))
    {
    case explain_v4l2_buf_type_check_no_idea:
    default:
        return explain_v4l2_format_check_type_no_idea;

    case explain_v4l2_buf_type_check_notsup:
        return explain_v4l2_format_check_type_notsup;

    case explain_v4l2_buf_type_check_nosuch:
        return explain_v4l2_format_check_type_nosuch;

    case explain_v4l2_buf_type_check_ok:
        break;
    }
    return explain_v4l2_format_check_type_ok;
}

#endif

/* vim: set ts=8 sw=4 et : */
