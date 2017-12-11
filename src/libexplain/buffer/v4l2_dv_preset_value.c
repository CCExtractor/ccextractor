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

#include <libexplain/buffer/v4l2_dv_preset_value.h>
#include <libexplain/parse_bits.h>


#ifdef HAVE_LINUX_VIDEODEV2_H
#if defined(VIDIOC_S_DV_PRESET) || defined(VIDIOC_G_DV_PRESET)

void
explain_buffer_v4l2_dv_preset_value(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "V4L2_DV_INVALID", V4L2_DV_INVALID },
        { "V4L2_DV_480P59_94", V4L2_DV_480P59_94 },
        { "V4L2_DV_576P50", V4L2_DV_576P50 },
        { "V4L2_DV_720P24", V4L2_DV_720P24 },
        { "V4L2_DV_720P25", V4L2_DV_720P25 },
        { "V4L2_DV_720P30", V4L2_DV_720P30 },
        { "V4L2_DV_720P50", V4L2_DV_720P50 },
        { "V4L2_DV_720P59_94", V4L2_DV_720P59_94 },
        { "V4L2_DV_720P60", V4L2_DV_720P60 },
        { "V4L2_DV_1080I29_97", V4L2_DV_1080I29_97 },
        { "V4L2_DV_1080I30", V4L2_DV_1080I30 },
        { "V4L2_DV_1080I25", V4L2_DV_1080I25 },
        { "V4L2_DV_1080I50", V4L2_DV_1080I50 },
        { "V4L2_DV_1080I60", V4L2_DV_1080I60 },
        { "V4L2_DV_1080P24", V4L2_DV_1080P24 },
        { "V4L2_DV_1080P25", V4L2_DV_1080P25 },
        { "V4L2_DV_1080P30", V4L2_DV_1080P30 },
        { "V4L2_DV_1080P50", V4L2_DV_1080P50 },
        { "V4L2_DV_1080P60", V4L2_DV_1080P60 },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}

#endif
#endif

/* vim: set ts=8 sw=4 et : */
