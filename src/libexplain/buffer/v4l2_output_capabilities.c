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

#include <libexplain/buffer/v4l2_output_capabilities.h>
#include <libexplain/parse_bits.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_output_capabilities(explain_string_buffer_t *sb,
    unsigned value)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef V4L2_OUT_CAP_PRESETS
        { "V4L2_OUT_CAP_PRESETS", V4L2_OUT_CAP_PRESETS },
#endif
#ifdef V4L2_OUT_CAP_CUSTOM_TIMINGS
        { "V4L2_OUT_CAP_CUSTOM_TIMINGS", V4L2_OUT_CAP_CUSTOM_TIMINGS },
#endif
#ifdef V4L2_OUT_CAP_STD
        { "V4L2_OUT_CAP_STD", V4L2_OUT_CAP_STD },
#endif
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}

#endif

/* vim: set ts=8 sw=4 et : */
