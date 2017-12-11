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

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/linux/videodev2.h>

#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_colorspace.h>
#include <libexplain/buffer/v4l2_field.h>
#include <libexplain/buffer/v4l2_pix_format.h>
#include <libexplain/buffer/v4l2_pixel_format.h>
#include <libexplain/option.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_pix_format(explain_string_buffer_t *sb,
    const struct v4l2_pix_format *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ width = ");
    explain_buffer_uint32_t(sb, data->width);
    explain_string_buffer_puts(sb, ", height = ");
    explain_buffer_uint32_t(sb, data->height);
    explain_string_buffer_puts(sb, ", pixelformat = ");
    explain_buffer_v4l2_pixel_format(sb, data->pixelformat);
    explain_string_buffer_puts(sb, ", field = ");
    explain_buffer_v4l2_field(sb, data->field);
    if (data->bytesperline)
    {
        explain_string_buffer_puts(sb, ", bytesperline = ");
        explain_buffer_int32_t(sb, data->bytesperline);
    }
    explain_string_buffer_puts(sb, ", sizeimage = ");
    explain_buffer_uint32_t(sb, data->sizeimage);
    explain_string_buffer_puts(sb, ", colorspace = ");
    explain_buffer_v4l2_colorspace(sb, data->colorspace);
    if (data->priv != 0)
    {
        explain_string_buffer_puts(sb, ", priv = ");
        explain_buffer_uint32_t(sb, data->priv);
    }
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
