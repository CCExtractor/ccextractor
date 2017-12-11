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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/gettext.h>


static void
report_error(explain_string_buffer_t *sb, const char *caption)
{
    explain_buffer_gettext_printf
    (
        sb,
        /*
         * xgettext: This explanation is used in response to an ENOENT
         * error.  This explanation is only used if a more specific
         * cause cannot be determined.
         *
         * %1$s => the name of the offending system call argument.
         * %2$s => always identical to the above.
         */
        i18n("%s, or a directory component of %s, does not exist or is a "
            "dangling symbolic link"),
        caption,
        caption
    );
}


static void
report_error_dirname(explain_string_buffer_t *sb, const char *caption)
{
    explain_buffer_gettext_printf
    (
        sb,
        /*
         * xgettext: This explanation is used in response to an ENOENT
         * error.  This explanation is only used if a more specific
         * cause cannot be determined.
         *
         * %1$s => the name of the offending system call argument.
         */
        i18n("a directory component of %s does not exist or is a dangling "
            "symbolic link"),
        caption
    );
}


void
explain_buffer_enoent(explain_string_buffer_t *sb, const char *pathname,
    const char *pathname_caption, const explain_final_t *final_component)
{
    if
    (
        explain_buffer_errno_path_resolution
        (
            sb,
            ENOENT,
            pathname,
            pathname_caption,
            final_component
        )
    )
    {
        /*
         * Unable to find a specific cause,
         * emit the generic explanation.
         */
        if (final_component->must_exist)
            report_error(sb, pathname_caption);
        else
            report_error_dirname(sb, pathname_caption);
    }
}


void
explain_buffer_enoent2(explain_string_buffer_t *sb,
    const char *oldpath, const char *oldpath_caption,
    const explain_final_t *oldpath_final_component,
    const char *newpath, const char *newpath_caption,
    const explain_final_t *newpath_final_component)
{
    if
    (
        explain_buffer_errno_path_resolution
        (
            sb,
            ENOENT,
            oldpath,
            oldpath_caption,
            oldpath_final_component
        )
    &&
        explain_buffer_errno_path_resolution
        (
            sb,
            ENOENT,
            newpath,
            newpath_caption,
            newpath_final_component
        )
    )
    {
        char            oldpath_or_newpath[100];

        snprintf
        (
            oldpath_or_newpath,
            sizeof(oldpath_or_newpath),
            "%s or %s",
            oldpath_caption,
            newpath_caption
        );

        /*
         * Unable to find a specific cause,
         * emit the generic explanation.
         */
        if
        (
            oldpath_final_component->must_exist
        ||
            newpath_final_component->must_exist
        )
        {
            report_error(sb, oldpath_or_newpath);
        }
        else
        {
            report_error_dirname(sb, oldpath_or_newpath);
        }
    }
}


/* vim: set ts=8 sw=4 et : */
