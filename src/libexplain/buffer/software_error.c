/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/software_error.h>


void
explain_buffer_software_error(explain_string_buffer_t *sb)
{
    sb = sb->footnotes;
    explain_string_buffer_puts(sb, "; ");
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This message is used to explain that an error the
         * user is reading is more likely to be a software bug than it
         * is to be use user's fault.  E.g. things like EBADF and EFAULT
         * that are clearly beyond the user's control.
         */
        i18n("this is more likely to be a software error (a bug) "
        "than it is to be a user error")
    );
}


/* vim: set ts=8 sw=4 et : */
