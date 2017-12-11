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

#include <libexplain/buffer/erange.h>
#include <libexplain/buffer/gettext.h>


void
explain_buffer_erange(explain_string_buffer_t *sb)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a call to a
         * function would result in a return value that cannot be
         * represented by the return data type.
         *
         * The explanation has to make sense to a user without a
         * mathematical background, so saying things like "magnitude
         * exceeds data type representation limits" doesn't cut it.
         */
        i18n("the resulting value would have been too large to store")
    );
}


/* vim: set ts=8 sw=4 et : */
