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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/linux/videodev.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/int16_t.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/video_tuner.h>
#include <libexplain/buffer/video_tuner_flags.h>
#include <libexplain/buffer/video_tuner_mode.h>
#include <libexplain/is_efault.h>


#ifdef VIDIOCGTUNER

void
explain_buffer_video_tuner(explain_string_buffer_t *sb,
    const struct video_tuner *data, int extra)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ tuner = ");
    explain_buffer_int(sb, data->tuner);
    if (extra)
    {
        explain_string_buffer_puts(sb, ", name = ");
        explain_string_buffer_puts_quoted_n(sb, data->name, sizeof(data->name));
        explain_string_buffer_puts(sb, ", rangelow = ");
        explain_buffer_ulong(sb, data-> rangelow);
        explain_string_buffer_puts(sb, ", rangehigh = ");
        explain_buffer_ulong(sb, data-> rangehigh);
        explain_string_buffer_puts(sb, ", flags = ");
        explain_buffer_video_tuner_flags(sb, data->flags);
        explain_string_buffer_puts(sb, ", mode = ");
        explain_buffer_video_tuner_mode(sb, data->mode);
        explain_string_buffer_puts(sb, ", signal = ");
        explain_buffer_uint16_t(sb, data->signal);
    }
    explain_string_buffer_puts(sb, " }");
}


int
explain_video_tuner_get_n(int fildes)
{
    int lo = 0;
    int hi = 200;
    for (;;)
    {
        int mid = (lo + hi) / 2;
        struct video_tuner qry;
        memset(&qry, 0, sizeof(qry));
        qry.tuner = mid;
        if (ioctl(fildes, VIDIOCGTUNER, &qry) >= 0)
        {
            if (hi <= 0 && lo <= 0)
                return -1;
            /* mid < naudios */
            lo = mid + 1;
            if (lo > hi)
                return lo;
        }
        else if (errno != EINVAL)
        {
            return -1;
        }
        else
        {
            if (hi <= 0 && lo <= 0)
                return -1;
            /* mid >= naudios */
            hi = mid - 1;
            if (lo >= hi)
                return lo;
        }
    }
}

#endif

/* vim: set ts=8 sw=4 et : */
