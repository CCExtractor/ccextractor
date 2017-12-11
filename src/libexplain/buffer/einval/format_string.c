/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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

#include <libexplain/ac/assert.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/einval.h>


void
explain_buffer_einval_format_string(explain_string_buffer_t *sb,
    const char *caption, const char *value, size_t errpos)
{
    const char      *end;
    const char      *percent;
    size_t          nbytes;
    explain_string_buffer_t yuck_sb;
    char            yuck[1000];
    const char      *internal;

    /*
     * The sequences "...%l%..." and "...%l%" are errors, but we don't
     * want the last %, we want the first.  Thus we move back one.
     * But must also be wary of "...%" or "...%l" i.e. unterminated
     * specifications.
     *
     * The unterminated cases that start with '%' are hard to detect, so
     * intially we ignore them.
     *
     * The last character is the offending character, or the actual
     * specifier (which is not in the "internal" set), step over it (it
     * could also be a percent).
     */
    assert(errpos > 0);
    end = value + errpos;
    percent = end - 1;

    /*
     * The errpos indicates the *end* of the broken format,
     * hunt backwards for the starting '%'.
     */
    internal = " #$'+-.0123456789ILZhjlqtz";
    while (percent > value)
    {
        unsigned char c;
        --percent;
        c = *percent;
        if (c == '%')
            break;
        if (!strchr(internal, c))
        {
            /*
             * Oh dear, the error is uglier than we thought.
             * Probably an unterminated specifier (like "...%").
             */
            percent = end - 1;
            break;
        }
    }

    /*
     * Build ourselves a quoted string.
     */
    explain_string_buffer_init(&yuck_sb, yuck, sizeof(yuck));
    nbytes = end - percent;
    explain_string_buffer_puts_quoted_n(&yuck_sb, percent, nbytes);

    /*
     * Explain why it is broken.
     */
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a system call
         * reports a problem with a printf(3)-style format string,
         * in the case where a format specifier is malformed.
         *
         * %1$s =>  the name of the offending system-call argument.
         * %2$s =>  the offending format specification (already quoted)
         * %3$ld => the byte position of the invalid format specifier within
         *          the format string
         */
        i18n("within the %s argument the conversion specification %s, "
            "starting at position %ld, is not valid"),
        caption,
        yuck,
        (long)(percent - value)
    );
}


void
explain_buffer_einval_format_string_hole(explain_string_buffer_t *sb,
    const char *caption, int hole_index)
{
    char            buf[30];

    snprintf(buf, sizeof(buf), "\"%%%d$\"", hole_index);
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a system call
         * reports a problem with a printf(3)-style format string,
         * in the case where a %n$ specifier has not been used.
         *
         * %1$s => the name of the offending system-call argument.
         * %2$s => the argument position of the missing format specifier,
         *         already quoted (including percent and dollar sign).
         */
        i18n("the %s argument does not contain a %s specification"),
        caption,
        buf
    );
}


/* vim: set ts=8 sw=4 et : */
