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
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/emlink.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enospc.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/linkat.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/exdev.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/utimensat_flags.h>
#include <libexplain/explanation.h>
#include <libexplain/fildes_is_dot.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_linkat_system_call(explain_string_buffer_t *sb, int errnum,
    int old_fildes, const char *old_path, int new_fildes, const char *new_path,
    int flags)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "linkat(old_fildes = ");
    explain_buffer_fildes(sb, old_fildes);
    explain_string_buffer_puts(sb, ", old_path = ");
    explain_buffer_pathname(sb, old_path);
    explain_string_buffer_puts(sb, ", new_fildes = ");
    explain_buffer_fildes(sb, new_fildes);
    explain_string_buffer_puts(sb, ", new_path = ");
    explain_buffer_pathname(sb, new_path);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_utimensat_flags(sb, flags);
    explain_string_buffer_putc(sb, ')');
}


static int
is_a_directory(int fildes)
{
    struct stat st;

    if (fildes == AT_FDCWD)
        return 1;
    if (fstat(fildes, &st) < 0)
        return -1;
    return !!S_ISDIR(st.st_mode);
}


static int
get_mode_at(int fildes, const char *pathname)
{
    struct stat     st;

    if (fstatat(fildes, pathname, &st, 0) < 0)
        return S_IFREG;
    return st.st_mode;
}


static void
resolve(int fildes, const char *path, char *full, size_t full_size)
{
    explain_string_buffer_t sb;
    explain_string_buffer_init(&sb, full, full_size);
    if (path[0] == '/' || explain_fildes_is_dot(fildes))
    {
        explain_string_buffer_puts(&sb, path);
        return;
    }
    if (getcwd(full, full_size))
        sb.position += strlen(full);
    else
        explain_string_buffer_putc(&sb, '.');
    explain_string_buffer_path_join(&sb, path);
}



void
explain_buffer_errno_linkat_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int old_fildes, const char *old_path,
    int new_fildes, const char *new_path, int flags)
{
    explain_final_t oldpath_fc;
    explain_final_t newpath_fc;
    char old_path_full[PATH_MAX];
    char new_path_full[PATH_MAX];

    resolve(old_fildes, old_path, old_path_full, sizeof(old_path_full));
    resolve(new_fildes, new_path, new_path_full, sizeof(new_path_full));

    explain_final_init(&oldpath_fc);
    explain_final_init(&newpath_fc);
    oldpath_fc.must_exist = 1;
    if (flags & AT_SYMLINK_FOLLOW)
        oldpath_fc.follow_symlink = 1;
    newpath_fc.must_exist = 0;
    newpath_fc.want_to_create = 1;
    newpath_fc.st_mode = get_mode_at(old_fildes, old_path);

    switch (errnum)
    {
    case EBADF:
        if (0 == is_a_directory(old_fildes))
        {
            explain_buffer_enotdir_fd(sb, old_fildes, "old_fildes");
            return;
        }
        if (0 == is_a_directory(new_fildes))
        {
            explain_buffer_enotdir_fd(sb, new_fildes, "new_fildes");
            return;
        }
        /* No idea... */
        break;

    case ENOTDIR:
        if (!is_a_directory(old_fildes))
        {
            explain_buffer_enotdir_fd(sb, old_fildes, "old_fildes");
            return;
        }
        if (!is_a_directory(new_fildes))
        {
            explain_buffer_enotdir_fd(sb, new_fildes, "new_fildes");
            return;
        }

        explain_buffer_enotdir2
        (
            sb,
            old_path_full,
            "old_path",
            &oldpath_fc,
            new_path_full,
            "new_path",
            &newpath_fc
        );
        return;

    case EACCES:
        if
        (
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                old_path_full,
                "old_path",
                &oldpath_fc
            )
        &&
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                new_path_full,
                "new_path",
                &newpath_fc
            )
        )

        /*
         * Unable to pin point an exact cause, go with the generic
         * explanation.
         */
        explain_string_buffer_puts
        (
            sb,
            "write access to the directory containing newpath is "
            "denied, or search permission is denied for one of the "
            "directories in the path prefix of oldpath or newpath "
        );
        return;

    case EEXIST:
        explain_buffer_eexist(sb, new_path);
        return;

    case EFAULT:
        if (explain_is_efault_path(old_path))
        {
            explain_buffer_efault(sb, "old_path");
            return;
        }
        if (explain_is_efault_path(new_path))
        {
            explain_buffer_efault(sb, "new_path");
            return;
        }
        explain_buffer_efault(sb, "old_path or new_path");
        return;

    case EIO:
        explain_buffer_eio_path(sb, old_path);
        return;

    case ELOOP:
        explain_buffer_eloop2
        (
            sb,
            old_path_full,
            "ol_path",
            &oldpath_fc,
            new_path_full,
            "ne_path",
            &newpath_fc
        );
        return;

    case EMLINK:
        explain_buffer_emlink(sb, old_path_full, new_path_full);
        return;

    case ENAMETOOLONG:
        explain_buffer_enametoolong2
        (
            sb,
            old_path_full,
            "olds path",
            &oldpath_fc,
            new_path_full,
            "new_path",
            &newpath_fc
        );
        return;

    case ENOENT:
        explain_buffer_enoent2
        (
            sb,
            old_path_full,
            "old_path",
            &oldpath_fc,
            new_path_full,
            "new_path",
            &newpath_fc
        );
        return;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        return;

    case ENOSPC:
        explain_buffer_enospc(sb, new_path_full, "new_path");
        return;

    case EPERM:
        {
            struct stat     oldpath_st;

            if (stat(old_path_full, &oldpath_st) >= 0)
            {
                if (S_ISDIR(oldpath_st.st_mode))
                {
                    explain_string_buffer_puts
                    (
                        sb,
                        "old_path is a directory and it is not possible "
                        "to make hard links to directories"
                    );
                    explain_string_buffer_puts(sb->footnotes, "; ");
                    explain_string_buffer_puts
                    (
                        sb->footnotes,
                        "have you considered using a symbolic link?"
                    );
                }
                else
                {
                    explain_string_buffer_puts
                    (
                        sb,
                        "the file system containing oldpath and newpath "
                        "does not support the creation of hard links"
                    );
                }
            }
            else
            {
                /*
                 * Unable to pin point a specific cause,
                 * issue the generic explanation.
                 */
                explain_string_buffer_puts
                (
                    sb,
                    "old_path is a directory; or, the file system "
                    "containing old_path and new_path does not support "
                    "the creation of hard links"
                );
            }
            return;
        }

    case EROFS:
        explain_buffer_erofs(sb, old_path_full, "old_path");
        return;

    case EXDEV:
        explain_buffer_exdev(sb, old_path_full, new_path_full, "link");
        return;

    default:
        /* No idea... */
        break;
    }
    explain_buffer_errno_generic(sb, errnum, syscall_name);
}


void
explain_buffer_errno_linkat(explain_string_buffer_t *sb, int errnum,
    int old_fildes, const char *old_path, int new_fildes, const char *new_path,
    int flags)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_linkat_system_call
    (
        &exp.system_call_sb,
        errnum,
        old_fildes,
        old_path,
        new_fildes,
        new_path,
        flags
    );
    explain_buffer_errno_linkat_explanation
    (
        &exp.explanation_sb,
        errnum,
        "linkat",
        old_fildes,
        old_path,
        new_fildes,
        new_path,
        flags
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
