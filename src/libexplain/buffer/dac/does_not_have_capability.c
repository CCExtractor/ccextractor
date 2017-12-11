/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/buffer/dac.h>


void
explain_buffer_does_not_have_capability(explain_string_buffer_t *sb,
    const char *name)
{
    explain_string_buffer_puts(sb, " (");
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when further explaining a
         * "process is not privileged" message, to include the specific
         * absent capability.
         *
         * %1$s => the name of the capability,
         *         e.g. "CAP_FOWNER"
         */
        i18n("does not have the %s capability"),
        name
    );
    explain_string_buffer_putc(sb, ')');
}


/* vim: set ts=8 sw=4 et : */
