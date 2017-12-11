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

#include <libexplain/ac/linux/videodev.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/video_mbuf.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>

#ifdef VIDIOCGMBUF


void
explain_buffer_video_mbuf(explain_string_buffer_t *sb,
    const struct video_mbuf *data)
{
    int             len;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ size = ");
    explain_buffer_int(sb, data->size);
    explain_string_buffer_puts(sb, ", frames = ");
    explain_buffer_int(sb, data->frames);
    explain_string_buffer_puts(sb, ", offsets = ");
    len = data->frames;
    if (len < 0)
        len = 0;
    else if ((size_t)len > SIZEOF(data->offsets))
        len = SIZEOF(data->offsets);
    explain_buffer_int_array(sb, data->offsets, len);
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
