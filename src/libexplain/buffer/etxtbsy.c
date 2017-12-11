/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/buffer/etxtbsy.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/path_to_pid.h>


static void
report_error(explain_string_buffer_t *sb)
{
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a process attempts to
         * write to an executable file that is currently being executed.
         */
        i18n("write access was requested to an executable image that is "
        "currently being executed")
    );
}


void
explain_buffer_etxtbsy(explain_string_buffer_t *sb, const char *pathname)
{
    report_error(sb);
    explain_buffer_path_to_pid(sb, pathname);
}


void
explain_buffer_etxtbsy_fildes(explain_string_buffer_t *sb, int fildes)
{
    report_error(sb);
    explain_buffer_fildes_to_pid(sb, fildes);
}


/* vim: set ts=8 sw=4 et : */
