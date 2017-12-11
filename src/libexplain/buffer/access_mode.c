/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/access_mode.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>
#include <libexplain/string_buffer.h>


static const explain_parse_bits_table_t table[] =
{
    { "F_OK", F_OK },
    { "R_OK", R_OK },
    { "W_OK", W_OK },
    { "X_OK", X_OK },
};


void
explain_buffer_access_mode(explain_string_buffer_t *sb, int mode)
{
    if (mode == 0)
        explain_string_buffer_puts(sb, "F_OK");
    else
        explain_parse_bits_print(sb, mode, table, SIZEOF(table));
}


int
explain_access_mode_parse(const char *text)
{
    int             result;

    result = -1;
    explain_parse_bits(text, table, SIZEOF(table), &result);
    return result;
}


int
explain_access_mode_parse_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
