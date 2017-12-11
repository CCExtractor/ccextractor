/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011-2013 Peter Miller
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
#include <libexplain/buffer/v4l2_buf_flags.h>
#include <libexplain/parse_bits.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_buf_flags(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "V4L2_BUF_FLAG_MAPPED", V4L2_BUF_FLAG_MAPPED },
        { "V4L2_BUF_FLAG_QUEUED", V4L2_BUF_FLAG_QUEUED },
        { "V4L2_BUF_FLAG_DONE", V4L2_BUF_FLAG_DONE },
        { "V4L2_BUF_FLAG_KEYFRAME", V4L2_BUF_FLAG_KEYFRAME },
        { "V4L2_BUF_FLAG_PFRAME", V4L2_BUF_FLAG_PFRAME },
        { "V4L2_BUF_FLAG_BFRAME", V4L2_BUF_FLAG_BFRAME },
#ifdef V4L2_BUF_FLAG_ERROR
        { "V4L2_BUF_FLAG_ERROR", V4L2_BUF_FLAG_ERROR },
#endif
        { "V4L2_BUF_FLAG_TIMECODE", V4L2_BUF_FLAG_TIMECODE },
#ifdef V4L2_BUF_FLAG_INPUT
        /*  watch out for systems where it is defined, but empty */
        { "V4L2_BUF_FLAG_INPUT", V4L2_BUF_FLAG_INPUT + 0 },
#endif
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}

#endif

/* vim: set ts=8 sw=4 et : */
