/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/eoverflow.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/statvfs.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_statvfs_system_call(explain_string_buffer_t *sb, int
    errnum, const char *pathname, struct statvfs *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "statvfs(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_statvfs_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, const char *pathname, struct statvfs
    *data)
{
    explain_final_t final_component;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/statvfs.html
     */
    (void)data;
    explain_final_init(&final_component);
    switch (errnum)
    {
    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EFAULT:
        if (explain_is_efault_path(pathname))
            explain_buffer_efault(sb, "pathname");
        else
            explain_buffer_efault(sb, "data");
        break;

    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        break;

    case EIO:
        explain_buffer_eio_path(sb, pathname);
        break;

    case ELOOP:
        explain_buffer_eloop(sb, pathname, "pathname", &final_component);
        break;

    case ENAMETOOLONG:
        explain_buffer_enametoolong(sb, pathname, "pathname", &final_component);
        break;

    case ENOENT:
        explain_buffer_enoent(sb, pathname, "pathname", &final_component);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOSYS:
#if defined(EOPNOTSUPP) && ENOSYS != EOPNOTSUPP
    case EOPNOTSUPP:
#endif
#if defined(ENOTSUP) && (ENOTSUP != EOPNOTSUPP)
    case ENOTSUP:
#endif
        explain_buffer_enosys_pathname(sb, pathname, "pathname", syscall_name);
        break;

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    case EOVERFLOW:
        explain_buffer_eoverflow(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_statvfs(explain_string_buffer_t *sb, int errnum, const char
    *pathname, struct statvfs *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_statvfs_system_call(&exp.system_call_sb, errnum,
        pathname, data);
    explain_buffer_errno_statvfs_explanation(&exp.explanation_sb, errnum,
        "statvfs", pathname, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
