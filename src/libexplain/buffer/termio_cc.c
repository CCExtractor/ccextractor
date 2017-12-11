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

#include <libexplain/buffer/termio_cc.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static void
char_name(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "VINTR", VINTR },
        { "VQUIT", VQUIT },
        { "VERASE", VERASE },
        { "VKILL", VKILL },
        { "VEOF", VEOF },
#ifdef VTIME
        { "VTIME", VTIME },
#endif
#ifdef VMIN
        { "VMIN", VMIN },
#endif
#ifdef VSWTC
        { "VSWTC", VSWTC },
#endif
#ifdef VSTART
        { "VSTART", VSTART },
#endif
#ifdef VSTOP
        { "VSTOP", VSTOP },
#endif
#ifdef VSUSP
        { "VSUSP", VSUSP },
#endif
#ifdef VEOL
        { "VEOL", VEOL },
#endif
#ifdef VREPRINT
        { "VREPRINT", VREPRINT },
#endif
#ifdef VDISCARD
        { "VDISCARD", VDISCARD },
#endif
#ifdef VWERASE
        { "VWERASE", VWERASE },
#endif
#ifdef VLNEXT
        { "VLNEXT", VLNEXT },
#endif
#ifdef VEOL2
        { "VEOL2", VEOL2 },
#endif
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_termio_cc(explain_string_buffer_t *sb, const unsigned char *data,
    size_t data_size)
{
    int             first;
    size_t          j;

    explain_string_buffer_putc(sb, '{');
    first = 1;
    for (j = 0; j < data_size; ++j)
    {
        unsigned char   c;

        c = data[j];
        if (c)
        {
            if (!first)
                explain_string_buffer_puts(sb, ", ");
            char_name(sb, j);
            explain_string_buffer_puts(sb, " = ");
            explain_string_buffer_putc_quoted(sb, c);
            first = 0;
        }
    }
    explain_string_buffer_putc(sb, '}');
}


/* vim: set ts=8 sw=4 et : */
