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

#include <libexplain/ac/linux/videodev2.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/boolean.h>
#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/v4l2_control_id.h>
#include <libexplain/buffer/v4l2_ctrl_type.h>
#include <libexplain/buffer/v4l2_queryctrl.h>
#include <libexplain/buffer/v4l2_queryctrl_flags.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_queryctrl(explain_string_buffer_t *sb,
    const struct v4l2_queryctrl *data, int extra, int fildes)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ id = ");
    explain_buffer_v4l2_control_id(sb, data->id);
    if (extra)
    {
        explain_string_buffer_puts(sb, ", type = ");
        explain_buffer_v4l2_ctrl_type(sb, data->type);
        explain_string_buffer_puts(sb, ", name = ");
        explain_string_buffer_putsu_quoted_n
        (
            sb,
            data->name,
            sizeof(data->name)
        );
        explain_string_buffer_puts(sb, ", minimum = ");
        explain_buffer_int32_t(sb, data->minimum);
        explain_string_buffer_puts(sb, ", maximum = ");
        explain_buffer_int32_t(sb, data->maximum);
        explain_string_buffer_puts(sb, ", step = ");
        explain_buffer_int32_t(sb, data->step);
        explain_string_buffer_puts(sb, ", default_value = ");
        switch (data->type)
        {
        case V4L2_CTRL_TYPE_INTEGER64:
        case V4L2_CTRL_TYPE_CTRL_CLASS:
#if defined(V4L2_CTRL_TYPE_STRING) || defined(HAVE_V4L2_CTRL_TYPE_STRING)
        case V4L2_CTRL_TYPE_STRING:
#endif
#if defined(V4L2_CTRL_TYPE_BITMASK) || defined(HAVE_V4L2_CTRL_TYPE_BITMASK)
        case V4L2_CTRL_TYPE_BITMASK:
#endif
            /*
             * In theory, these types only occur for v4l2_ext_control
             * values.  In practice, they better be right.
             */

        case V4L2_CTRL_TYPE_INTEGER:
        case V4L2_CTRL_TYPE_BUTTON:
        default:
            explain_buffer_int32_t(sb, data->default_value);
            break;

        case V4L2_CTRL_TYPE_BOOLEAN:
            explain_buffer_boolean(sb, data->default_value);
            break;

        case V4L2_CTRL_TYPE_MENU:
            {
                struct v4l2_querymenu value;

                explain_buffer_int32_t(sb, data->default_value);

                memset(&value, 0, sizeof(value));
                value.id = data->id;
                value.index = data->default_value;
                if
                (
                    ioctl(fildes, VIDIOC_QUERYMENU, &value) >= 0
                &&
                    value.name[0]
                )
                {
                    explain_string_buffer_putc(sb, ' ');
                    explain_string_buffer_putsu_quoted_n
                    (
                        sb,
                        value.name,
                        sizeof(value.name)
                    );
                }
            }
            break;
        }
        explain_string_buffer_puts(sb, ", flags = ");
        explain_buffer_v4l2_queryctrl_flags(sb, data->flags);
    }
    explain_string_buffer_puts(sb, " }");
}


enum explain_v4l2_queryctrl_check_id_t
explain_v4l2_queryctrl_check_id(int fildes, const struct v4l2_queryctrl *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        return explain_v4l2_queryctrl_check_id_no_idea;
    switch (explain_v4l2_control_id_check(fildes, data->id))
    {
    case explain_v4l2_control_id_check_no_idea:
    default:
        return explain_v4l2_queryctrl_check_id_no_idea;

    case explain_v4l2_control_id_check_nosuch:
        return explain_v4l2_queryctrl_check_id_nosuch;

    case explain_v4l2_control_id_check_notsup:
        return explain_v4l2_queryctrl_check_id_notsup;

    case explain_v4l2_control_id_check_ok:
        return explain_v4l2_queryctrl_check_id_ok;
    }
}

/* vim: set ts=8 sw=4 et : */
#endif
