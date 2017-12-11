/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2012, 2013 Peter Miller
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
#include <libexplain/ac/fcntl.h> /* for AT_FDCWD */

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/futimens.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/utimens.h>
#include <libexplain/buffer/errno/utimensat.h>
#include <libexplain/buffer/esrch.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/timespec.h>
#include <libexplain/buffer/utimensat_fildes.h>
#include <libexplain/buffer/utimensat_flags.h>
#include <libexplain/explanation.h>
#include <libexplain/fileinfo.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_utimensat_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, const char *pathname, const struct timespec *data,
    int flags)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "utimensat(fildes = ");
    explain_buffer_utimensat_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_timespec_array(sb, data, 2);
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
    return S_ISDIR(st.st_mode);
}


static int
valid_nsec(long value)
{
    if (value == UTIME_NOW)
        return 1;
    if (value == UTIME_OMIT)
        return 1;
    return (value >= 0 && value < 1000000000L);
}


static void
user_path_at(explain_string_buffer_t *sb, int fildes, const char *pathname)
{
     char dirpath[PATH_MAX + 1];

     if (!pathname || !*pathname)
        pathname = ".";

     if (pathname[0] == '/')
     {
         explain_string_buffer_puts(sb, pathname);
         return;
     }
     if (fildes == AT_FDCWD)
     {
         explain_string_buffer_puts(sb, pathname);
         return;
     }
     explain_fileinfo_self_fd_n(fildes, dirpath, sizeof(dirpath));
     explain_string_buffer_puts(sb, dirpath);
     explain_string_buffer_path_join(sb, pathname);
}


