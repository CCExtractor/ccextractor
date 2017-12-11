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

#include <libexplain/ac/netdb.h>

#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/hstrerror.h>
#include <libexplain/buffer/int.h>
#include <libexplain/errno_info.h>
#include <libexplain/option.h>
#include <libexplain/parse_bits.h>

static const explain_parse_bits_table_t table[] =
{
#ifdef HOST_NOT_FOUND
    { "HOST_NOT_FOUND", HOST_NOT_FOUND },
#endif
#ifdef TRY_AGAIN
    { "TRY_AGAIN", TRY_AGAIN },
#endif
#ifdef NO_RECOVERY
    { "NO_RECOVERY", NO_RECOVERY },
#endif
#ifdef NO_DATA
    { "NO_DATA", NO_DATA },
#endif
#ifdef NETDB_INTERNAL
    { "NETDB_INTERNAL", NETDB_INTERNAL },
#endif
#ifdef NETDB_SUCCESS
    { "NETDB_SUCCESS", NETDB_SUCCESS },
#endif
#ifdef NO_ADDRESS
    { "NO_ADDRESS", NO_ADDRESS },
#endif
};


static void
print_h_errno_name(explain_string_buffer_t *sb, int h_errno_value,
    int errno_value)
{
    const explain_parse_bits_table_t *tp;

    for (tp = table; tp < ENDOF(table); ++tp)
    {
        if (tp->value == h_errno_value)
        {
            explain_string_buffer_puts(sb, tp->name);
#ifdef NETDB_INTERNAL
            if (tp->value == NETDB_INTERNAL)
            {
                const explain_errno_info_t *eip;

                eip = explain_errno_info_by_number(errno_value);
                if (eip)
                {
                    explain_string_buffer_putc(sb, ' ');
                    explain_string_buffer_puts(sb, eip->name);
                }
            }
#endif
            return;
        }
    }
}


static void
print_h_errno_description(explain_string_buffer_t *sb, int h_errno_value,
    int errno_value)
{
    const explain_parse_bits_table_t *tp;

    for (tp = table; tp < ENDOF(table); ++tp)
    {
        if (tp->value == h_errno_value)
        {
            explain_string_buffer_puts(sb, hstrerror(h_errno_value));
#ifdef NETDB_INTERNAL
            if (tp->value == NETDB_INTERNAL)
            {
                const explain_errno_info_t *eip;

                eip = explain_errno_info_by_number(errno_value);
                if (eip)
                {
                    explain_string_buffer_putc(sb, ' ');
                    explain_string_buffer_puts(sb, eip->description);
                }
            }
#endif
            return;
        }
    }

    /* no idea */
    explain_buffer_gettext
    (
        sb,

        /*
         * xgettext: This message is used when hstreror is unable to translate
         * an h_errno value, in which causes this fall-back message to be used.
         */
        i18n("unknown <netdb.h> error")
    );
}


void
explain_buffer_hstrerror(explain_string_buffer_t *sb, int h_errno_value,
    int errno_value)
{
    print_h_errno_description(sb, h_errno_value, errno_value);
    explain_string_buffer_puts(sb, " (");
    if (explain_option_numeric_errno())
    {
        explain_buffer_int(sb, h_errno_value);
#ifdef NETDB_INTERNAL
        if (h_errno_value == NETDB_INTERNAL)
        {
            explain_string_buffer_putc(sb, ' ');
            explain_buffer_int(sb, errno_value);
        }
#endif
        explain_string_buffer_puts(sb, ", ");
    }
    print_h_errno_name(sb, h_errno_value, errno_value);
    explain_string_buffer_putc(sb, ')');
}


/* vim: set ts=8 sw=4 et : */
