/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013, 2014 Peter Miller
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

#include <libexplain/ac/linux/kd.h>

#include <libexplain/buffer/console_font_op.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_KD_H

static void
explain_buffer_kd_font_op(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef KD_FONT_OP_SET
        { "KD_FONT_OP_SET", KD_FONT_OP_SET },
#endif
#ifdef KD_FONT_OP_GET
        { "KD_FONT_OP_GET", KD_FONT_OP_GET },
#endif
#ifdef KD_FONT_OP_SET_DEFAULT
        { "KD_FONT_OP_SET_DEFAULT", KD_FONT_OP_SET_DEFAULT },
#endif
#ifdef KD_FONT_OP_COPY
        { "KD_FONT_OP_COPY", KD_FONT_OP_COPY },
#endif
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


static void
explain_buffer_kd_font_flag(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef KD_FONT_FLAG_DONT_RECALC
        { "KD_FONT_FLAG_DONT_RECALC", KD_FONT_FLAG_DONT_RECALC },
#endif
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}

#endif


void
explain_buffer_console_font_op(explain_string_buffer_t *sb,
    const struct console_font_op *value)
{
#ifdef HAVE_LINUX_KD_H
    if (explain_is_efault_pointer(value, sizeof(*value)))
        explain_buffer_pointer(sb, value);
    else
    {
        explain_string_buffer_puts(sb, "{ op = ");
        explain_buffer_kd_font_op(sb, value->op);
        explain_string_buffer_puts(sb, ", flags = ");
        explain_buffer_kd_font_flag(sb, value->flags);
        explain_string_buffer_printf
        (
            sb,
            ", width = %u, height = %u, charcount = %u, data = ",
            value->width,
            value->height,
            value->charcount
        );
        explain_buffer_pointer(sb, value->data);
        explain_string_buffer_puts(sb, " }");
    }
#else
    explain_buffer_pointer(sb, value);
#endif
}


/* vim: set ts=8 sw=4 et : */
