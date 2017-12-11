/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/sys/socket.h>

#include <libexplain/buffer/eaddrinuse.h>
#include <libexplain/buffer/gettext.h>


void
explain_buffer_eaddrinuse(explain_string_buffer_t *sb, int fildes)
{
    int             reuseaddr;
    socklen_t       optlen;

    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This message is used as an explanation for an
         * EADDRINUSE error.  See connect(2) and bind(2) for more
         * information.
         */
        i18n("local address is already in use; or, the address was in "
        "use very recently")
    );
    optlen = sizeof(reuseaddr);
    if
    (
        getsockopt(fildes, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, &optlen) < 0
    ||
        !reuseaddr
    )
    {
        /*
         * Linux will only allow port re-use with the SO_REUSEADDR
         * option when this option was set both in the previous program
         * that performed a bind(2) to the port and in the program that
         * wants to re-use the port.
         *
         * Other unix dialects only require that the re-user use this
         * option.
         */
        explain_string_buffer_puts(sb, ", ");
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used when and EADDRINUSE error
             * is seen, and the socket does not have the SO_REUSEADDR
             * socket option enabled.  See socket(7) for more information.
             */
            i18n("the SO_REUSEADDR socket option can be used to shorten "
            "the wait")
        );
    }
}


/* vim: set ts=8 sw=4 et : */
