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

#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/gettext.h>


void
explain_buffer_enametoolong_gethostname(explain_string_buffer_t *sb,
    int actual_size, const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining an EINVAL or
         * ENAMETOOLONG error returned by the gethostname, getdomainname
         * (etc) system call, in the case where the supplied data buffer
         * is smaller than the actual value.
         *
         * %1$s => the name of the offending system call argument
         * %2$d => the minimum size (in bytes) needed to hold the actual
         *         value
         */
        i18n("the %s argument was incorrectly specified, the actual value "
            "requires at least %d bytes, or preferably use the HOST_NAME_MAX "
            "macro"),
        caption,
        actual_size + 1
    );
}


/* vim: set ts=8 sw=4 et : */
