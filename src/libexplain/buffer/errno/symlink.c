/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enospc.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/symlink.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>
#include <libexplain/string_buffer.h>


static void
explain_buffer_errno_symlink_system_call(explain_string_buffer_t *sb,
    int errnum, const char *oldpath, const char *newpath)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "symlink(oldpath = ");
    explain_buffer_pathname(sb, oldpath);
    explain_string_buffer_puts(sb, ", newpath = ");
    explain_buffer_pathname(sb, newpath);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_symlink_explanation(explain_string_buffer_t *sb,
    int errnum, const char *oldpath, const char *newpath)
{
    explain_final_t final_component;

    explain_final_init(&final_component);
    final_component.must_exist = 0;
    final_component.must_not_exist = 1;
    final_component.want_to_create = 1;
    final_component.st_mode = S_IFLNK;

    switch (errnum)
    {
    case EACCES:
        explain_buffer_eacces(sb, newpath, "newpath", &final_component);
        break;

    case EEXIST:
        explain_buffer_eexist(sb, newpath);
        break;

    case EFAULT:
        if (explain_is_efault_path(oldpath))
        {
            explain_buffer_efault(sb, "oldpath");
            break;
        }
        if (explain_is_efault_path(newpath))
        {
            explain_buffer_efault(sb, "newpath");
            break;
        }
        explain_buffer_efault(sb, "oldpath or newpath");
        break;

    case EIO:
        explain_buffer_eio_path_dirname(sb, newpath);
        break;

    case ELOOP:
    case EMLINK: /* BSD */
        explain_buffer_eloop(sb, newpath, "newpath", &final_component);
        break;

    case ENAMETOOLONG:
        {
            size_t          oldpath_len;
            long            path_max;

            oldpath_len = strlen(oldpath);
            path_max = pathconf(newpath, _PC_PATH_MAX);
            if (path_max <= 0)
                path_max = PATH_MAX;
            if (oldpath_len > (size_t)path_max)
            {
                /* FIXME: i18n */
                explain_string_buffer_puts(sb, "oldpath is too long");
                explain_string_buffer_printf
                (
                    sb,
                    " (%ld > %ld)",
                    (long)oldpath_len,
                    path_max
                );
            }
            else
            {
                explain_buffer_enametoolong
                (
                    sb,
                    newpath,
                    "newpath",
                    &final_component
                );
            }
        }
        break;

    case ENOENT:
        if (!*oldpath)
        {
            explain_string_buffer_puts
            (
                sb,
                /* FIXME: i18n */
                "oldpath is the empty string; if you meant the current "
                "directory, use \".\" instead"
            );
            break;
        }
        explain_buffer_enoent(sb, newpath, "newpath", &final_component);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOSPC:
        explain_buffer_enospc(sb, newpath, "newpath");
        break;

    case ENOTDIR:
        explain_buffer_enotdir(sb, newpath, "newpath", &final_component);
        break;

    case EPERM:
        /* FIXME: i18n */
        explain_string_buffer_puts
        (
            sb,
            "the file system containing newpath"
        );
        explain_buffer_mount_point_dirname(sb, newpath);
        explain_string_buffer_puts
        (
            sb,
            " does not support the creation of symbolic links"
        );
        break;

    case EROFS:
        explain_buffer_erofs(sb, newpath, "newpath");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "symlink");
        break;
    }
}


void
explain_buffer_errno_symlink(explain_string_buffer_t *sb, int errnum,
    const char *oldpath, const char *newpath)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_symlink_system_call
    (
        &exp.system_call_sb,
        errnum,
        oldpath,
        newpath
    );
    explain_buffer_errno_symlink_explanation
    (
        &exp.explanation_sb,
        errnum,
        oldpath,
        newpath
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
