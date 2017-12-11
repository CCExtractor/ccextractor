/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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
#include <libexplain/buffer/errno/fstatvfs.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_fstatvfs_system_call(explain_string_buffer_t *sb, int
    errnum, int fildes, struct statvfs *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fstatvfs(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fstatvfs_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, int fildes, struct statvfs *data)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/fstatvfs.html
     */
    (void)data;
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        break;

    case EIO:
        explain_buffer_eio(sb);
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
        explain_buffer_enosys_fildes(sb, fildes, "fildes", syscall_name);
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
explain_buffer_errno_fstatvfs(explain_string_buffer_t *sb, int errnum, int
    fildes, struct statvfs *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fstatvfs_system_call(&exp.system_call_sb, errnum,
        fildes, data);
    explain_buffer_errno_fstatvfs_explanation(&exp.explanation_sb, errnum,
        "fstatvfs", fildes, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
