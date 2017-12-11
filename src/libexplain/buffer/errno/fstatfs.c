/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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
#include <libexplain/ac/sys/statfs.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/eoverflow.h>
#include <libexplain/buffer/errno/fstatfs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_fstatfs_system_call(explain_string_buffer_t *sb, int
    errnum, int fildes, struct statfs *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fstatfs(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_fstatfs_explanation(explain_string_buffer_t *sb, int
    errnum, int fildes, struct statfs *data)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/fstatfs.html
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
        explain_buffer_eintr(sb, "fstatfs");
        break;

    case EIO:
        explain_buffer_eio_fildes(sb, fildes);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOSYS:
        explain_buffer_enosys_fildes(sb, fildes, "fildes", "fstatfs");
        break;

    case EOVERFLOW:
        explain_buffer_eoverflow(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "fstatfs");
        break;
    }
}


void
explain_buffer_errno_fstatfs(explain_string_buffer_t *sb, int errnum, int
    fildes, struct statfs *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fstatfs_system_call(&exp.system_call_sb, errnum,
        fildes, data);
    explain_buffer_errno_fstatfs_explanation(&exp.explanation_sb, errnum,
        fildes, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
