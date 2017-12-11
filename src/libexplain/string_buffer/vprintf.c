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

#include <libexplain/ac/stdio.h>
#include <libexplain/ac/string.h>

#include <libexplain/string_buffer.h>


void
explain_string_buffer_vprintf(explain_string_buffer_t *sb,
    const char *fmt, va_list ap)
{
    char *end = sb->message + sb->maximum;
    char *cp = sb->message + sb->position;
    vsnprintf(cp, end - cp, fmt, ap);
    sb->position += strlen(cp);
}


/* vim: set ts=8 sw=4 et : */
