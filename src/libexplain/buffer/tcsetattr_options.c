/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/ac/termios.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/tcsetattr_options.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>
#include <libexplain/strtol.h>


#ifdef HAVE_TCSETATTR

static const explain_parse_bits_table_t table[] =
{
    { "TCSANOW", TCSANOW },
    { "TCSADRAIN", TCSADRAIN },
    { "TCSAFLUSH", TCSAFLUSH },
};


void
explain_buffer_tcsetattr_options(explain_string_buffer_t *sb, int data)
{
    explain_parse_bits_print_single(sb, data, table, SIZEOF(table));
}


int
explain_parse_tcsetattr_options_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}

#else

void
explain_buffer_tcsetattr_options(explain_string_buffer_t *sb, int data)
{
    explain_buffer_int(sb, data);
}


int
explain_parse_tcsetattr_options_or_die(const char *text, const char *caption)
{
    (void)caption;
    return explain_strtol_or_die(text, 0, 0);
}

#endif


/* vim: set ts=8 sw=4 et : */
