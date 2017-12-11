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
#include <libexplain/ac/linux/videodev2.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_frequency.h>
#include <libexplain/buffer/v4l2_tuner_type.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_frequency(explain_string_buffer_t *sb,
    const struct v4l2_frequency *data, int extra)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ tuner = ");
    explain_buffer_uint32_t(sb, data->tuner);
    if (extra)
    {
        explain_string_buffer_puts(sb, ", type = ");
        explain_buffer_v4l2_tuner_type(sb, data->type);
        explain_string_buffer_puts(sb, ", frequency = ");
        explain_buffer_uint32_t(sb, data->frequency);
        if (!explain_uint32_array_all_zero(data->reserved,
            SIZEOF(data->reserved)))
        {
            explain_string_buffer_puts(sb, ", reserved = ");
            explain_buffer_uint32_array(sb, data->reserved,
                SIZEOF(data->reserved));
        }
    }
    explain_string_buffer_puts(sb, " }");
}


int
explain_v4l2_frequency_get_n(int fildes)
{
    const int       max = 200;
    int             lo;
    int             hi;

    lo = 0;
    hi = max;
    for (;;)
    {
        struct v4l2_frequency qry;
        int             mid;

        mid = (lo + hi) / 2;
        memset(&qry, 0, sizeof(qry));
        qry.tuner = mid;
        if (ioctl(fildes, VIDIOC_G_FREQUENCY, &qry) >= 0)
        {
            if (hi <= 0 && lo <= 0)
                return -1;
            /* mid < n */
            lo = mid + 1;
            if (lo > hi)
            {
                done:
                if (lo >= max)
                {
                    /*
                     * Many drivers have the same bug: they do not check
                     * data->tuner at all, with the result that they appear
                     * to support an infinite number of frequencies.
                     */
                    return 1;
                }
                return lo;
            }
        }
        else if (errno != EINVAL)
        {
            return -1;
        }
        else
        {
            if (hi <= 0 && lo <= 0)
                return -1;
            /* mid >= n */
            hi = mid - 1;
            if (lo >= hi)
                goto done;
        }
    }
}

#endif

/* vim: set ts=8 sw=4 et : */
