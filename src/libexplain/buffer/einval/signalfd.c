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


void
explain_buffer_einval_signalfd(explain_string_buffer_t *sb, const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when the signalfd system
         * call reposrt an EINVAL error, in the case where the file descriptor
         * is actually open, but does not refer to a valid signalfd file
         * descriptor.
         *
         * %1$s => the name of the offending system call argument.
         */
        i18n("the %s argument does not refer to a valid signalfd "
            "file descriptor"),
        caption
    );
}


/* vim: set ts=8 sw=4 et : */
