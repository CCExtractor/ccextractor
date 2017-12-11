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

#include <libexplain/buffer/address_family.h>
#include <libexplain/buffer/eafnosupport.h>
#include <libexplain/fildes_to_address_family.h>


void
explain_buffer_eafnosupport(explain_string_buffer_t *sb, int fildes,
    const char *fildes_caption, const struct sockaddr *sock_addr,
    const char *sock_addr_caption)
{
    int             fildes_af;

    (void)sock_addr;
    fildes_af = explain_fildes_to_address_family(fildes);
    if (fildes_af >= 0)
    {
        char            aftxt[30];
        explain_string_buffer_t aftxt_sb;

        explain_string_buffer_init(&aftxt_sb, aftxt, sizeof(aftxt));
        explain_buffer_address_family(&aftxt_sb, fildes_af);
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used when socket(2) and
             * {bind(2) or connect(2)} disagree about the file
             * descriptor's address family.
             *
             * %1$s => The name of the system call argument containing
             *         the sockaddr with the erroneous address family.
             * %2$s => The name of the system call argument
             *         containing the socket file descriptor.
             * %3$s => The value of the socket file descriptor's
             *         address family, as obtained from the file
             *         descriptor itself.
             */
            i18n("%s does not have the correct address family, "
                "%s address family is %s"),
            sock_addr_caption,
            fildes_caption,
            aftxt
        );
    }
    else
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used when socket() and
             * connect() disagree about the file descriptor's
             * address family.  It is used when the file
             * descriptor's actual address family cannot be
             * determined.
             *
             * %1$s => The name of the system call argument containing
             *         the sockaddr with the erroneous address family.
             * %2$s => The name of the system call argument containing
             *         the file descriptor with the other address family.
             */
            i18n("%s does not have the same address family as %s"),
            sock_addr_caption,
            fildes_caption
        );
    }
}


/* vim: set ts=8 sw=4 et : */
