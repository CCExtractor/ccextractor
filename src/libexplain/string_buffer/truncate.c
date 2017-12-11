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

#include <libexplain/string_buffer.h>


void
explain_string_buffer_truncate(explain_string_buffer_t *sb,
    long new_position)
{
    size_t          new_size;

    if (sb->maximum == 0)
        return;
    new_size = new_position < 0 ? 0 : new_position;
    if (new_size < sb->position)
    {
        sb->position = new_size;
        sb->message[sb->position] = '\0';
    }
}


/* vim: set ts=8 sw=4 et : */
