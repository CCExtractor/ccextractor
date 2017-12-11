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

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/capability.h>
#include <libexplain/string_buffer.h>


void
explain_buffer_eperm_mknod(explain_string_buffer_t *sb,
    const char *pathname, const char *pathname_caption, int mode)
{
    explain_string_buffer_t filtyp_sb;
    char            filtyp[100];

    explain_string_buffer_init(&filtyp_sb, filtyp, sizeof(filtyp));
    explain_buffer_file_type(&filtyp_sb, mode);

    /*
     * mode requested creation of something other than a regular file,
     * FIFO (named pipe), or Unix domain socket, and the caller is not
     * privileged (Linux: does not have the CAP_MKNOD capability); also
     * returned if the file system containing pathname does not support
     * the type of node requested.
     */
    if ((S_ISCHR(mode) || S_ISBLK(mode)) && !explain_capability_mknod())
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued when a system call
             * reports an EPERM error, in the case where a file node is
             * being created (e.g. mkdir or mknod).
             *
             * %1$s => The name of the offending syscall argument.
             * %2$s => The name of the mount point, in parentheses
             * %3$s => The type of node being created, already translated
             */
            i18n("the process does not have permission to create a %s"),
            filtyp
        );
        explain_buffer_dac_sys_mknod(sb);
    }
    else
    {
        explain_string_buffer_t mntpt_sb;
        char            mntpt[100];

        explain_string_buffer_init(&mntpt_sb, mntpt, sizeof(mntpt));
        explain_buffer_mount_point_dirname(&mntpt_sb, pathname);

        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued when a system call
             * reports an EPERM error, in the case where a file node is
             * being created (e.g. mkdir or mknod).
             *
             * %1$s => The name of the offending syscall argument.
             * %2$s => The name of the mount point, in parentheses
             * %3$s => The type of node being created, already translated
             */
            i18n("the file system containing %s %s does not support the "
                "creation of a %s"),
            pathname_caption,
            mntpt,
            filtyp
        );
    }
}


/* vim: set ts=8 sw=4 et : */
