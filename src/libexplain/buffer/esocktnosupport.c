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

#include <libexplain/buffer/esocktnosupport.h>
#include <libexplain/buffer/socket_type.h>


void
explain_buffer_esocktnosupport(explain_string_buffer_t *sb,
        const char *syscall_name, int fildes)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a system call
         * reports an ESOCKTNOSUPPORT error.
         *
         * %1$s => the name of the offending system call
         */
        i18n("%s is not supported by the network type"),
        syscall_name
    );
    explain_buffer_socket_type_from_fildes(sb, fildes);
}


/* vim: set ts=8 sw=4 et : */
