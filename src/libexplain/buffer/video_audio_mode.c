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

#include <libexplain/buffer/video_audio_mode.h>
#include <libexplain/parse_bits.h>

#ifdef VIDIOCGAUDIO


void
explain_buffer_video_audio_mode(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "VIDEO_SOUND_MONO", VIDEO_SOUND_MONO },
        { "VIDEO_SOUND_STEREO", VIDEO_SOUND_STEREO },
        { "VIDEO_SOUND_LANG1", VIDEO_SOUND_LANG1 },
        { "VIDEO_SOUND_LANG2", VIDEO_SOUND_LANG2 },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}

#endif

/* vim: set ts=8 sw=4 et : */
