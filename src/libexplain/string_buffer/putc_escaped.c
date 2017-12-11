/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/limits.h>

#include <libexplain/string_buffer.h>


void
explain_string_buffer_putc_escaped(explain_string_buffer_t *sb, int c,
    int delimiter)
{
    if (c == delimiter || c == '\\')
    {
        explain_string_buffer_putc(sb, '\\');
        explain_string_buffer_putc(sb, c);
        return;
    }
    switch (c)
    {
    case '\a':
        explain_string_buffer_puts(sb, "\\a");
        break;

    case '\b':
        explain_string_buffer_puts(sb, "\\b");
        break;

    case '\f':
        explain_string_buffer_puts(sb, "\\f");
        break;

    case '\n':
        explain_string_buffer_puts(sb, "\\n");
        break;

    case '\r':
        explain_string_buffer_puts(sb, "\\r");
        break;

    case '\t':
        explain_string_buffer_puts(sb, "\\t");
        break;

    case '\v':
        explain_string_buffer_puts(sb, "\\v");
        break;

    case ' ':
    case '!':
    case '"':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case ':':
    case ';':
    case '<':
    case '=':
    case '>':
    case '?':
    case '@':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '[':
    case '\\':
    case ']':
    case '^':
    case '_':
    case '`':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case '{':
    case '|':
    case '}':
    case '~':
        printable:
        explain_string_buffer_putc(sb, c);
        break;

    default:
        /* this test will be locale specific */
        if (isprint((unsigned char)c))
            goto printable;

        /*
         * We can't use "\\%03o" or "\\%3.3o" or "\\%03.3o" here because
         * different systems interpret them differently.  One or the
         * other is often space padded rather than zero padded.
         * So we convert the value ourselves.
         */
        explain_string_buffer_putc(sb, '\\');
        if (CHAR_BIT > 8)
            explain_string_buffer_putc(sb, ((c >> 6) & 7) + '0');
        else
            explain_string_buffer_putc(sb, ((c >> 6) & 3) + '0');
        explain_string_buffer_putc(sb, ((c >> 3) & 7) + '0');
        explain_string_buffer_putc(sb, (c & 7) + '0');
        break;
    }
}


/* vim: set ts=8 sw=4 et : */
