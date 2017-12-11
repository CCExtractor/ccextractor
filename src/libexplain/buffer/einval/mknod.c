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

#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/more_appropriate.h>


void
explain_buffer_einval_mknod(explain_string_buffer_t *sb, int mode,
    const char *mode_caption, const char *syscall_name)
{
    char            filtyp[100];
    explain_string_buffer_t filtyp_sb;

    (void)mode_caption;
    explain_string_buffer_init(&filtyp_sb, filtyp, sizeof(filtyp));
    explain_buffer_file_type(&filtyp_sb, mode);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a system call
         * reports an EINVAL error, in the case where mkdir was trying
         * to create an inappropriate kind of file node.
         */
        i18n("the %s system call cannot be used to create a %s"),
        syscall_name,
        filtyp
    );

    switch (mode & S_IFMT)
    {
    case S_IFDIR:
        explain_buffer_more_appropriate_syscall(sb, "mkdir");
        break;

    case S_IFLNK:
        explain_buffer_more_appropriate_syscall(sb, "symlink");
        break;

#ifdef S_IFSOCK
    case S_IFSOCK:
        explain_buffer_more_appropriate_syscall(sb, "socket");
        break;
#endif

#ifdef S_IFIFO
    case S_IFIFO:
        explain_buffer_more_appropriate_syscall(sb, "bind");
        break;
#endif

    default:
        break;
    }
}


/* vim: set ts=8 sw=4 et : */
