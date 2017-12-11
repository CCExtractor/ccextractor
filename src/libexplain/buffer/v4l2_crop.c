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

#include <libexplain/buffer/v4l2_buf_type.h>
#include <libexplain/buffer/v4l2_crop.h>
#include <libexplain/buffer/v4l2_rect.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_crop(explain_string_buffer_t *sb,
    const struct v4l2_crop *data, int extra)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_v4l2_buf_type(sb, data->type);
    if (extra)
    {
        explain_string_buffer_puts(sb, ", c = ");
        explain_buffer_v4l2_rect(sb, &data->c);
    }
    explain_string_buffer_puts(sb, " }");
}


enum explain_v4l2_crop_check_type_t
explain_v4l2_crop_check_type(int fildes, const struct v4l2_crop *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        return explain_v4l2_crop_check_type_no_idea;
    switch (explain_v4l2_buf_type_check(fildes, data->type))
    {
    case explain_v4l2_buf_type_check_no_idea:
    default:
        return explain_v4l2_crop_check_type_no_idea;

    case explain_v4l2_buf_type_check_notsup:
        return explain_v4l2_crop_check_type_notsup;

    case explain_v4l2_buf_type_check_nosuch:
        return explain_v4l2_crop_check_type_nosuch;

    case explain_v4l2_buf_type_check_ok:
        break;
    }
    return explain_v4l2_crop_check_type_ok;
}

#endif

/* vim: set ts=8 sw=4 et : */
