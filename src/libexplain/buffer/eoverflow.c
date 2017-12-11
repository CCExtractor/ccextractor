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

#include <libexplain/buffer/eoverflow.h>
#include <libexplain/buffer/gettext.h>


void
explain_buffer_eoverflow(explain_string_buffer_t *sb)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext:  This error message is issued to explain an
         * EOVERFLOW error.  This is the generic form, some system calls
         * have much more specific EOVERFLOW explaianions, and those
         * should be used if preference whenever possible.
         */
        i18n("some values were too large to be represented in the returned "
            "struct")
    );
}


/* vim: set ts=8 sw=4 et : */
