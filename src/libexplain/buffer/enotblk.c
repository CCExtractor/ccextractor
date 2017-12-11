/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/enotblk.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/wrong_file_type.h>


void
explain_buffer_enotblk(explain_string_buffer_t *sb, const char *pathname,
    const char *caption)
{
    struct stat st;
    explain_buffer_wrong_file_type_st
    (
        sb,
        (stat(pathname, &st) >= 0 ? &st : (struct stat *)NULL),
        caption,
        S_IFBLK
    );
}


/* vim: set ts=8 sw=4 et : */
