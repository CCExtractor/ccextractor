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

#include <libexplain/buffer/video_palette.h>
#include <libexplain/parse_bits.h>

#ifdef VIDIOCGPICT


void
explain_buffer_video_palette(explain_string_buffer_t *sb, int value)
{
    static explain_parse_bits_table_t table[] =
    {
        { "VIDEO_PALETTE_GREY", VIDEO_PALETTE_GREY },
        { "VIDEO_PALETTE_HI240", VIDEO_PALETTE_HI240 },
        { "VIDEO_PALETTE_RGB565", VIDEO_PALETTE_RGB565 },
        { "VIDEO_PALETTE_RGB24", VIDEO_PALETTE_RGB24 },
        { "VIDEO_PALETTE_RGB32", VIDEO_PALETTE_RGB32 },
        { "VIDEO_PALETTE_RGB555", VIDEO_PALETTE_RGB555 },
        { "VIDEO_PALETTE_YUV422", VIDEO_PALETTE_YUV422 },
        { "VIDEO_PALETTE_YUYV", VIDEO_PALETTE_YUYV },
        { "VIDEO_PALETTE_UYVY", VIDEO_PALETTE_UYVY },
        { "VIDEO_PALETTE_YUV420", VIDEO_PALETTE_YUV420 },
        { "VIDEO_PALETTE_YUV411", VIDEO_PALETTE_YUV411 },
        { "VIDEO_PALETTE_RAW", VIDEO_PALETTE_RAW },
        { "VIDEO_PALETTE_YUV422P", VIDEO_PALETTE_YUV422P },
        { "VIDEO_PALETTE_YUV411P", VIDEO_PALETTE_YUV411P },
        { "VIDEO_PALETTE_YUV420P", VIDEO_PALETTE_YUV420P },
        { "VIDEO_PALETTE_YUV410P", VIDEO_PALETTE_YUV410P },
        { "VIDEO_PALETTE_PLANAR", VIDEO_PALETTE_PLANAR },
        { "VIDEO_PALETTE_COMPONENT", VIDEO_PALETTE_COMPONENT },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}

#endif

/* vim: set ts=8 sw=4 et : */
