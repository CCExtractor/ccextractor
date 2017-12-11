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

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/linux/videodev2.h>

#include <libexplain/buffer/v4l2_pixel_format.h>
#include <libexplain/option.h>
#include <libexplain/parse_bits.h>


#ifdef HAVE_LINUX_VIDEODEV2_H

/*
 * awk '/#define V4L2_PIX_FMT_/{
 *     print "#ifdef " $2
 *     print "        { \"" $2 "\", " $2 " },"
 *     print "#endif"
 * }
 * ' /usr/include/linux/videodev2.h
 */
static const explain_parse_bits_table_t table[] =
{
#ifdef V4L2_PIX_FMT_RGB332
    { "V4L2_PIX_FMT_RGB332", V4L2_PIX_FMT_RGB332 },
#endif
#ifdef V4L2_PIX_FMT_RGB444
    { "V4L2_PIX_FMT_RGB444", V4L2_PIX_FMT_RGB444 },
#endif
#ifdef V4L2_PIX_FMT_RGB555
    { "V4L2_PIX_FMT_RGB555", V4L2_PIX_FMT_RGB555 },
#endif
#ifdef V4L2_PIX_FMT_RGB565
    { "V4L2_PIX_FMT_RGB565", V4L2_PIX_FMT_RGB565 },
#endif
#ifdef V4L2_PIX_FMT_RGB555X
    { "V4L2_PIX_FMT_RGB555X", V4L2_PIX_FMT_RGB555X },
#endif
#ifdef V4L2_PIX_FMT_RGB565X
    { "V4L2_PIX_FMT_RGB565X", V4L2_PIX_FMT_RGB565X },
#endif
#ifdef V4L2_PIX_FMT_BGR666
    { "V4L2_PIX_FMT_BGR666", V4L2_PIX_FMT_BGR666 },
#endif
#ifdef V4L2_PIX_FMT_BGR24
    { "V4L2_PIX_FMT_BGR24", V4L2_PIX_FMT_BGR24 },
#endif
#ifdef V4L2_PIX_FMT_RGB24
    { "V4L2_PIX_FMT_RGB24", V4L2_PIX_FMT_RGB24 },
#endif
#ifdef V4L2_PIX_FMT_BGR32
    { "V4L2_PIX_FMT_BGR32", V4L2_PIX_FMT_BGR32 },
#endif
#ifdef V4L2_PIX_FMT_RGB32
    { "V4L2_PIX_FMT_RGB32", V4L2_PIX_FMT_RGB32 },
#endif
#ifdef V4L2_PIX_FMT_GREY
    { "V4L2_PIX_FMT_GREY", V4L2_PIX_FMT_GREY },
#endif
#ifdef V4L2_PIX_FMT_Y4
    { "V4L2_PIX_FMT_Y4", V4L2_PIX_FMT_Y4 },
#endif
#ifdef V4L2_PIX_FMT_Y6
    { "V4L2_PIX_FMT_Y6", V4L2_PIX_FMT_Y6 },
#endif
#ifdef V4L2_PIX_FMT_Y10
    { "V4L2_PIX_FMT_Y10", V4L2_PIX_FMT_Y10 },
#endif
#ifdef V4L2_PIX_FMT_Y16
    { "V4L2_PIX_FMT_Y16", V4L2_PIX_FMT_Y16 },
#endif
#ifdef V4L2_PIX_FMT_PAL8
    { "V4L2_PIX_FMT_PAL8", V4L2_PIX_FMT_PAL8 },
#endif
#ifdef V4L2_PIX_FMT_YVU410
    { "V4L2_PIX_FMT_YVU410", V4L2_PIX_FMT_YVU410 },
#endif
#ifdef V4L2_PIX_FMT_YVU420
    { "V4L2_PIX_FMT_YVU420", V4L2_PIX_FMT_YVU420 },
#endif
#ifdef V4L2_PIX_FMT_YUYV
    { "V4L2_PIX_FMT_YUYV", V4L2_PIX_FMT_YUYV },
#endif
#ifdef V4L2_PIX_FMT_YYUV
    { "V4L2_PIX_FMT_YYUV", V4L2_PIX_FMT_YYUV },
#endif
#ifdef V4L2_PIX_FMT_YVYU
    { "V4L2_PIX_FMT_YVYU", V4L2_PIX_FMT_YVYU },
#endif
#ifdef V4L2_PIX_FMT_UYVY
    { "V4L2_PIX_FMT_UYVY", V4L2_PIX_FMT_UYVY },
#endif
#ifdef V4L2_PIX_FMT_VYUY
    { "V4L2_PIX_FMT_VYUY", V4L2_PIX_FMT_VYUY },
#endif
#ifdef V4L2_PIX_FMT_YUV422P
    { "V4L2_PIX_FMT_YUV422P", V4L2_PIX_FMT_YUV422P },
#endif
#ifdef V4L2_PIX_FMT_YUV411P
    { "V4L2_PIX_FMT_YUV411P", V4L2_PIX_FMT_YUV411P },
#endif
#ifdef V4L2_PIX_FMT_Y41P
    { "V4L2_PIX_FMT_Y41P", V4L2_PIX_FMT_Y41P },
#endif
#ifdef V4L2_PIX_FMT_YUV444
    { "V4L2_PIX_FMT_YUV444", V4L2_PIX_FMT_YUV444 },
#endif
#ifdef V4L2_PIX_FMT_YUV555
    { "V4L2_PIX_FMT_YUV555", V4L2_PIX_FMT_YUV555 },
#endif
#ifdef V4L2_PIX_FMT_YUV565
    { "V4L2_PIX_FMT_YUV565", V4L2_PIX_FMT_YUV565 },
#endif
#ifdef V4L2_PIX_FMT_YUV32
    { "V4L2_PIX_FMT_YUV32", V4L2_PIX_FMT_YUV32 },
#endif
#ifdef V4L2_PIX_FMT_YUV410
    { "V4L2_PIX_FMT_YUV410", V4L2_PIX_FMT_YUV410 },
#endif
#ifdef V4L2_PIX_FMT_YUV420
    { "V4L2_PIX_FMT_YUV420", V4L2_PIX_FMT_YUV420 },
#endif
#ifdef V4L2_PIX_FMT_HI240
    { "V4L2_PIX_FMT_HI240", V4L2_PIX_FMT_HI240 },
#endif
#ifdef V4L2_PIX_FMT_HM12
    { "V4L2_PIX_FMT_HM12", V4L2_PIX_FMT_HM12 },
#endif
#ifdef V4L2_PIX_FMT_NV12
    { "V4L2_PIX_FMT_NV12", V4L2_PIX_FMT_NV12 },
#endif
#ifdef V4L2_PIX_FMT_NV21
    { "V4L2_PIX_FMT_NV21", V4L2_PIX_FMT_NV21 },
#endif
#ifdef V4L2_PIX_FMT_NV16
    { "V4L2_PIX_FMT_NV16", V4L2_PIX_FMT_NV16 },
#endif
#ifdef V4L2_PIX_FMT_NV61
    { "V4L2_PIX_FMT_NV61", V4L2_PIX_FMT_NV61 },
#endif
#ifdef V4L2_PIX_FMT_SBGGR8
    { "V4L2_PIX_FMT_SBGGR8", V4L2_PIX_FMT_SBGGR8 },
#endif
#ifdef V4L2_PIX_FMT_SGBRG8
    { "V4L2_PIX_FMT_SGBRG8", V4L2_PIX_FMT_SGBRG8 },
#endif
#ifdef V4L2_PIX_FMT_SGRBG8
    { "V4L2_PIX_FMT_SGRBG8", V4L2_PIX_FMT_SGRBG8 },
#endif
#ifdef V4L2_PIX_FMT_SRGGB8
    { "V4L2_PIX_FMT_SRGGB8", V4L2_PIX_FMT_SRGGB8 },
#endif
#ifdef V4L2_PIX_FMT_SBGGR10
    { "V4L2_PIX_FMT_SBGGR10", V4L2_PIX_FMT_SBGGR10 },
#endif
#ifdef V4L2_PIX_FMT_SGBRG10
    { "V4L2_PIX_FMT_SGBRG10", V4L2_PIX_FMT_SGBRG10 },
#endif
#ifdef V4L2_PIX_FMT_SGRBG10
    { "V4L2_PIX_FMT_SGRBG10", V4L2_PIX_FMT_SGRBG10 },
#endif
#ifdef V4L2_PIX_FMT_SRGGB10
    { "V4L2_PIX_FMT_SRGGB10", V4L2_PIX_FMT_SRGGB10 },
#endif
#ifdef V4L2_PIX_FMT_SGRBG10DPCM8
    { "V4L2_PIX_FMT_SGRBG10DPCM8", V4L2_PIX_FMT_SGRBG10DPCM8 },
#endif
#ifdef V4L2_PIX_FMT_SBGGR16
    { "V4L2_PIX_FMT_SBGGR16", V4L2_PIX_FMT_SBGGR16 },
#endif
#ifdef V4L2_PIX_FMT_MJPEG
    { "V4L2_PIX_FMT_MJPEG", V4L2_PIX_FMT_MJPEG },
#endif
#ifdef V4L2_PIX_FMT_JPEG
    { "V4L2_PIX_FMT_JPEG", V4L2_PIX_FMT_JPEG },
#endif
#ifdef V4L2_PIX_FMT_DV
    { "V4L2_PIX_FMT_DV", V4L2_PIX_FMT_DV },
#endif
#ifdef V4L2_PIX_FMT_MPEG
    { "V4L2_PIX_FMT_MPEG", V4L2_PIX_FMT_MPEG },
#endif
#ifdef V4L2_PIX_FMT_CPIA1
    { "V4L2_PIX_FMT_CPIA1", V4L2_PIX_FMT_CPIA1 },
#endif
#ifdef V4L2_PIX_FMT_WNVA
    { "V4L2_PIX_FMT_WNVA", V4L2_PIX_FMT_WNVA },
#endif
#ifdef V4L2_PIX_FMT_SN9C10X
    { "V4L2_PIX_FMT_SN9C10X", V4L2_PIX_FMT_SN9C10X },
#endif
#ifdef V4L2_PIX_FMT_SN9C20X_I420
    { "V4L2_PIX_FMT_SN9C20X_I420", V4L2_PIX_FMT_SN9C20X_I420 },
#endif
#ifdef V4L2_PIX_FMT_PWC1
    { "V4L2_PIX_FMT_PWC1", V4L2_PIX_FMT_PWC1 },
#endif
#ifdef V4L2_PIX_FMT_PWC2
    { "V4L2_PIX_FMT_PWC2", V4L2_PIX_FMT_PWC2 },
#endif
#ifdef V4L2_PIX_FMT_ET61X251
    { "V4L2_PIX_FMT_ET61X251", V4L2_PIX_FMT_ET61X251 },
#endif
#ifdef V4L2_PIX_FMT_SPCA501
    { "V4L2_PIX_FMT_SPCA501", V4L2_PIX_FMT_SPCA501 },
#endif
#ifdef V4L2_PIX_FMT_SPCA505
    { "V4L2_PIX_FMT_SPCA505", V4L2_PIX_FMT_SPCA505 },
#endif
#ifdef V4L2_PIX_FMT_SPCA508
    { "V4L2_PIX_FMT_SPCA508", V4L2_PIX_FMT_SPCA508 },
#endif
#ifdef V4L2_PIX_FMT_SPCA561
    { "V4L2_PIX_FMT_SPCA561", V4L2_PIX_FMT_SPCA561 },
#endif
#ifdef V4L2_PIX_FMT_PAC207
    { "V4L2_PIX_FMT_PAC207", V4L2_PIX_FMT_PAC207 },
#endif
#ifdef V4L2_PIX_FMT_MR97310A
    { "V4L2_PIX_FMT_MR97310A", V4L2_PIX_FMT_MR97310A },
#endif
#ifdef V4L2_PIX_FMT_SN9C2028
    { "V4L2_PIX_FMT_SN9C2028", V4L2_PIX_FMT_SN9C2028 },
#endif
#ifdef V4L2_PIX_FMT_SQ905C
    { "V4L2_PIX_FMT_SQ905C", V4L2_PIX_FMT_SQ905C },
#endif
#ifdef V4L2_PIX_FMT_PJPG
    { "V4L2_PIX_FMT_PJPG", V4L2_PIX_FMT_PJPG },
#endif
#ifdef V4L2_PIX_FMT_OV511
    { "V4L2_PIX_FMT_OV511", V4L2_PIX_FMT_OV511 },
#endif
#ifdef V4L2_PIX_FMT_OV518
    { "V4L2_PIX_FMT_OV518", V4L2_PIX_FMT_OV518 },
#endif
#ifdef V4L2_PIX_FMT_STV0680
    { "V4L2_PIX_FMT_STV0680", V4L2_PIX_FMT_STV0680 },
#endif
#ifdef V4L2_PIX_FMT_TM6000
    { "V4L2_PIX_FMT_TM6000", V4L2_PIX_FMT_TM6000 },
#endif
#ifdef V4L2_PIX_FMT_CIT_YYVYUY
    { "V4L2_PIX_FMT_CIT_YYVYUY", V4L2_PIX_FMT_CIT_YYVYUY },
#endif
#ifdef V4L2_PIX_FMT_KONICA420
    { "V4L2_PIX_FMT_KONICA420", V4L2_PIX_FMT_KONICA420 },
#endif
};


