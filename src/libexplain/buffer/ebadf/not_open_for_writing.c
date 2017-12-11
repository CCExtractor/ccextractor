/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/open_flags.h>


void
explain_buffer_ebadf_not_open_for_writing(explain_string_buffer_t *sb,
    const char *caption, int flags)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when an attempt is made to write to a
         * file descriptor (or I/O stream) that was not opened for writing.  The
         * actual open mode will be printed separately.
         *
         * %1$s => the name of the offending argument
         */
        i18n("the %s argument does not refer to an object that is open for "
            "writing"),
        caption
    );
    if (flags >= 0)
    {
        explain_string_buffer_puts(sb, " (");
        explain_buffer_open_flags(sb, flags);
        explain_string_buffer_putc(sb, ')');
    }
}


/* vim: set ts=8 sw=4 et : */
