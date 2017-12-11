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
#include <libexplain/buffer/int64_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_control_id.h>
#include <libexplain/buffer/v4l2_ext_control.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_ext_control(explain_string_buffer_t *sb,
    const struct v4l2_ext_control *data, int fildes)
{
    struct v4l2_queryctrl qry;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ id = ");
    explain_buffer_v4l2_control_id(sb, data->id);

    if (!explain_uint32_array_all_zero(data->reserved2,
        SIZEOF(data->reserved2)))
    {
        explain_string_buffer_puts(sb, ", reserved = ");
        explain_buffer_uint32_array(sb, data->reserved2,
            SIZEOF(data->reserved2));
    }

    memset(&qry, 0, sizeof(qry));
    qry.id = data->id;
    if (ioctl(fildes, VIDIOC_QUERYCTRL, &qry) >= 0)
    {
        switch (qry.type)
        {
        case V4L2_CTRL_TYPE_INTEGER:
        case V4L2_CTRL_TYPE_BUTTON:
#if defined(V4L2_CTRL_TYPE_BITMASK) || defined(HAVE_V4L2_CTRL_TYPE_BITMASK)
        case V4L2_CTRL_TYPE_BITMASK:
#endif
            explain_buffer_uint32_t(sb, data->value);
            break;

        case V4L2_CTRL_TYPE_INTEGER64:
            explain_buffer_uint64_t(sb, data->value64);
            break;

        case V4L2_CTRL_TYPE_CTRL_CLASS:
            /* Huh?!? */
            break;

#if defined(V4L2_CTRL_TYPE_STRING) || defined(HAVE_V4L2_CTRL_TYPE_STRING)
        case V4L2_CTRL_TYPE_STRING:
            explain_string_buffer_puts(sb, ", size = ");
            explain_buffer_uint32_t(sb, data->size);
            if (explain_is_efault_string(data->string))
                explain_buffer_pointer(sb, data->string);
            else
                explain_string_buffer_puts_quoted(sb, data->string);
            break;
#endif

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

        default:
            /* Maybe this code is out of date? */
            break;
        }
    }
    explain_string_buffer_puts(sb, " }");
}


void
explain_buffer_v4l2_ext_control_array(explain_string_buffer_t *sb,
    const struct v4l2_ext_control *data, unsigned data_size, int fildes)
{
    size_t          j;

    if (explain_is_efault_pointer(data, data_size * sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    for (j = 0; j < data_size; ++j)
    {
        if (j)
            explain_string_buffer_puts(sb, ", ");
        explain_buffer_v4l2_ext_control(sb, data + j, fildes);
    }
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
