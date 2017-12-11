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

#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/v4l2_jpegcompression.h>
#include <libexplain/buffer/v4l2_jpeg_markers.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_jpegcompression(explain_string_buffer_t *sb,
    const struct v4l2_jpegcompression *data)
{
    int             size;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ quality = ");
    explain_buffer_int(sb, data->quality);
    explain_string_buffer_puts(sb, ", APPn = ");
    explain_buffer_int(sb, data->APPn);
    explain_string_buffer_puts(sb, ", APP_len = ");
    explain_buffer_int(sb, data->APP_len);

    size = data->APP_len;
    if (size < 0)
        size = 0;
    else if ((unsigned)size > sizeof(data->APP_data))
        size = sizeof(data->APP_data);
    if (size > 0)
    {
        explain_string_buffer_puts(sb, ", APP_data = ");
        explain_buffer_mostly_text(sb, data->APP_data, size);
    }

    explain_string_buffer_puts(sb, ", COM_len = ");
    explain_buffer_int(sb, data->COM_len);

    size = data->COM_len;
    if (size < 0)
        size = 0;
    else if ((unsigned)size > sizeof(data->COM_data))
        size = sizeof(data->COM_data);
    if (size > 0)
    {
        explain_string_buffer_puts(sb, ", COM_data = ");
        explain_buffer_mostly_text(sb, data->COM_data, size);
    }

    explain_string_buffer_puts(sb, ", jpeg_markers = ");
    explain_buffer_v4l2_jpeg_markers(sb, data->jpeg_markers);
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
