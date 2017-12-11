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

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/gettext.h>


void
explain_buffer_einval_not_listening(explain_string_buffer_t *sb)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This message is used to explain an EINVAL error
         * reported by the accept(2) system call, in the case where
         * the file descriptor is actually a socket, but is not in a
         * state that permits the use of the accept(2) system call.
         */
        i18n("the socket is not listening for connections")
    );
}


/* vim: set ts=8 sw=4 et : */
