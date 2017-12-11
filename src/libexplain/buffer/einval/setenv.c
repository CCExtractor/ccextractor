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

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/is_the_null_pointer.h>


void
explain_buffer_einval_setenv(explain_string_buffer_t *sb, const char *name,
    const char *name_caption)
{
    if (name == NULL)
    {
        explain_buffer_is_the_null_pointer(sb, name_caption);
    }
    else if (*name == '\0')
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used when a system call
             * argument is passed an emoty string, and it should not be.
             *
             * %1$s => The name of the system call's offending argument.
             */
            i18n("%s is the empty string, and it should not be"),
            name_caption
        );
    }
    else
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used when a system call
             * argument is passed a string containing an equals ('=')
             * character, when it should not.
             *
             * %1$s => The name of the system call's offending argument.
             */
            i18n("%s contains an equals ('=') character, and it should not"),
            name_caption
        );
    }
}


/* vim: set ts=8 sw=4 et : */
