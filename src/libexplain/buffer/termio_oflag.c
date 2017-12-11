/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/ac/linux/termios.h>

#include <libexplain/buffer/termio_oflag.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


void
explain_buffer_termio_oflag(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "OPOST", OPOST },
#ifdef OLCUC
        { "OLCUC", OLCUC },
#endif
        { "ONLCR", ONLCR },
        { "OCRNL", OCRNL },
        { "ONOCR", ONOCR },
        { "ONLRET", ONLRET },
#ifdef OFILL
        { "OFILL", OFILL },
#endif
#ifdef OFDEL
        { "OFDEL", OFDEL },
#endif
#ifdef NL1
        { "NL1", NL1 },
#endif
#ifdef BS1
        { "BS1", BS1 },
#endif
#ifdef VT1
        { "VT1", VT1 },
#endif
#ifdef FF1
        { "FF1", FF1 },
#endif
    };

    int first = 1;

#ifdef CRDLY
    switch (value & CRDLY)
    {
    default:
    case CR0:
        break;

    case CR1:
        explain_string_buffer_puts(sb, "CR1");
        first = 0;
        break;

    case CR2:
        explain_string_buffer_puts(sb, "CR2");
        first = 0;
        break;

    case CR3:
        explain_string_buffer_puts(sb, "CR3");
        first = 0;
        break;
    }
    value &= ~CRDLY;
#endif

#ifdef TABDLY
    switch (value & TABDLY)
    {
    case TAB0:
        value &= ~TABDLY;
        break;

#ifdef TAB1
    case TAB1:
        if (!first)
            explain_string_buffer_puts(sb, " | ");
        explain_string_buffer_puts(sb, "TAB1");
        first = 0;
        value &= ~TABDLY;
        break;
#endif

#ifdef TAB2
    case TAB2:
        if (!first)
            explain_string_buffer_puts(sb, " | ");
        explain_string_buffer_puts(sb, "TAB2");
        first = 0;
        value &= ~TABDLY;
        break;
#endif

    case TAB3:
        if (!first)
            explain_string_buffer_puts(sb, " | ");
        explain_string_buffer_puts(sb, "TAB3");
        first = 0;
        value &= ~TABDLY;
        break;

    default:
        break;
    }
#endif

    if (!value && !first)
        return;
    if (!first)
        explain_string_buffer_puts(sb, " | ");
    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


/* vim: set ts=8 sw=4 et : */
