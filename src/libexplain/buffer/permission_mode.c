/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/permission_mode.h>
#include <libexplain/option.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>
#include <libexplain/string_buffer.h>


static const explain_parse_bits_table_t short_table[] =
{
    { "S_ISUID", S_ISUID },
    { "S_ISGID", S_ISGID },
    { "S_ISVTX", S_ISVTX },
};


static const explain_parse_bits_table_t long_table[] =
{
    { "S_ISUID", S_ISUID },
    { "S_ISGID", S_ISGID },
    { "S_ISVTX", S_ISVTX },
    { "S_IRWXU", S_IRWXU },
    { "S_IRUSR", S_IRUSR },
    { "S_IWUSR", S_IWUSR },
    { "S_IXUSR", S_IXUSR },
    { "S_IRWXG", S_IRWXG },
    { "S_IRGRP", S_IRGRP },
    { "S_IWGRP", S_IWGRP },
    { "S_IXGRP", S_IXGRP },
    { "S_IRWXO", S_IRWXO },
    { "S_IROTH", S_IROTH },
    { "S_IWOTH", S_IWOTH },
    { "S_IXOTH", S_IXOTH },
};


void
explain_buffer_permission_mode(explain_string_buffer_t *sb, mode_t mode)
{
    const explain_parse_bits_table_t *table;
    const explain_parse_bits_table_t *table_end;
    const explain_parse_bits_table_t *tp;
    int             first;

    if (mode == 0)
    {
        explain_string_buffer_putc(sb, '0');
        return;
    }
    first = 1;
    table = short_table;
    table_end = short_table + SIZEOF(short_table);
    if (explain_option_symbolic_mode_bits())
    {
        table = long_table;
        table_end = long_table + SIZEOF(long_table);
    }
    for (tp = table; tp < table_end; ++tp)
    {
        unsigned value = tp->value;
        if (value != 0 && (mode & value) == value)
        {
            if (!first)
                explain_string_buffer_puts(sb, " | ");
            explain_string_buffer_puts(sb, tp->name);
            first = 0;
            mode -= value;
        }
    }
    if (mode != 0)
    {
        if (!first)
            explain_string_buffer_puts(sb, " | ");
        explain_string_buffer_printf(sb, "%#o", mode);
    }
}


mode_t
explain_permission_mode_parse_or_die(const char *text, const char *caption)
{
    return
        explain_parse_bits_or_die
        (
            text,
            long_table,
            SIZEOF(long_table),
            caption
        );
}


/* vim: set ts=8 sw=4 et : */
