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

#include <libexplain/buffer/enospc.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/mount_point.h>


void
explain_buffer_enospc(explain_string_buffer_t *sb, const char *pathname,
    const char *pathname_caption)
{
    char            mntpt[100];
    explain_string_buffer_t mntpt_sb;

    explain_string_buffer_init(&mntpt_sb, mntpt, sizeof(mntpt));
    if (explain_buffer_mount_point(&mntpt_sb, pathname) < 0)
        explain_buffer_mount_point_dirname(&mntpt_sb, pathname);
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used to provide an
         * explanation for and ENOSPC error returned by an
         * open(2) system call, in the case where there is no
         * more room for a new file.
         *
         * %1$s => The name of the problematic system call argument
         * %2$s => The file system mount point and usage,
         *         in parentheses
         */
        i18n("the file system containing %s %s has no space for a new "
            "directory entry"),
        pathname_caption,
        mntpt
    );
    /* FIXME: has no blocks left or has no inodes left? */
    /* FIXME: ENOSPC can be caused by quota system, too. */
}


void
explain_buffer_enospc_fildes(explain_string_buffer_t *sb, int fildes,
    const char *fildes_caption)
{
    struct stat     st;

    if (fstat(fildes, &st) == 0)
    {
        if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode))
        {
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext:  This error message is used to explain an
                 * ENOSPC error, in the case where a device has no space
                 * for more data.
                 *
                 * %1$s => The name of the offending syscall argument.
                 */
                i18n("the device referred to by %s has no more space for data"),
                fildes_caption
            );
        }
        else
        {
            char            mntpt[100];
            explain_string_buffer_t mntpt_sb;

            explain_string_buffer_init(&mntpt_sb, mntpt, sizeof(mntpt));
            explain_buffer_mount_point_stat(&mntpt_sb, &st);

            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext:  This error message is used to explain an
                 * ENOSPC error, in the case where a file system has no
                 * room to increase the size of a file.
                 *
                 * %1$s => The name of the problematic system call argument
                 * %2$s => The file system mount point and usage,
                 *         in parentheses
                 */
                i18n("the file system containing %s %s has no more space for "
                    "data"),
                fildes_caption,
                mntpt
            );
        }
    }
    else
    {
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This error message is used to explain an
             * ENOSPC error, in the case where more specific information
             * is not available.
             */
            i18n("the device containing the file referred to by the file "
                "descriptor has no space for the data; or, the file system "
                "containing the file has no space for the data")
        );
    }
}


/* vim: set ts=8 sw=4 et : */
