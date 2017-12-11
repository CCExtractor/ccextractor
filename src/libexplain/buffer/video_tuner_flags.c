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

#include <libexplain/buffer/video_tuner_flags.h>
#include <libexplain/parse_bits.h>

#ifdef VIDIOCGTUNER


void
explain_buffer_video_tuner_flags(explain_string_buffer_t *sb, int value)
{
    static explain_parse_bits_table_t table[] =
    {
        { "VIDEO_TUNER_PAL", VIDEO_TUNER_PAL },
        { "VIDEO_TUNER_NTSC", VIDEO_TUNER_NTSC },
        { "VIDEO_TUNER_SECAM", VIDEO_TUNER_SECAM },
        { "VIDEO_TUNER_LOW", VIDEO_TUNER_LOW },
        { "VIDEO_TUNER_NORM", VIDEO_TUNER_NORM },
        { "VIDEO_TUNER_STEREO_ON", VIDEO_TUNER_STEREO_ON },
        { "VIDEO_TUNER_RDS_ON", VIDEO_TUNER_RDS_ON },
        { "VIDEO_TUNER_MBS_ON", VIDEO_TUNER_MBS_ON },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}

#endif

/* vim: set ts=8 sw=4 et : */
