/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2012, 2013 Peter Miller
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

#include <libexplain/ac/fcntl.h> /* for AT_FDCWD */
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */

#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/fileinfo.h>


int
explain_buffer_errno_path_resolution_at(explain_string_buffer_t *sb,
    int errnum, int fildes, const char *pathname, const char *pathname_caption,
    const explain_final_t *final_component)
{
    char            path2[PATH_MAX + 1];
    explain_string_buffer_t sb2;

    /*
     * The explain_buffer_erro_path_resolution function doesn't grok
     * "relative to fildes" so turn the fildes into a path, and build a
     * path that the path_resolution code does understand.
     */
    explain_string_buffer_init(&sb2, path2, sizeof(path2));
    if (fildes == AT_FDCWD || pathname[0] == '/')
    {
        explain_string_buffer_puts(&sb2, pathname);
    }
    else
    {
        char            path3[PATH_MAX + 1];

        if (!explain_fileinfo_self_fd_n(fildes, path3, sizeof(path3)))
            return -1;
        explain_string_buffer_puts(&sb2, path3);
        explain_string_buffer_path_join(&sb2, pathname);
    }

    return
        explain_buffer_errno_path_resolution
        (
            sb,
            errnum,
            path2,
            pathname_caption,
            final_component
        );
}


/* vim: set ts=8 sw=4 et : */
