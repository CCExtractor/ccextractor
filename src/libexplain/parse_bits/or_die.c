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

#include <libexplain/ac/stdio.h>

#include <libexplain/common_message_buffer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/program_name.h>
#include <libexplain/string_buffer.h>
#include <libexplain/option.h>
#include <libexplain/output.h>


int
explain_parse_bits_or_die(const char *text,
    const explain_parse_bits_table_t *table, size_t table_size,
    const char *caption)
{
    int             result;

    result = -1;
    if (explain_parse_bits(text, table, table_size, &result) < 0)
    {
        explain_string_buffer_t sb;
        char            errstr[500];

        snprintf(errstr, sizeof(errstr), "%s", explain_parse_bits_get_error());
        explain_string_buffer_init
        (
            &sb,
            explain_common_message_buffer,
            explain_common_message_buffer_size
        );
        if (explain_option_assemble_program_name())
        {
            const char      *prog;

            prog = explain_program_name_get();
            if (prog && *prog)
            {
                explain_string_buffer_puts(&sb, prog);
                explain_string_buffer_puts(&sb, ": ");
            }
        }
        if (caption)
        {
            explain_string_buffer_puts(&sb, caption);
            explain_string_buffer_puts(&sb, ": ");
        }
        explain_string_buffer_puts(&sb, errstr);
        explain_output_message(explain_common_message_buffer);
        explain_output_exit_failure();
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
