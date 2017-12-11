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
#include <libexplain/buffer/v4l2_dv_enum_preset.h>
#include <libexplain/buffer/v4l2_dv_preset_value.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_VIDEODEV2_H
#ifdef VIDIOC_ENUM_DV_PRESETS

void
explain_buffer_v4l2_dv_enum_preset(explain_string_buffer_t *sb,
    const struct v4l2_dv_enum_preset *data, int extra)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ index = ");
    explain_buffer_uint32_t(sb, data->index);
    if (extra)
    {
        explain_string_buffer_puts(sb, ", preset = ");
        explain_buffer_v4l2_dv_preset_value(sb, data->preset);
        explain_string_buffer_puts(sb, ", name = ");
        explain_string_buffer_putsu_quoted_n(sb, data->name,
            sizeof(data->name));
        explain_string_buffer_puts(sb, ", width = ");
        explain_buffer_uint32_t(sb, data->width);
        explain_string_buffer_puts(sb, ", height = ");
        explain_buffer_uint32_t(sb, data->height);
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
explain_v4l2_dv_enum_preset_get_n(int fildes)
{
    int lo = 0;
    int hi = 200;
    for (;;)
    {
        struct v4l2_dv_enum_preset qry;
        int             mid;

        mid = (lo + hi) / 2;
        memset(&qry, 0, sizeof(qry));
        qry.index = mid;
        if (ioctl(fildes, VIDIOC_ENUM_DV_PRESETS, &qry) >= 0)
        {
            if (hi <= 0 && lo <= 0)
                return -1;
            /* mid < n */
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
            /* mid >= n */
            hi = mid - 1;
            if (lo >= hi)
                return lo;
        }
    }
}

#endif
#endif

/* vim: set ts=8 sw=4 et : */