void
explain_buffer_errno_utimensat_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int fildes, const char *pathname,
    const struct timespec *data, int flags)
{
    explain_final_t final_component;
    struct timespec phony_data[2];
    char pathname2[PATH_MAX + 1];

    /*
     * Strange handling of corner case.  See Linux kernel sources file
     * fs/utimes.c, around line 122, for more detail.
     */
    if (pathname == NULL)
    {
        explain_buffer_errno_futimens_explanation(sb, errnum, syscall_name,
            fildes, data);
        return;
    }

    /*
     * Extract an absolute path from the arguments.  This has the
     * potential to exceed PATH_MAX, which is partly why the *st
     * funtions exist.
     */
    {
        explain_string_buffer_t sb2;
        explain_string_buffer_init(&sb2, pathname2, sizeof(pathname2));
        user_path_at(&sb2, fildes, pathname);
        pathname = pathname2;
    }

    /*
     * Default the data, if necessary.
     */
    if (!data)
    {
        phony_data[0].tv_sec = 0;
        phony_data[0].tv_nsec= UTIME_NOW;
        phony_data[1].tv_sec = 0;
        phony_data[1].tv_nsec= UTIME_NOW;
        data = phony_data;
    }

    explain_final_init(&final_component);
    if
    (
        data[0].tv_nsec != UTIME_OMIT
    ||
        data[1].tv_nsec != UTIME_OMIT
    )
    {
        if (data[0].tv_nsec == UTIME_NOW && data[1].tv_nsec == UTIME_NOW)
            final_component.want_to_write = 1;
        else
            final_component.want_to_modify_inode = 1;
    }
    if (0 == (flags & (AT_SYMLINK_FOLLOW | AT_SYMLINK_NOFOLLOW)))
        final_component.follow_symlink = 1;
    if (flags & AT_SYMLINK_FOLLOW)
        final_component.follow_symlink = 1;
    if (flags & AT_SYMLINK_NOFOLLOW)
        final_component.follow_symlink = 0;
    final_component.must_exist = 1;

    switch (errnum)
    {
    case EINVAL:
        /* see fs/utimes.c: 134 */
        if (!valid_nsec(data[0].tv_nsec))
        {
            explain_buffer_einval_vague(sb, "data[0]");
            return;
        }
        if (!valid_nsec(data[1].tv_nsec))
        {
            explain_buffer_einval_vague(sb, "data[1]");
            return;
        }
        if (data[0].tv_nsec == UTIME_NOW && data[1].tv_nsec == UTIME_NOW)
        {
            explain_buffer_gettext
            (
                sb,
                /* FIXME:i8n */
                "both tv_nsec values are UTIME_NOW"
            );
            return;
        }
        if (flags & ~explain_allbits_utimensat_flags())
        {
            /*
             * Invalid value in flags.
             */
            explain_buffer_einval_vague(sb, "flags");
            return;
        }
        if (data)
        {
            /*
             * "Invalid value in one of the tv_nsec fields (value outside
             * range 0 to 999,999,999, and not UTIME_NOW or UTIME_OMIT); or an
             * invalid value in one of the tv_sec fields."
             */
            if (!valid_nsec(data[0].tv_nsec))
            {
                explain_buffer_einval_vague(sb, "data[0].tv_nsec");
                return;
            }
            if (!valid_nsec(data[1].tv_nsec))
            {
                explain_buffer_einval_vague(sb, "data[1].tv_nsec");
                return;
            }
        }
        break;

    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        return;

    case EBADF:
        {
            struct stat st;
            if (fstat(fildes, &st) < 0)
            {
                explain_buffer_ebadf(sb, fildes, "fildes");
                return;
            }
            if (!S_ISDIR(st.st_mode))
            {
                explain_buffer_ebadf_dir(sb, "fildes");
                return;
            }
        }
        break;

    case EFAULT:
        if (explain_is_efault_pointer(data, sizeof(*data) * 2))
        {
            explain_buffer_efault(sb, "data");
            return;
        }

        /*
         * 'data' pointed to an invalid address; or, 'fildes' was
         * AT_FDCWD, and 'pathname' is NULL or an invalid address
         */
        if (explain_is_efault_path(pathname))
        {
            explain_buffer_efault(sb, "pathname");
            return;
        }
        break;

    case ELOOP:
    case EMLINK:
        /*
         * Too many symbolic links were encountered in resolving
         * pathname.
         */
        explain_buffer_eloop(sb, pathname, "pathname", &final_component);
        return;

    case ENAMETOOLONG:
        /*
         * pathname is too long.
         */
        explain_buffer_enametoolong(sb, pathname, "pathname", &final_component);
        return;

    case ENOENT:
        /*
         * A component of pathname does not refer to an existing
         * directory or file, or pathname is an empty string.
         */
        explain_buffer_enoent(sb, pathname, "pathname", &final_component);
        return;

    case ENOTDIR:
        /*
         * pathname is a relative pathname, but dirfd is neiher
         * AT_FDCWD nor a file descriptor referring to a directory;
         */
        if
        (
            pathname[0] != '/'
        &&
            fildes != AT_FDCWD
        &&
            0 == is_a_directory(fildes)
        )
        {
            explain_buffer_enotdir_fd(sb, fildes, "fildes");
            return;
        }

        /*
         * or, one of the prefix components of pathname is not a directory.
         */
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        return;

    case EPERM:
        if (!data)
            break;
        if (data[0].tv_nsec == UTIME_OMIT && data[1].tv_nsec == UTIME_OMIT)
            break;
        break;

    case EROFS:
        /*
         * "The file is on a read-only file system."
         */
        explain_buffer_erofs(sb, pathname, "pathname");
        break;

    case ESRCH:
        /*
         * "Search permission is denied for one of the prefix components
         * of pathname."
         */
        errnum = ENOENT;
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    default:
        break;
    }

    explain_buffer_errno_utimens_explanation(sb, errnum, syscall_name,
        pathname, data);
}


void
explain_buffer_errno_utimensat(explain_string_buffer_t *sb, int errnum,
    int fildes, const char *pathname, const struct timespec *data, int flags)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_utimensat_system_call(&exp.system_call_sb, errnum,
        fildes, pathname, data, flags);
    explain_buffer_errno_utimensat_explanation(&exp.explanation_sb, errnum,
        "utimensat", fildes, pathname, data, flags);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
