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

#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_ctrl_class.h>
#include <libexplain/buffer/v4l2_ext_control.h>
#include <libexplain/buffer/v4l2_ext_controls.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_ext_controls(explain_string_buffer_t *sb,
    const struct v4l2_ext_controls *data, int extra, int fildes)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ctrl_class = ");
    explain_buffer_v4l2_ctrl_class(sb, data->ctrl_class);
    explain_string_buffer_puts(sb, ", count = ");
    explain_buffer_uint32_t(sb, data->count);
    if (extra)
    {
        explain_string_buffer_puts(sb, ", error_idx = ");
        explain_buffer_uint32_t(sb, data->error_idx);
        if (!explain_uint32_array_all_zero(data->reserved,
            SIZEOF(data->reserved)))
        {
            explain_string_buffer_puts(sb, ", reserved = ");
            explain_buffer_uint32_array(sb, data->reserved,
                SIZEOF(data->reserved));
        }
        if (data->count > 0)
        {
            explain_string_buffer_puts(sb, ", controls = ");
            explain_buffer_v4l2_ext_control_array
            (
                sb,
                data->controls,
                data->count,
                fildes
            );
        }
    }
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
