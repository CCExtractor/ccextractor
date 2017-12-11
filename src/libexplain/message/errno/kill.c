/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/types.h>

#include <libexplain/buffer/errno/kill.h>
#include <libexplain/kill.h>


void
explain_message_errno_kill(char *message, int message_size, int errnum, pid_t
    pid, int sig)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_kill(&sb, errnum, pid, sig);
}


/* vim: set ts=8 sw=4 et : */
