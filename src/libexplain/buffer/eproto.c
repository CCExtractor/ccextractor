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

#include <libexplain/buffer/eproto.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/socket_type.h>


void
explain_buffer_eproto_accept(explain_string_buffer_t *sb, int fildes)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This message is used to explain and EPROTO error reported
         * by an accept(2) system call, in the case where a protocol error has
         * occurred.
         */
        i18n("a protocol error has occurred")
    );
    explain_buffer_socket_type_from_fildes(sb, fildes);
}


/* vim: set ts=8 sw=4 et : */
