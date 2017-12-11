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

#include <libexplain/buffer/addrinfo_flags.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static const explain_parse_bits_table_t table[] =
{
    { "AI_PASSIVE", AI_PASSIVE },
#ifdef AI_CANONNAME
    { "AI_CANONNAME", AI_CANONNAME },
#endif
#ifdef AI_NUMERICHOST
    { "AI_NUMERICHOST", AI_NUMERICHOST },
#endif
#ifdef AI_V4MAPPED
    { "AI_V4MAPPED", AI_V4MAPPED },
#endif
#ifdef AI_ALL
    { "AI_ALL", AI_ALL },
#endif
#ifdef AI_ADDRCONFIG
    { "AI_ADDRCONFIG", AI_ADDRCONFIG },
#endif
#ifdef AI_IDN
    { "AI_IDN", AI_IDN },
#endif
#ifdef AI_CANONIDN
    { "AI_CANONIDN", AI_CANONIDN },
#endif
#ifdef AI_IDN_ALLOW_UNASSIGNED
    { "AI_IDN_ALLOW_UNASSIGNED", AI_IDN_ALLOW_UNASSIGNED },
#endif
#ifdef AI_IDN_USE_STD3_ASCII_RULES
    { "AI_IDN_USE_STD3_ASCII_RULES", AI_IDN_USE_STD3_ASCII_RULES },
#endif
#ifdef AI_NUMERICSERV
    { "AI_NUMERICSERV", AI_NUMERICSERV },
#endif
};


void
explain_buffer_addrinfo_flags(explain_string_buffer_t *sb, int value)
{
    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


int
explain_parse_addrinfo_flags_or_die(const char *text,
    const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
