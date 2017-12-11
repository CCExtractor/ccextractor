/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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

#include <libexplain/string_buffer.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


void
explain_string_buffer_puts_shell_quoted(explain_string_buffer_t *sb,
    const char *text)
{
    const char      *cp;
    int             needs_quoting;
    unsigned char   mode;
    unsigned char   c;

    if (!text)
        text = "";

    /*
     * Work out if the string needs quoting.
     *
     * The empty string is not quoted, even though it could be argued
     * that it needs to be.  It has proven more useful in the present
     * form, because it allows empty filename lists to pass through
     * and remain empty.
     */
    needs_quoting = 0;
    mode = 0;
    cp = text;
    for (;;)
    {
        c = *cp++;
        switch (c)
        {
        case 0:
            break;

        default:
            if (isspace(c) || !isprint(c))
                needs_quoting = 1;
            continue;

        case '!':
            /*
             * special for bash and csh
             *
             * You can only quote exclamation marks within single
             * quotes.  You can't escape them within double quotes.
             */
            mode = '\'';
            needs_quoting = 1;
            break;

        case '\'':
            /*
             * You can only quote single quote within double
             * quotes.  You can't escape them within single quotes.
             */
            mode = '"';
            needs_quoting = 1;
            break;

        case '"':
        case '#':
        case '$':
        case '&':
        case '(':
        case ')':
        case '*':
        case ':':
        case ';':
        case '<':
        case '=':
        case '>':
        case '?':
        case '[':
        case '\\':
        case ']':
        case '^':
        case '`':
        case '{':
        case '|':
        case '}':
        case '~':
            needs_quoting = 1;
            continue;
        }
        break;
    }

    /*
     * If it doesn't need quoting, return immediately.
     */
    if (!needs_quoting)
    {
        explain_string_buffer_puts(sb, text);
        return;
    }

    /*
     * If we have a choice, use single quote mode,
     * it's shorter and easier to read.
     */
    if (!mode)
        mode = '\'';
    explain_string_buffer_putc(sb, mode);

    /*
     * Form the quoted string, using the minimum number of escapes.
     *
     * The gotcha here is the backquote: the `blah` substitution is
     * still active within double quotes.  And so are a few others.
     *
     * Also, there are some difficulties: the single quote can't be
     * quoted within single quotes, and the exclamation mark can't
     * be quoted by anything *except* single quotes.  Sheesh.
     *
     * Also, the rules change depending on which style of quoting
     * is in force at the time.
     */
    cp = text;
    for (;;)
    {
        c = *cp++;
        if (!c)
            break;
        if (mode == '\'')
        {
            /* within single quotes */
            if (c == '\'')
            {
                /*
                 * You can't quote a single quote within
                 * single quotes.  Need to change to
                 * double quote mode.
                 */
                explain_string_buffer_puts(sb, "'\"'");
                mode = '"';
            }
            else
                explain_string_buffer_putc(sb, c);
        }
        else
        {
            /* within double quotes */
            switch (c)
            {
            case '!':
                /*
                 * You can't quote an exclamation mark
                 * within double quotes.  Need to change
                 * to single quote mode.
                 */
                explain_string_buffer_puts(sb, "\"'!");
                mode = '\'';
                break;

            case '\n':
            case '"':
            case '\\':
            case '`': /* stop command substitutions */
            case '$': /* stop variable substitutions */
                explain_string_buffer_putc(sb, '\\');
                /* fall through... */

            default:
                explain_string_buffer_putc(sb, c);
                break;
            }
        }
    }
    explain_string_buffer_putc(sb, mode);
}


/* vim: set ts=8 sw=4 et : */
