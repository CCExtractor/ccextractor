/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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
explain_buffer_einval_bits(explain_string_buffer_t *sb,
    const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining an EINVAL
         * error returned by a system call that is complaining about
         * undefined bits in a bitfield argument, e.g. access(2).
         *
         * %1$s => the name of the offending system call argument
         */
        i18n("the %s argument was incorrectly specified, it contained "
            "undefined bits"),
        caption
    );
}


void
explain_buffer_einval_too_small(explain_string_buffer_t *sb,
    const char *caption, long value)
{
    if (value < 0)
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used when explaining an EINVAL
             * error returned by a system call that is complaining about a
             * size being too small or negative (e.g. bind's sock_addr_size
             * field).
             *
             * %1$s => the name of the offending system call argument
             */
            i18n("the %s argument was incorrectly specified, it was negative"),
            caption
        );
    }
    else
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used when explaining an EINVAL
             * error returned by a system call that is complaining about a
             * size being too small or negative (e.g. bind's sock_addr_size
             * field).
             *
             * %1$s => the name of the offending system call argument
             */
            i18n("the %s argument was incorrectly specified, it was too small"),
            caption
        );
    }
}


void
explain_buffer_einval_vague(explain_string_buffer_t *sb,
    const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when an argument of a
         * system call results in an EINVAL error being reported.
         *
         * %1$s => the name of the offending system call argument.
         */
        i18n("the %s argument was incorrectly specified"),
        caption
    );
}


void
explain_buffer_einval_value(explain_string_buffer_t *sb,
    const char *caption, long value)
{
    explain_buffer_einval_vague(sb, caption);
    explain_string_buffer_printf(sb, " (%ld)", value);
}


void
explain_buffer_einval_not_a_number(explain_string_buffer_t *sb,
    const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a call to
         * strtol is given a string containing no digits.
         * Similarly for related functions.
         *
         * %1$s => the name of the offending system call argument
         */
        i18n("the %s argument does not appear to be a number"),
        caption
    );
}


/* vim: set ts=8 sw=4 et : */
