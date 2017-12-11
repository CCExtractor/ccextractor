/*
 * libexplain - a library of system-call-specific strerror replacements
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

#include <libexplain/buffer/must_be_multiple_of_page_size.h>
#include <libexplain/getpagesize.h>


void
explain_buffer_must_be_multiple_of_page_size(explain_string_buffer_t *sb,
    const char *caption)
{
    int             n;

    explain_string_buffer_printf_gettext
    (
        sb,
        /**
          * xgettext: This message is used to explain when a system call
          * argument is required to be a multipe of the page size, but is not.
          */
        i18n("the %s must be a multiple of the page size"),
        caption
    );

    n = explain_getpagesize();
    if (n > 0)
        explain_string_buffer_printf(sb, " (%d)", n);
}


/* vim: set ts=8 sw=4 et : */
