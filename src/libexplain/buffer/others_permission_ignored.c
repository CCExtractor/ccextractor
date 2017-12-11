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

#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/others_permission_ignored.h>
#include <libexplain/buffer/rwx.h>


void
explain_buffer_others_permission_ignored(explain_string_buffer_t *sb, int mode)
{
    char            mode_text[10];
    explain_string_buffer_t mode_text_sb;

    explain_string_buffer_init(&mode_text_sb, mode_text, sizeof(mode_text));
    explain_buffer_rwx(&mode_text_sb, mode & S_IRWXO);
    explain_string_buffer_puts(sb, ", ");
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining why
         * the "other" permission mode bits are ignored.
         *
         * %1$s => the "rwx" bits, including the quotes, like the
         *         3-character string used in 'ls -l' output.
         */
        i18n("the others permission mode %s is ignored"),
        mode_text
    );
}


/* vim: set ts=8 sw=4 et : */
