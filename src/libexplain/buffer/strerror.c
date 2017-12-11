/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/string.h>

#include <libexplain/buffer/strerror.h>
#include <libexplain/errno_info.h>
#include <libexplain/gettext.h>
#include <libexplain/option.h>
#include <libexplain/string_buffer.h>


void
explain_buffer_strerror(explain_string_buffer_t *sb, int errnum)
{
    const explain_errno_info_t *eip;
    int             first;
    char            errbuf[1024];

    explain_internal_strerror_r(errnum, errbuf, sizeof(errbuf));
    if (!errbuf[0])
    {
        const char      *no_idea;

        no_idea =
            explain_gettext
            (
                /*
                 * xgettext: This message is used when streror (or strerror_r)
                 * is unable to translate an errno value, in which ase this
                 * fall-back message is used.  This does not occur with glibc,
                 * but other libc implemntations are more flakey.
                 */
                i18n("unknown system error")
            );
        explain_strendcpy(errbuf, no_idea, errbuf + sizeof(errbuf));
    }
    explain_string_buffer_puts(sb, errbuf);
    first = 1;
    if (explain_option_numeric_errno())
    {
        explain_string_buffer_printf(sb, " (%d", errnum);
        first = 0;
    }
    eip = explain_errno_info_by_number(errnum);
    if (eip)
    {
        if (first)
            explain_string_buffer_puts(sb, " (");
        else
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, eip->name);
        first = 0;
    }
    if (!first)
        explain_string_buffer_putc(sb, ')');
}


/* vim: set ts=8 sw=4 et : */
