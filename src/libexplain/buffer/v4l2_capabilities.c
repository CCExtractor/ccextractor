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

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/v4l2_capabilities.h>
#include <libexplain/parse_bits.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_capabilities(explain_string_buffer_t *sb, unsigned value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "V4L2_CAP_VIDEO_CAPTURE", V4L2_CAP_VIDEO_CAPTURE },
        { "V4L2_CAP_VIDEO_OUTPUT", V4L2_CAP_VIDEO_OUTPUT },
        { "V4L2_CAP_VIDEO_OVERLAY", V4L2_CAP_VIDEO_OVERLAY },
        { "V4L2_CAP_VBI_CAPTURE", V4L2_CAP_VBI_CAPTURE },
        { "V4L2_CAP_VBI_OUTPUT", V4L2_CAP_VBI_OUTPUT },
        { "V4L2_CAP_SLICED_VBI_CAPTURE", V4L2_CAP_SLICED_VBI_CAPTURE },
        { "V4L2_CAP_SLICED_VBI_OUTPUT", V4L2_CAP_SLICED_VBI_OUTPUT },
        { "V4L2_CAP_RDS_CAPTURE", V4L2_CAP_RDS_CAPTURE },
        { "V4L2_CAP_VIDEO_OUTPUT_OVERLAY", V4L2_CAP_VIDEO_OUTPUT_OVERLAY },
#ifdef V4L2_CAP_HW_FREQ_SEEK
        { "V4L2_CAP_HW_FREQ_SEEK", V4L2_CAP_HW_FREQ_SEEK },
#endif
#ifdef V4L2_CAP_RDS_OUTPUT
        { "V4L2_CAP_RDS_OUTPUT", V4L2_CAP_RDS_OUTPUT },
#endif
        { "V4L2_CAP_TUNER", V4L2_CAP_TUNER },
        { "V4L2_CAP_AUDIO", V4L2_CAP_AUDIO },
        { "V4L2_CAP_RADIO", V4L2_CAP_RADIO },
#ifdef V4L2_CAP_MODULATOR
        { "V4L2_CAP_MODULATOR", V4L2_CAP_MODULATOR },
#endif
        { "V4L2_CAP_READWRITE", V4L2_CAP_READWRITE },
        { "V4L2_CAP_ASYNCIO", V4L2_CAP_ASYNCIO },
        { "V4L2_CAP_STREAMING", V4L2_CAP_STREAMING },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}

#endif


/* vim: set ts=8 sw=4 et : */
