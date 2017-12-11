/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/netdb.h>

#include <libexplain/buffer/gai_strerror.h>
#include <libexplain/buffer/strerror.h>
#include <libexplain/option.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static const explain_parse_bits_table_t table[] =
{
    { "EAI_BADFLAGS", EAI_BADFLAGS },
    { "EAI_NONAME", EAI_NONAME },
    { "EAI_AGAIN", EAI_AGAIN },
    { "EAI_FAIL", EAI_FAIL },
    { "EAI_FAMILY", EAI_FAMILY },
    { "EAI_SOCKTYPE", EAI_SOCKTYPE },
    { "EAI_SERVICE", EAI_SERVICE },
    { "EAI_MEMORY", EAI_MEMORY },
    { "EAI_SYSTEM", EAI_SYSTEM },
#ifdef EAI_OVERFLOW
    { "EAI_OVERFLOW", EAI_OVERFLOW },
#endif
#ifdef EAI_NODATA
    { "EAI_NODATA", EAI_NODATA },
#endif
#ifdef EAI_ADDRFAMILY
    { "EAI_ADDRFAMILY", EAI_ADDRFAMILY },
#endif
#ifdef EAI_INPROGRESS
    { "EAI_INPROGRESS", EAI_INPROGRESS },
#endif
#ifdef EAI_CANCELED
    { "EAI_CANCELED", EAI_CANCELED },
#endif
#ifdef EAI_NOTCANCELED
    { "EAI_NOTCANCELED", EAI_NOTCANCELED },
#endif
#ifdef EAI_ALLDONE
    { "EAI_ALLDONE", EAI_ALLDONE },
#endif
#ifdef EAI_INTR
    { "EAI_INTR", EAI_INTR },
#endif
#ifdef EAI_IDN_ENCODE
    { "EAI_IDN_ENCODE", EAI_IDN_ENCODE },
#endif
};


void
explain_buffer_gai_strerror(explain_string_buffer_t *sb, int errnum)
{
    const explain_parse_bits_table_t *tp;
    int             first;

    if (errnum > 0)
    {
        explain_buffer_strerror(sb, errnum);
        return;
    }

    explain_string_buffer_puts(sb, gai_strerror(errnum));

    first = 1;
    if (explain_option_numeric_errno())
    {
        explain_string_buffer_printf(sb, " (%d", errnum);
        first = 0;
    }
    tp = explain_parse_bits_find_by_value(errnum, table, SIZEOF(table));
    if (tp)
    {
        if (first)
            explain_string_buffer_puts(sb, " (");
        else
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, tp->name);
        first = 0;
    }
    if (!first)
        explain_string_buffer_putc(sb, ')');
}


/* vim: set ts=8 sw=4 et : */
