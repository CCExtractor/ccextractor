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

#include <libexplain/buffer/does_not_have_inode_modify_permission.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/fchmod.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/permission_mode.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_fchmod_system_call(explain_string_buffer_t *sb, int errnum,
    int fildes, mode_t mode)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fchmod(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", mode = ");
    explain_buffer_permission_mode(sb, mode);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_fchmod_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int fildes, mode_t mode)
{
    (void)mode;
    switch (errnum)
    {
    case EIO:
        explain_buffer_eio(sb);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case EPERM:
        /*
         * The effective UID does not match the owner of the file, and
         * the process is not privileged (Linux: it does not have the
         * CAP_FOWNER capability).
         */
        explain_buffer_does_not_have_inode_modify_permission_fd
        (
            sb,
            fildes,
            "fildes"
        );
        break;

    case EROFS:
        explain_buffer_erofs_fildes(sb, fildes, "fildes");
        break;

    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_fchmod(explain_string_buffer_t *sb, int errnum, int fildes,
    mode_t mode)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fchmod_system_call(&exp.system_call_sb, errnum, fildes,
        mode);
    explain_buffer_errno_fchmod_explanation(&exp.explanation_sb, errnum,
        "fchmod", fildes, mode);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