void
explain_buffer_v4l2_pixel_format(explain_string_buffer_t *sb, unsigned value)
{
    const explain_parse_bits_table_t *tp;
    unsigned char cc[4];

    tp = explain_parse_bits_find_by_value(value, table, SIZEOF(table));
    if (tp)
    {
        explain_string_buffer_puts(sb, tp->name);
        return;
    }

    /*
     * if it consists of four printable characters,
     * it means the above table is probably incomplete
     */
    cc[0] = value;
    cc[1] = value >> 8;
    cc[2] = value >> 16;
    cc[3] = value >> 24;
    if (isprint(cc[0]) && isprint(cc[1]) && isprint(cc[2]) && isprint(cc[3]))
    {
        explain_string_buffer_puts(sb, "v4l2_fourcc(");
        explain_string_buffer_putc_quoted(sb, cc[0]);
        explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_putc_quoted(sb, cc[1]);
        explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_putc_quoted(sb, cc[2]);
        explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_putc_quoted(sb, cc[3]);
        explain_string_buffer_putc(sb, ')');

        if (explain_option_debug())
        {
            explain_string_buffer_puts(sb->footnotes, "; ");
            explain_string_buffer_printf
            (
                sb->footnotes,
                /* FIXME: i18n */
                "the V4L2_PIX_FMT table in %s may be out of date",
                __FILE__
            );
        }
        return;
    }

    /*
     * If all else fails, print it in hexadecimal.
     */
    explain_string_buffer_printf(sb, "0x%08X", value);
}


enum explain_v4l2_pixel_format_check_t
explain_v4l2_pixel_format_check(unsigned value)
{
    if (explain_parse_bits_find_by_value(value, table, SIZEOF(table)))
        return explain_v4l2_pixel_format_check_ok;
    return explain_v4l2_pixel_format_check_nosuch;
};

#endif

/* vim: set ts=8 sw=4 et : */
