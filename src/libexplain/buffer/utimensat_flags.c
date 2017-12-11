/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2012, 2013 Peter Miller
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

#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/utimensat_flags.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef AT_SYMLINK_NOFOLLOW
    { "AT_SYMLINK_NOFOLLOW", AT_SYMLINK_NOFOLLOW },
#endif
#ifdef AT_REMOVEDIR
    { "AT_REMOVEDIR", AT_REMOVEDIR },
#endif
#ifdef AT_SYMLINK_FOLLOW
    { "AT_SYMLINK_FOLLOW", AT_SYMLINK_FOLLOW },
#endif
#ifdef AT_NO_AUTOMOUNT
    { "AT_NO_AUTOMOUNT", AT_NO_AUTOMOUNT },
#endif
#ifdef AT_EMPTY_PATH
    { "AT_EMPTY_PATH", AT_EMPTY_PATH },
#endif
};


void
explain_buffer_utimensat_flags(explain_string_buffer_t *sb, int value)
{
    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


int
explain_parse_utimensat_flags_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


int
explain_parse_utimensat_flags(const char *text, int *result)
{
    return explain_parse_bits(text, table, SIZEOF(table), result);
}


int
explain_allbits_utimensat_flags(void)
{
    int result = 0;
    const explain_parse_bits_table_t *tp;
    for (tp = table; tp < ENDOF(table); ++tp)
        result |= tp->value;
    return result;
}


/* vim: set ts=8 sw=4 et : */
