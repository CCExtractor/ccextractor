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

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/v4l2_std_id.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

void
explain_buffer_v4l2_std_id(explain_string_buffer_t *sb,
    unsigned long long value)
{
    /*
     * awk '/#define V4L2_STD_/ {
     *     print "#ifdef " $2
     *     print "        { \"" $2 "\", " $2 " },"
     *     print "#endif"
     * } ' /usr/include/linux/videodev2.h
     */
    static const explain_parse_bits_table_t table[] =
    {
#ifdef V4L2_STD_PAL_B
        { "V4L2_STD_PAL_B", V4L2_STD_PAL_B },
#endif
#ifdef V4L2_STD_PAL_B1
        { "V4L2_STD_PAL_B1", V4L2_STD_PAL_B1 },
#endif
#ifdef V4L2_STD_PAL_G
        { "V4L2_STD_PAL_G", V4L2_STD_PAL_G },
#endif
#ifdef V4L2_STD_PAL_H
        { "V4L2_STD_PAL_H", V4L2_STD_PAL_H },
#endif
#ifdef V4L2_STD_PAL_I
        { "V4L2_STD_PAL_I", V4L2_STD_PAL_I },
#endif
#ifdef V4L2_STD_PAL_D
        { "V4L2_STD_PAL_D", V4L2_STD_PAL_D },
#endif
#ifdef V4L2_STD_PAL_D1
        { "V4L2_STD_PAL_D1", V4L2_STD_PAL_D1 },
#endif
#ifdef V4L2_STD_PAL_K
        { "V4L2_STD_PAL_K", V4L2_STD_PAL_K },
#endif
#ifdef V4L2_STD_PAL_M
        { "V4L2_STD_PAL_M", V4L2_STD_PAL_M },
#endif
#ifdef V4L2_STD_PAL_N
        { "V4L2_STD_PAL_N", V4L2_STD_PAL_N },
#endif
#ifdef V4L2_STD_PAL_Nc
        { "V4L2_STD_PAL_Nc", V4L2_STD_PAL_Nc },
#endif
#ifdef V4L2_STD_PAL_60
        { "V4L2_STD_PAL_60", V4L2_STD_PAL_60 },
#endif
#ifdef V4L2_STD_NTSC_M
        { "V4L2_STD_NTSC_M", V4L2_STD_NTSC_M },
#endif
#ifdef V4L2_STD_NTSC_M_JP
        { "V4L2_STD_NTSC_M_JP", V4L2_STD_NTSC_M_JP },
#endif
#ifdef V4L2_STD_NTSC_443
        { "V4L2_STD_NTSC_443", V4L2_STD_NTSC_443 },
#endif
#ifdef V4L2_STD_NTSC_M_KR
        { "V4L2_STD_NTSC_M_KR", V4L2_STD_NTSC_M_KR },
#endif
#ifdef V4L2_STD_SECAM_B
        { "V4L2_STD_SECAM_B", V4L2_STD_SECAM_B },
#endif
#ifdef V4L2_STD_SECAM_D
        { "V4L2_STD_SECAM_D", V4L2_STD_SECAM_D },
#endif
#ifdef V4L2_STD_SECAM_G
        { "V4L2_STD_SECAM_G", V4L2_STD_SECAM_G },
#endif
#ifdef V4L2_STD_SECAM_H
        { "V4L2_STD_SECAM_H", V4L2_STD_SECAM_H },
#endif
#ifdef V4L2_STD_SECAM_K
        { "V4L2_STD_SECAM_K", V4L2_STD_SECAM_K },
#endif
#ifdef V4L2_STD_SECAM_K1
        { "V4L2_STD_SECAM_K1", V4L2_STD_SECAM_K1 },
#endif
#ifdef V4L2_STD_SECAM_L
        { "V4L2_STD_SECAM_L", V4L2_STD_SECAM_L },
#endif
#ifdef V4L2_STD_SECAM_LC
        { "V4L2_STD_SECAM_LC", V4L2_STD_SECAM_LC },
#endif
#ifdef V4L2_STD_ATSC_8_VSB
        { "V4L2_STD_ATSC_8_VSB", V4L2_STD_ATSC_8_VSB },
#endif
#ifdef V4L2_STD_ATSC_16_VSB
        { "V4L2_STD_ATSC_16_VSB", V4L2_STD_ATSC_16_VSB },
#endif
#ifdef V4L2_STD_MN
        { "V4L2_STD_MN", V4L2_STD_MN },
#endif
#ifdef V4L2_STD_B
        { "V4L2_STD_B", V4L2_STD_B },
#endif
#ifdef V4L2_STD_GH
        { "V4L2_STD_GH", V4L2_STD_GH },
#endif
#ifdef V4L2_STD_DK
        { "V4L2_STD_DK", V4L2_STD_DK },
#endif
#ifdef V4L2_STD_PAL_BG
        { "V4L2_STD_PAL_BG", V4L2_STD_PAL_BG },
#endif
#ifdef V4L2_STD_PAL_DK
        { "V4L2_STD_PAL_DK", V4L2_STD_PAL_DK },
#endif
#ifdef V4L2_STD_PAL
        { "V4L2_STD_PAL", V4L2_STD_PAL },
#endif
#ifdef V4L2_STD_NTSC
        { "V4L2_STD_NTSC", V4L2_STD_NTSC },
#endif
#ifdef V4L2_STD_SECAM_DK
        { "V4L2_STD_SECAM_DK", V4L2_STD_SECAM_DK },
#endif
#ifdef V4L2_STD_SECAM
        { "V4L2_STD_SECAM", V4L2_STD_SECAM },
#endif
#ifdef V4L2_STD_525_60
        { "V4L2_STD_525_60", V4L2_STD_525_60 },
#endif
#ifdef V4L2_STD_625_50
        { "V4L2_STD_625_50", V4L2_STD_625_50 },
#endif
#ifdef V4L2_STD_ATSC
        { "V4L2_STD_ATSC", V4L2_STD_ATSC },
#endif
#ifdef V4L2_STD_UNKNOWN
        { "V4L2_STD_UNKNOWN", V4L2_STD_UNKNOWN },
#endif
#ifdef V4L2_STD_ALL
        { "V4L2_STD_ALL", V4L2_STD_ALL },
#endif
    };
    if (value >= (1uLL << 32))
    {
        explain_string_buffer_printf(sb, "0x%LX", value);
        return;
    }

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_v4l2_std_id_ptr(explain_string_buffer_t *sb,
    const unsigned long long *data)
{
    if (explain_is_efault_pointer(data, sizeof(data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    explain_buffer_v4l2_std_id(sb, *data);
    explain_string_buffer_puts(sb, " }");
}

#endif

/* vim: set ts=8 sw=4 et : */
