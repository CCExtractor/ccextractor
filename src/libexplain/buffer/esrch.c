/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/esrch.h>
#include <libexplain/string_buffer.h>


void
explain_buffer_esrch(explain_string_buffer_t *sb, const char *caption,
    pid_t pid)
{
    if (pid >= 0)
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued when a system call
             * reports an ESRCH error, in the case where the pid was
             * positive.
             *
             * %1$s => the name of the offending system call argument.
             */
            i18n("the %s process does not exist"),
            caption
        );
    }
    else
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued when a system call
             * reports an ESRCH error, in the case where the pid was
             * negative.
             *
             * %1$s => the name of the offending system call argument.
             */
            i18n("the %s process group does not exist"),
            caption
        );
    }
}


/* vim: set ts=8 sw=4 et : */
