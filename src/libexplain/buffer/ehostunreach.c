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

#include <libexplain/buffer/ehostunreach.h>
#include <libexplain/buffer/gettext.h>


void
explain_buffer_ehostunreach(explain_string_buffer_t *sb)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This eror message is issued to explain an
         * EHOSTUNREACH error.
         */
        i18n("the remote host does not appear to be connected to the "
            "network or is turned off")
    );
}


/* vim: set ts=8 sw=4 et : */
