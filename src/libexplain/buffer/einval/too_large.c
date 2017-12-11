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

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/gettext.h>


void
explain_buffer_einval_too_large(explain_string_buffer_t *sb,
    const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining an EINVAL
         * error returned by a system call that is complaining about a
         * size being too large.
         *
         * %1$s => the name of the offending system call argument
         */
        i18n("the %s argument was incorrectly specified, it was too large"),
        caption
    );
}


/* vim: set ts=8 sw=4 et : */
