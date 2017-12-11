/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/buffer/gid.h>
#include <libexplain/buffer/errno/chown.h>
#include <libexplain/buffer/errno/fchown.h>
#include <libexplain/buffer/errno/lchown.h>
#include <libexplain/buffer/errno/lchownat.h>
#include <libexplain/explanation.h>
#include <libexplain/fileinfo.h>


static void
explain_buffer_errno_lchownat_system_call(explain_string_buffer_t *sb, int
    errnum, int fildes, const char *pathname, int uid, int gid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "lchownat(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", uid = ");
    explain_buffer_uid(sb, uid);
    explain_string_buffer_puts(sb, ", gid = ");
    explain_buffer_gid(sb, gid);
    explain_string_buffer_putc(sb, ')');
}


static int
is_a_directory(int fildes)
{
    struct stat st;
    if (fildes == AT_FDCWD)
        return 1;
    return S_ISDIR(st.st_mode);
}


void
explain_buffer_errno_lchownat_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, int fildes, const char *pathname, int uid,
    int gid)
{
    if (is_a_directory(fildes))
    {
        if (!pathname || !*pathname)
            pathname = ".";
        if (pathname[0] == '/')
        {
            explain_buffer_errno_lchown_explanation(sb, errnum, syscall_name,
                pathname, uid, gid);
            return;
        }
        else if (fildes == AT_FDCWD && pathname[0] != '/')
        {
            explain_buffer_errno_lchown_explanation(sb, errnum, syscall_name,
                pathname, uid, gid);
        }
        else
        {
            char path2[PATH_MAX];
            explain_string_buffer_t path2_sb;
            char dir_of[PATH_MAX];
            explain_fileinfo_self_fd_n(fildes, dir_of, sizeof(dir_of));
            explain_string_buffer_init(&path2_sb, path2, sizeof(path2));
            explain_string_buffer_puts(&path2_sb, dir_of);
            explain_string_buffer_path_join(&path2_sb, pathname);
            explain_buffer_errno_lchown_explanation(sb, errnum, syscall_name,
                path2, uid, gid);
            return;
        }
    }
    else
    {
        /* not a dir => ENOTDIR */
        errnum = ENOTDIR;

        if (!pathname)
        {
            explain_buffer_errno_fchown_explanation(sb, errnum, syscall_name,
                fildes, uid, gid, "fildes");
            return;
        }
        else
        {
            if (pathname[0] == '/')
            {
                explain_buffer_errno_lchown_explanation(sb, errnum,
                    syscall_name, pathname, uid, gid);
                return;
            }
            else
            {
                char path2[PATH_MAX];
                explain_string_buffer_t path2_sb;
                char dir_of[PATH_MAX];
                explain_fileinfo_self_fd_n(fildes, dir_of, sizeof(dir_of));
                explain_string_buffer_init(&path2_sb, path2, sizeof(path2));
                explain_string_buffer_puts(&path2_sb, dir_of);
                explain_string_buffer_path_join(&path2_sb, pathname);
                explain_buffer_errno_lchown_explanation(sb, errnum,
                    syscall_name, path2, uid, gid);
                return;
            }
        }
    }
}


void
explain_buffer_errno_lchownat(explain_string_buffer_t *sb, int errnum, int
    fildes, const char *pathname, int uid, int gid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_lchownat_system_call(&exp.system_call_sb, errnum,
        fildes, pathname, uid, gid);
    explain_buffer_errno_lchownat_explanation(&exp.explanation_sb, errnum,
        "lchownat", fildes, pathname, uid, gid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
