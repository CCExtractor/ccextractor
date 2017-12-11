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

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/errno/futimesat.h>
#include <libexplain/buffer/errno/utimes.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/timeval.h>
#include <libexplain/explanation.h>
#include <libexplain/fileinfo.h>


static void
explain_buffer_errno_futimesat_system_call(explain_string_buffer_t *sb, int
    errnum, int fildes, const char *pathname, const struct timeval *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "futimesat(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_timeval_array(sb, data, 2);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_futimesat_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, int fildes, const char *pathname, const
    struct timeval *data)
{
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case ENOTDIR:
        if (pathname[0] == '/')
        {
             explain_buffer_errno_utimes_explanation(sb, errnum, syscall_name,
                 pathname, data);
             break;
        }
        if (fildes == AT_FDCWD)
        {
             explain_buffer_errno_utimes_explanation(sb, errnum, syscall_name,
                 pathname, data);
             break;
        }
        if (!explain_fildes_is_a_directory(fildes))
        {
            explain_buffer_enotdir_fd(sb, fildes, "fildes");
            break;
        }

        /*
         * form an absolute path by ombining the fd's dir with the pathname
         */
        {
            char dir2[PATH_MAX + 1];
            char pathname2[PATH_MAX + 1];
            explain_string_buffer_t sb2;
            explain_string_buffer_init(&sb2, pathname2, sizeof(pathname2));
            explain_fileinfo_self_fd_n(fildes, dir2, sizeof(dir2));
            explain_string_buffer_puts(&sb2, dir2);
            explain_string_buffer_path_join(&sb2, pathname);

            explain_buffer_errno_utimes_explanation(sb, errnum, syscall_name,
                pathname2, data);
        }
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_futimesat(explain_string_buffer_t *sb, int errnum, int
    fildes, const char *pathname, const struct timeval *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_futimesat_system_call(&exp.system_call_sb, errnum,
        fildes, pathname, data);
    explain_buffer_errno_futimesat_explanation(&exp.explanation_sb, errnum,
        "futimesat", fildes, pathname, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
