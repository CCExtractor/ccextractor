/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/parse_bits.h>
#include <libexplain/string_to_thing.h>


static const explain_parse_bits_table_t table[] =
{
    { "yes", 1},
    { "no", 0},
    { "true", 1},
    { "false", 0},
};


int
explain_parse_bool_or_die(const char *text)
{
    return
        !!
        explain_parse_bits_or_die
        (
            text,
            table,
            SIZEOF(table),
            "bool on command line"
        );
}


/* vim: set ts=8 sw=4 et : */
