/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#include <libexplain/ac/pwd.h>

#include <libexplain/buffer/uid.h>
#include <libexplain/option.h>


void
explain_buffer_uid(explain_string_buffer_t *sb, int uid)
{
    explain_string_buffer_printf(sb, "%d", uid);
    if (uid >= 0 && explain_option_dialect_specific())
    {
        struct passwd   *pw;

        pw = getpwuid(uid);
        if (pw)
        {
            explain_string_buffer_putc(sb, ' ');
            explain_string_buffer_puts_quoted(sb, pw->pw_name);
        }
    }
}


/* vim: set ts=8 sw=4 et : */
