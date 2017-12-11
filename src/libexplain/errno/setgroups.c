/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
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

#include <libexplain/ac/unistd.h>

#include <libexplain/common_message_buffer.h>
#include <libexplain/setgroups.h>


const char *
explain_errno_setgroups(int errnum, size_t data_size, const gid_t *data)
{
    explain_message_errno_setgroups(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, data_size, data);
    return explain_common_message_buffer;
}


/* vim: set ts=8 sw=4 et : */
