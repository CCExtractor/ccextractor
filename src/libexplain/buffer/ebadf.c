/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013, 2014 Peter Miller
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

#include <libexplain/ac/fcntl.h>

#include <libexplain/buffer/check_fildes_range.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/software_error.h>
#include <libexplain/gettext.h>


void
explain_buffer_ebadf(explain_string_buffer_t *sb, int fildes,
    const char *caption)
{
    if (explain_buffer_check_fildes_range(sb, fildes, caption) >= 0)
    {
        /* all done */
    }
    else if (fcntl(fildes, F_GETFL, 0) < 0)
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used when a file descriptor is not
             * valid and does not refer to an open file.
             *
             * %1$s => the name of the offending system call argument.
             */
            i18n("the %s argument does not refer to an open file"),
            caption
        );
    }
    else
    {
        explain_buffer_einval_vague(sb, caption);
    }
    explain_buffer_software_error(sb);
}


void
explain_buffer_ebadf_stream(explain_string_buffer_t *sb, const char *caption)
{
    /*
     * Linux man fileno says:
     * "This function should not fail and thus not set errno.
     * However, in case fileno() detects that its argument is not a
     * valid stream, it must return -1 and set errno to EBADF."
     */
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a FILE* pointer
         * does not refer to a valid file stream.
         *
         * %1$s => the name of the offending system call argument
         */
        i18n("the %s argument does not refer a valid file stream"),
        caption
    );
}


void
explain_buffer_ebadf_dir(explain_string_buffer_t *sb, const char *caption)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a DIR* pointer
         * does not refer to a valid directory stream.
         *
         * %1$s => the name of the offending system call argument
         */
        i18n("the %s argument does not refer a valid directory stream"),
        caption
    );
}


/* vim: set ts=8 sw=4 et : */
