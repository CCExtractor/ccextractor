/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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

#include <libexplain/buffer/dangerous.h>


void
explain_buffer_dangerous_system_call(explain_string_buffer_t *sb,
    const char *syscall_name)
{
    sb = sb->footnotes;
    explain_string_buffer_puts(sb, "; ");
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext:  This error message is issued when a dangerous
         * system call is used.  (Not that the user can do anything
         * about it, of course, but it will eventually get into a bug
         * report.)
         *
         * %1$s => the name of the dangerous system call.
         */
        i18n("the %s system call is dangerous, a more secure alternative "
            "should be used"),
        syscall_name
    );
}


void
explain_buffer_dangerous_system_call2(explain_string_buffer_t *sb,
    const char *syscall_name, const char *suggested_alternative)
{
    sb = sb->footnotes;
    explain_string_buffer_puts(sb, "; ");
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext:  This error message is issued when a dangerous
         * system call is used.  (Not that the user can do anything
         * about it, of course, but it will eventually get into a bug
         * report.)
         *
         * %1$s => the name of the dangerous system call.
         * %2$s => the name of the alternative system call.
         */
        i18n("the %s system call is dangerous, the more secure %s system call "
            "should be used instead"),
        syscall_name,
        suggested_alternative
    );
}


/* vim: set ts=8 sw=4 et : */
