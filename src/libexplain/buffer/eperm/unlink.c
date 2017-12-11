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
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/dirname.h>


void
explain_buffer_eperm_unlink(explain_string_buffer_t *sb, const char *pathname,
    const char *pathname_caption, const char *syscall_name)
{
    struct stat     pathname_st;
    struct stat     dir_st;
    char            dir[PATH_MAX];

    explain_dirname(dir, pathname, sizeof(dir));
    if
    (
        stat(dir, &dir_st) >= 0
    &&
        (dir_st.st_mode & S_ISVTX)
    &&
        geteuid() != dir_st.st_uid
    &&
        stat(pathname, &pathname_st) >= 0
    &&
        geteuid() != pathname_st.st_uid
    )
    {
        explain_string_buffer_t proc_uid_sb;
        explain_string_buffer_t pathname_uid_sb;
        explain_string_buffer_t dir_uid_sb;
        explain_string_buffer_t dir_quoted_sb;
        char            proc_uid[50];
        char            pathname_uid[50];
        char            dir_uid[50];
        char            dir_quoted[PATH_MAX];

        explain_string_buffer_init(&dir_quoted_sb, dir_quoted,
            sizeof(dir_quoted));
        explain_string_buffer_puts_quoted(&dir_quoted_sb, dir);

        explain_string_buffer_init(&proc_uid_sb, proc_uid,
            sizeof(proc_uid));
        explain_buffer_uid(&proc_uid_sb, geteuid());

        explain_string_buffer_init(&pathname_uid_sb, pathname_uid,
            sizeof(pathname_uid));
        explain_buffer_uid(&pathname_uid_sb, pathname_st.st_uid);

        explain_string_buffer_init(&dir_uid_sb, dir_uid,
            sizeof(dir_uid));
        explain_buffer_uid(&dir_uid_sb, dir_st.st_uid);

        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext:  This error message is used to explain an EPERM error
             * reported by the unlink(2) system call, in the case where the
             * directory containing pathname has the sticky bit (S_ISVTX) set
             * and the process's effective UID is neither the UID of the file to
             * be deleted nor that of the directory containing it.
             *
             * %1$s => The path for the directory containing the file ot be
             *         unlinked, already quoted.
             * %2$s => The process's effective UID, and user name (already
             *         quoted) if available
             * %3$s => The file to be deleted's effective UID, and user name
             *         (already quoted) if available
             * %4$s => The directory's effective UID, and user name (already
             *         quoted) if available
             */
            i18n("the directory containing pathname (%s) has the "
                "sticky bit (S_ISVTX) set and the process's effective "
                "UID (%s) is neither the UID of the file to be deleted "
                "(%s) nor that of the directory containing it (%s)"),
            dir_quoted,
            proc_uid,
            pathname_uid,
            dir_uid
        );
    }
    else
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext:  This error message is used to explain an EPERM
             * error reported by the unlink(2) system call, in the case
             * where the file system does not allow unlinking of files;
             * or, the directory containing pathname has the sticky
             * bit (S_ISVTX) set and the process's effective UID is
             * neither the UID of the file to be deleted nor that of the
             * directory containing it.
             *
             * $1$s => the name of the offending system call argument
             * $2$s => the name of the offended system call
             */
            i18n("the %s does not refer to a file system object to which %s "
                "may be applied; or, the directory containing pathname has "
                "the sticky bit (S_ISVTX) set and the process's effective UID "
                "is neither the UID of the file to be deleted nor that of the "
                "directory containing it"),
            pathname_caption,
            syscall_name
        );
    }
    explain_buffer_dac_fowner(sb);
}


/* vim: set ts=8 sw=4 et : */
