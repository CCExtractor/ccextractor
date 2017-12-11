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

#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/v4l2_buf_type.h>
#include <libexplain/buffer/v4l2_fmtdesc.h>
#include <libexplain/buffer/v4l2_fmtdesc_flags.h>
#include <libexplain/buffer/v4l2_pixel_format.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_fmtdesc(explain_string_buffer_t *sb,
    const struct v4l2_fmtdesc *data, int extra)
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
        explain_string_buffer_puts(sb, ", flags = ");
        explain_buffer_v4l2_fmtdesc_flags(sb, data->flags);
        explain_string_buffer_puts(sb, ", description = ");
        explain_string_buffer_putsu_quoted_n(sb, data->description,
            sizeof(data->description));
        explain_string_buffer_puts(sb, ", pixelformat = ");
        explain_buffer_v4l2_pixel_format(sb, data->pixelformat);
        if (!explain_uint32_array_all_zero(data->reserved,
            SIZEOF(data->reserved)))
        {
            explain_string_buffer_puts(sb, ", reserved = ");
            explain_buffer_uint32_array(sb, data->reserved,
                SIZEOF(data->reserved));
        }
    }
    explain_string_buffer_puts(sb, " }");
}


int
explain_v4l2_fmtdesc_get_nformats(int fildes, int type)
{
    int lo = 0;
    int hi = 200;
    for (;;)
    {
        int mid = (lo + hi) / 2;
        struct v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc, 0, sizeof(fmtdesc));
        fmtdesc.index = mid;
        fmtdesc.type = type; /* NOT optional */
        if (ioctl(fildes, VIDIOC_ENUM_FMT, &fmtdesc) >= 0)
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

#endif

/* vim: set ts=8 sw=4 et : */
