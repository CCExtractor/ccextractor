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
#ifndef HAVE_LINUX_TERMIOS_H
#include <libexplain/ac/termios.h>
#endif

#include <libexplain/buffer/termio_baud.h>
#include <libexplain/buffer/termio_cflag.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


void
explain_buffer_termio_cflag(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "CSTOPB", CSTOPB },
        { "CREAD", CREAD },
        { "PARENB", PARENB },
        { "PARODD", PARODD },
        { "HUPCL", HUPCL },
        { "CLOCAL", CLOCAL },
#ifdef CMSPAR
        { "CMSPAR", CMSPAR },
#endif
        { "CRTSCTS", CRTSCTS },
    };

    int first = 1;
    int n;

#ifdef CBAUD
    n = value & CBAUD;
    if (n)
    {
        explain_buffer_termio_baud(sb, n);
        first = 0;
        value &= ~CBAUD;
    }
#endif

#ifdef IBSHIFT
    n = (value & CIBAUD) >> IBSHIFT;
    if (n)
    {
        if (!first)
            explain_string_buffer_puts(sb, " | ");
        explain_string_buffer_putc(sb, '(');
        explain_buffer_termio_baud(sb, n);
        explain_string_buffer_puts(sb, " << IBSHIFT)");
        first = 0;
        value &= ~CIBAUD;
    }
#endif

    n = value & CSIZE;
    if (!first)
        explain_string_buffer_puts(sb, " | ");
    switch (n)
    {
    case CS5:
        explain_string_buffer_puts(sb, "CS5");
        break;

    case CS6:
        explain_string_buffer_puts(sb, "CS6");
        break;

    case CS7:
        explain_string_buffer_puts(sb, "CS7");
        break;

    case CS8:
    default:
        explain_string_buffer_puts(sb, "CS8");
        break;
    }
    value &= ~CSIZE;
    first = 0;

    if (!value)
        return;
    explain_string_buffer_puts(sb, " | ");
    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


/* vim: set ts=8 sw=4 et : */
