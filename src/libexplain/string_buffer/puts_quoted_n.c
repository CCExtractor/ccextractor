/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011-2013 Peter Miller
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

#include <libexplain/string_buffer.h>


void
explain_string_buffer_puts_quoted_n(explain_string_buffer_t *sb,
    const char *s, size_t n)
{
    if (!s)
    {
        explain_string_buffer_puts(sb, "NULL");
        return;
    }
    explain_string_buffer_putc(sb, '"');
    while (n > 0)
    {
        unsigned char   c;

        c = *s++;
        --n;
        switch (c)
        {
        case '\0':
            explain_string_buffer_putc(sb, '"');
            return;

        case '?':
            /*
             * Watch out for C string contents that could look like a
             * trigraph, the second question mark will need to be quoted.
             */
            explain_string_buffer_putc(sb, '?');
            if (n >= 2 && s[0] == '?')
            {
                switch (s[1])
                {
                case '!':
                case '\'':
                case '(':
                case ')':
                case '-':
                case '/':
                case '<':
                case '=':
                case '>':
                    ++s;
                    --n;
                    explain_string_buffer_putc(sb, '\\');
                    explain_string_buffer_putc(sb, '?');
                    break;

                default:
                    /* not a trigraph */
                    break;
                }
            }
            break;

        default:
            explain_string_buffer_putc_escaped(sb, c, '"');
            break;
        }
    }
    explain_string_buffer_putc(sb, '"');
}


void
explain_string_buffer_putsu_quoted_n(explain_string_buffer_t *sb,
    const unsigned char *s, size_t n)
{
    explain_string_buffer_puts_quoted_n(sb, (const char *)s, n);
}


/* vim: set ts=8 sw=4 et : */
