/*
 * libexplain - Explain errno values returned by libc functions
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

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/eexist.h>
#include <libexplain/dirname.h>
#include <libexplain/option.h>


void
explain_buffer_eexist_tempname(explain_string_buffer_t *sb,
    const char *directory)
{
    char            qdir[PATH_MAX + 3];
    explain_string_buffer_t qdir_sb;

    explain_string_buffer_init(&qdir_sb, qdir, sizeof(qdir));
    explain_string_buffer_puts_quoted(&qdir_sb, directory);

    if (explain_option_dialect_specific())
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext:  This error message is issued when we
             * are unable to locate a unique temporary file.
             *
             * %1$s => The directory used to hold temporary files.
             * %2$d => the number of attempts (TMP_MAX)
             */
            i18n("the system was unable to find a unique unused temporary file "
                "name in the %s directory, after %d attempts"),
            qdir,
            (int)(TMP_MAX)
        );
    }
    else
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext:  This error message is issued when we
             * are unable to locate a unique temporary file.
             *
             * %1$s => The directory used to hold temporary files.
             */
            i18n("the system was unable to find a unique unused temporary file "
                "name in the %s directory"),
            qdir
        );
    }
}


void
explain_buffer_eexist_tempname_dirname(explain_string_buffer_t *sb,
    const char *child)
{
    char            parent[PATH_MAX + 1];

    explain_dirname(parent, child, sizeof(parent));
    explain_buffer_eexist_tempname(sb, parent);
}


/* vim: set ts=8 sw=4 et : */
