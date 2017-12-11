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
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/open.h>
#include <libexplain/buffer/errno/openat.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/open_flags.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/permission_mode.h>
#include <libexplain/explanation.h>
#include <libexplain/fileinfo.h>
#include <libexplain/fildes_is_dot.h>


static void
explain_buffer_errno_openat_system_call(explain_string_buffer_t *sb, int errnum,
    int fildes, const char *pathname, int flags, mode_t mode)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "openat(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_open_flags(sb, flags);
    explain_string_buffer_puts(sb, ", mode = ");
    explain_buffer_permission_mode(sb, mode);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_openat_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int fildes, const char *pathname, int flags,
    mode_t mode)
{
    (void)pathname;
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        return;

    case ENOTDIR:
        /*
         * This is ambiguous, it could refer to fildes or it could refer
         * to pathname.
         */
        if (!explain_fildes_is_a_directory(fildes))
        {
            explain_buffer_enotdir_fd(sb, fildes, "fildes");
            return;
        }
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        return;

    default:
        break;
    }

    if (explain_fildes_is_dot(fildes))
    {
        while (*pathname == '/')
            ++pathname;
        if (!*pathname)
            pathname = ".";
        explain_buffer_errno_open_explanation
        (
            sb,
            errnum,
            syscall_name,
            pathname,
            flags,
            mode
        );
        return;
    }

    {
        char long_path[PATH_MAX * 2];
        if (explain_fileinfo_self_fd_n(fildes, long_path, sizeof(long_path)))
        {
            char *lp = long_path + strlen(long_path);
            char *end = long_path + sizeof(long_path);
            lp = explain_strendcpy(lp, "/", end);
            lp = explain_strendcpy(lp, pathname, end);
            explain_buffer_errno_open_explanation
            (
                sb,
                errnum,
                syscall_name,
                long_path,
                flags,
                mode
            );
            return;
        }
        /* Fall through... */
    }

    explain_buffer_errno_generic(sb, errnum, syscall_name);
}


void
explain_buffer_errno_openat(explain_string_buffer_t *sb, int errnum, int fildes,
    const char *pathname, int flags, mode_t mode)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_openat_system_call(&exp.system_call_sb, errnum, fildes,
        pathname, flags, mode);
    explain_buffer_errno_openat_explanation(&exp.explanation_sb, errnum,
        "openat", fildes, pathname, flags, mode);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
