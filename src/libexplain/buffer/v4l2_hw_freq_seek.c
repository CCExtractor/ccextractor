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
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/boolean.h>
#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/v4l2_hw_freq_seek.h>
#include <libexplain/buffer/v4l2_tuner_type.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_VIDEODEV2_H
#ifdef VIDIOC_S_HW_FREQ_SEEK

void
explain_buffer_v4l2_hw_freq_seek(explain_string_buffer_t *sb,
    const struct v4l2_hw_freq_seek *data, int fildes)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ tuner = ");
    explain_buffer_uint32_t(sb, data->tuner);

    {
        struct v4l2_tuner qry;

        memset(&qry, 0, sizeof(qry));
        qry.index = data->tuner;
        if (ioctl(fildes, VIDIOC_G_TUNER, &qry) >= 0 && qry.name[0])
        {
            explain_string_buffer_putc(sb, ' ');
            explain_string_buffer_putsu_quoted_n(sb, qry.name,
                sizeof(qry.name));
        }
    }

    explain_string_buffer_puts(sb, ", type = ");
    explain_buffer_v4l2_tuner_type(sb, data->type);
    explain_string_buffer_puts(sb, ", seek_upward = ");
    explain_buffer_boolean(sb, data->seek_upward);
    explain_string_buffer_puts(sb, ", wrap_around = ");
    explain_buffer_boolean(sb, data->wrap_around);
#ifdef HAVE_v4l2_hw_freq_seek_spacing
    if (data->spacing)
    {
        explain_string_buffer_puts(sb, ", spacing = ");
        explain_buffer_uint32_t(sb, data->spacing);
    }
#endif
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
