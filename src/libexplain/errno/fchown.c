/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/common_message_buffer.h>
#include <libexplain/fchown.h>


const char *
explain_errno_fchown(int errnum, int fildes, int owner, int group)
{
    explain_message_errno_fchown
    (
        explain_common_message_buffer,
        explain_common_message_buffer_size,
        errnum,
        fildes,
        owner,
        group
    );
    return explain_common_message_buffer;
}


/* vim: set ts=8 sw=4 et : */
