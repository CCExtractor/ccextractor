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
#include <libexplain/ac/linux/videodev2.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/boolean.h>
#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/int64_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_control.h>
#include <libexplain/buffer/v4l2_control_id.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_control(explain_string_buffer_t *sb,
    const struct v4l2_control *data, int extra, int fildes)
{
#ifdef HAVE_LINUX_VIDEODEV2_H
    struct v4l2_queryctrl info;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ id = ");
    explain_buffer_v4l2_control_id(sb, data->id);

    /*
     * If possible, supplement with the name of the control.
     */
    memset(&info, 0, sizeof(info));
    info.id = data->id;
    if (ioctl(fildes, VIDIOC_QUERYCTRL, &info) < 0)
        info.type = V4L2_CTRL_TYPE_INTEGER;
    if (info.name[0])
    {
        explain_string_buffer_putc(sb, ' ');
        explain_string_buffer_putsu_quoted_n(sb, info.name, sizeof(info.name));
    }

    if (extra)
    {
        explain_string_buffer_puts(sb, ", value = ");

        /*
         * If possible, supplement with the name of the value.
         */
        switch (info.type)
        {
        case V4L2_CTRL_TYPE_INTEGER64:
            {
                struct v4l2_ext_control c;
                struct v4l2_ext_controls d;

                memset(&c, 0, sizeof(c));
                c.id = data->id;

                memset(&d, 0, sizeof(d));
                d.ctrl_class = V4L2_CTRL_ID2CLASS(data->id);
                d.count = 1;
                d.controls = &c;
                if (ioctl(fildes, VIDIOC_G_EXT_CTRLS, &d) >= 0)
                    explain_buffer_uint64_t(sb, c.value64);
            }
            break;

        case V4L2_CTRL_TYPE_CTRL_CLASS:
            /* Huh?!? */
            explain_buffer_int32_t(sb, data->value);
            break;

#if defined(V4L2_CTRL_TYPE_STRING) || defined(HAVE_V4L2_CTRL_TYPE_STRING)
        case V4L2_CTRL_TYPE_STRING:
            {
                struct v4l2_ext_control c;
                struct v4l2_ext_controls d;
                char            string[1000];

                memset(&c, 0, sizeof(c));
                c.id = data->id;
                c.size = sizeof(string);
                c.string = string;

                memset(&d, 0, sizeof(d));
                d.ctrl_class = V4L2_CTRL_ID2CLASS(data->id);
                d.count = 1;
                d.controls = &c;
                if (ioctl(fildes, VIDIOC_G_EXT_CTRLS, &d) >= 0)
                {
                    explain_string_buffer_puts_quoted_n
                    (
                        sb,
                        string,
                        sizeof(string)
                    );
                }
            }
            break;
#endif

        default:
        case V4L2_CTRL_TYPE_INTEGER:
        case V4L2_CTRL_TYPE_BUTTON:
#if defined(V4L2_CTRL_TYPE_BITMASK) || defined(HAVE_V4L2_CTRL_TYPE_BITMASK)
        case V4L2_CTRL_TYPE_BITMASK:
#endif
            explain_buffer_int32_t(sb, data->value);
            break;

        case V4L2_CTRL_TYPE_BOOLEAN:
            explain_buffer_boolean(sb, data->value);
            break;

        case V4L2_CTRL_TYPE_MENU:
            {
                struct v4l2_querymenu value;

                explain_buffer_int32_t(sb, data->value);

                memset(&value, 0, sizeof(value));
                value.id = data->id;
                value.index = data->value;
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
    }
    explain_string_buffer_puts(sb, " }");
#else
    explain_buffer_pointer(sb, data);
#endif
}


enum explain_v4l2_control_check_id_t
explain_v4l2_control_check_id(int fildes, const struct v4l2_control *data)
{
    struct v4l2_queryctrl dummy;

    if (explain_is_efault_pointer(data, sizeof(*data)))
        return explain_v4l2_control_check_id_no_idea;
    memset(&dummy, 0, sizeof(dummy));
    dummy.id = data->id;
    if (ioctl(fildes, VIDIOC_QUERYCTRL, &dummy) >= 0)
        return explain_v4l2_control_check_id_ok;
    switch (errno)
    {
    case EIO:
    case EINVAL:
        switch (V4L2_CTRL_ID2CLASS(data->id))
        {
        case V4L2_CTRL_CLASS_USER:
        case V4L2_CTRL_CLASS_MPEG:
#ifdef V4L2_CTRL_CLASS_CAMERA
        case V4L2_CTRL_CLASS_CAMERA:
#endif
#ifdef V4L2_CTRL_CLASS_FM_TX
        case V4L2_CTRL_CLASS_FM_TX:
#endif
            return explain_v4l2_control_check_id_notsup;

        default:
            return explain_v4l2_control_check_id_nosuch;
        }

    default:
        return explain_v4l2_control_check_id_no_idea;
    }
}

/* vim: set ts=8 sw=4 et : */
#endif
