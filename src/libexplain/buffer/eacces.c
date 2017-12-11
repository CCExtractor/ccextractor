/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/errno/path_resolution.h>


void
explain_buffer_eacces(explain_string_buffer_t *sb, const char *pathname,
    const char *caption, const explain_final_t *final_component)
{
    if
    (
        explain_buffer_errno_path_resolution
        (
            sb,
            EACCES,
            pathname,
            caption,
            final_component
        )
    )
    {
        if (final_component->want_to_unlink || final_component->want_to_create)
        {
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used to explain an EACCES error,
                 * when attempting to create or remove a file, when
                 * path_resolution(7) can not find anything more specific.
                 *
                 * %1$s => the name of the problematic system call cargument
                 * %2$s => identical to the above
                 */
                i18n("search permission is denied for a directory component of "
                    "%s; or, the directory containing %s is not writable by "
                    "the user"),
                caption,
                caption
            );
        }
        else if (final_component->want_to_read)
        {
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used to explain an EACCES error,
                 * when attempting to read a file, when path_resolution(7) can
                 * not find anything more specific.
                 *
                 * %1$s => the name of the problematic system call cargument
                 * %2$s => identical to the above
                 */
                i18n("read access to %s was not allowed, or one of the "
                    "directory components of %s did not allow search "
                    "permission"),
                caption,
                caption
            );
        }
        else if
        (
            final_component->want_to_write
        ||
            final_component->want_to_modify_inode
        )
        {
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used to explain an EACCES error,
                 * when attempting to write a file, when path_resolution(7) can
                 * not find anything more specific.
                 *
                 * %1$s => the name of the problematic system call cargument
                 * %2$s => identical to the above
                 */
                i18n("write access to %s was not allowed, or one of the "
                    "directory components of %s did not allow search "
                    "permission"),
                caption,
                caption
            );
        }
        else
        {
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used to explain an EACCES error,
                 * when path_resolution(7) can not find anything more specific.
                 *
                 * %1$s => the name of the problematic system call cargument
                 */
                i18n("search permission is denied for a directory "
                    "component of %s"),
                caption
            );
        }
    }
}


/* vim: set ts=8 sw=4 et : */
