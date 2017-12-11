/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
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
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/readlink.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/off_t.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_readlink_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, char *data, size_t data_size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "readlink(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_size_t(sb, data_size);
    explain_string_buffer_putc(sb, ')');
}


static off_t
get_actual_size(const char *pathname)
{
    struct stat     st;

    /*
     * According to coreutils...
     * some systems fail to set st_size for symlinks.
     */
    st.st_size = 0;

    if (lstat(pathname, &st) < 0)
        return -1;
    if (!S_ISLNK(st.st_mode))
        return -1;
    return st.st_size;
}


static void
explain_buffer_errno_readlink_explanation(explain_string_buffer_t *sb,
    int errnum, const char *pathname, char *data, size_t data_size)
{
    explain_final_t final_component;

    (void)data;
    explain_final_init(&final_component);
    final_component.want_to_read = 1;
    final_component.follow_symlink = 0;

    switch (errnum)
    {
    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EFAULT:
        if (explain_is_efault_path(pathname))
        {
            explain_buffer_efault(sb, "pathname");
            break;
        }
        explain_buffer_efault(sb, "data");
        break;

    case EINVAL:
        if ((ssize_t)data_size <= 0)
        {
            explain_buffer_einval_too_small
            (
                sb,
                "data_size",
                (ssize_t)data_size
            );
        }
        else
        {
            struct stat     st;

            /* FIXME: explain_buffer_wrong_file_type */
            if (lstat(pathname, &st) >= 0)
            {
                explain_string_buffer_puts(sb, "pathname is a ");
                explain_buffer_file_type_st(sb, &st);
                explain_string_buffer_puts(sb, ", not a ");
                explain_buffer_file_type(sb, S_IFLNK);
            }
            else
            {
                explain_string_buffer_puts(sb, "pathname is not a ");
                explain_buffer_file_type(sb, S_IFLNK);
            }
        }
        break;

    case EIO:
        explain_buffer_eio_path(sb, pathname);
        break;

    case ELOOP:
    case EMLINK: /* BSD */
        explain_buffer_eloop(sb, pathname, "pathname", &final_component);
        break;

    case ENAMETOOLONG:
        explain_buffer_enametoolong
        (
            sb,
            pathname,
            "pathname",
            &final_component
        );
        break;

    case ENOENT:
        explain_buffer_enoent(sb, pathname, "pathname", &final_component);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    case ERANGE:
        {
            off_t           actual_size;

            /*
             * According to coreutils...
             * On AIX 5L v5.3 and HP-UX 11i v2 04/09, readlink returns -1
             * with errno == ERANGE if the buffer is too small.
             */
            explain_buffer_einval_too_small(sb, "data_size", data_size);

            /*
             * Provide the actual size, if available.
             */
            actual_size = get_actual_size(pathname);
            if (actual_size > 0 && (off_t)data_size < actual_size)
            {
                explain_string_buffer_puts(sb, " (");
                explain_buffer_off_t(sb, actual_size);
                explain_string_buffer_putc(sb, ')');
            }
        }
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "readlink");
        break;
    }
}


void
explain_buffer_errno_readlink(explain_string_buffer_t *sb, int errnum,
    const char *pathname, char *data, size_t data_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_readlink_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        data,
        data_size
    );
    explain_buffer_errno_readlink_explanation
    (
        &exp.explanation_sb,
        errnum,
        pathname,
        data,
        data_size
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
