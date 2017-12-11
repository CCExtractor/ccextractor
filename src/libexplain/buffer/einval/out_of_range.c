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

#include <libexplain/buffer/einval.h>
#include <libexplain/option.h>


void
explain_buffer_einval_out_of_range(explain_string_buffer_t *sb,
    const char *caption, long lo, long hi)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a system call
         * reports a EINVAL error, in the case where an argument's value
         * is outside the valid range.
         *
         * %1$s => the name of the offending system call argument
         */
        i18n("the value of the %s argument is outside the valid range"),
        caption
    );
    if (explain_option_dialect_specific())
        explain_string_buffer_printf(sb, " (%ld..%ld)", lo, hi);
}


/* vim: set ts=8 sw=4 et : */
