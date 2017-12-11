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

#include <libexplain/buffer/ewouldblock.h>


void
explain_buffer_eagain(explain_string_buffer_t *sb, const char *caption)
{
    explain_buffer_ewouldblock(sb, caption);
}


void
explain_buffer_ewouldblock(explain_string_buffer_t *sb, const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: this error message is issued to explain an
         * EWOULDBLOCK error.  It is a generic response, suitable for
         * many system calls, however there are also some more specific
         * EWOULDBLOCK messages that contributors should check for
         * re-usability before using this one.
         *
         * %1$s => the name of the system call
         */
        i18n("the %s would have had to wait to complete however it was "
            "instructed not to do so"),
        caption
    );
}


void
explain_buffer_eagain_accept(explain_string_buffer_t *sb)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext:  This message is used to explain an EAGAIN
         * error reprted by an accept(2) system call, in the case
         * where the socket is marked non-blocking (O_NONBLOCK) and
         * no connections are waiting to be accepted.
         */
        i18n("the socket is marked non-blocking and no connections "
            "are present to be accepted")
    );
}


/* vim: set ts=8 sw=4 et : */
