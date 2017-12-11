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

#include <libexplain/buffer/enotsup.h>
#include <libexplain/buffer/gettext.h>


void
explain_buffer_enotsup_device(explain_string_buffer_t *sb, const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a system call
         * reports an ENOTSUP error, in the case where a value (usually
         * something to do with an ioctl's data argument) is not
         * supported by the device.
         *
         * %1$s => The name of the offentding argument, with suffiucient
         *         detail about the data->member that a programmer would
         *         find it unambiguous.
         *
         */
        i18n("the %s argument is not supported by the device"),
        caption
    );
}


/* vim: set ts=8 sw=4 et : */
