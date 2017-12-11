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

#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_timecode.h>
#include <libexplain/buffer/v4l2_timecode_flags.h>
#include <libexplain/buffer/v4l2_timecode_type.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

static int
all_zero(const unsigned char *data, size_t data_size)
{
    while (data_size > 0)
    {
        if (*data)
            return 0;
        ++data;
        --data_size;
    }
    return 1;
}


void
explain_buffer_v4l2_timecode(explain_string_buffer_t *sb,
    const struct v4l2_timecode *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_v4l2_timecode_type(sb, data->type);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_v4l2_timecode_flags(sb, data->flags);
    explain_string_buffer_puts(sb, ", frames = ");
    explain_buffer_int(sb, data->frames);
    explain_string_buffer_puts(sb, ", seconds = ");
    explain_buffer_int(sb, data->seconds);
    explain_string_buffer_puts(sb, ", minutes = ");
    explain_buffer_int(sb, data->minutes);
    explain_string_buffer_puts(sb, ", hours = ");
    explain_buffer_int(sb, data->hours);
    if (!all_zero(data->userbits, SIZEOF(data->userbits)))
    {
        explain_string_buffer_puts(sb, ", userbits = ");
        explain_buffer_mostly_text(sb, data->userbits, sizeof(data->userbits));
    }
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
