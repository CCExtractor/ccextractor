/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/ac/string.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/software_error.h>


void
explain_buffer_einval_mkstemp(explain_string_buffer_t *sb, const char *pathname,
    const char *pathname_caption)
{
    /*
     * The template was too small; or, last six characters of template
     * were not "XXXXXX".
     */
    if (strlen(pathname) < 6)
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext:  This error message is issued to explain an
             * EINVAL error reported by the mkstemp system call, in the
             * case where the file name template is too small.
             *
             * %1$s => The name of the offending system call argument.
             */
            i18n("the %s is too small, it must be at least six "
                "characters"),
            pathname_caption
        );
    }
    else
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext:  This error message is issued to explain an
             * EINVAL error reported by the mkstemp system call, in
             * the case where the file name template does not end in
             * "XXXXXX".
             *
             * %1$s => The name of the offending system call argument.
             */
            i18n("the last six characters of the %s were not "
                "\"XXXXXX\""),
            pathname_caption
        );
    }
    explain_buffer_software_error(sb);
}


/* vim: set ts=8 sw=4 et : */
