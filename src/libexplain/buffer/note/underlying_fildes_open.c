/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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

#include <libexplain/buffer/note/underlying_fildes_open.h>
#include <libexplain/buffer/gettext.h>


void
explain_buffer_note_underlying_fildes_open(explain_string_buffer_t *sb)
{
    explain_string_buffer_puts(sb->footnotes, "; ");
    explain_buffer_gettext
    (
        sb->footnotes,
        /*
         * xgettext:  This error message is used when an fclose(3) or freopen(3)
         * system call fails, and the underlying file descriptor may still be
         * open.
         */
        i18n("note that while the FILE stream is no longer valid, the "
        "underlying file descriptor may still be open")
    );
}


/* vim: set ts=8 sw=4 et : */
