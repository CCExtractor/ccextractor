/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2012, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/does_not_have_inode_modify_permission.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/chmod.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/permission_mode.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/capability.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_chmod_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, int mode)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "chmod(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", mode = ");
    explain_buffer_permission_mode(sb, mode);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_chmod_explanation(explain_string_buffer_t *sb,
    int errnum, const char *pathname, int mode)
{
    explain_final_t final_component;

    explain_final_init(&final_component);
    final_component.want_to_modify_inode = 1;

    explain_buffer_errno_chmod_explanation_fc
    (
        sb,
        errnum,
        "chmod",
        pathname,
        mode,
        &final_component
    );
}


void
explain_buffer_errno_chmod_explanation_fc(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname, int mode,
    const explain_final_t *final_component)
{
    (void)mode;
    switch (errnum)
    {
    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", final_component);
        break;

    case EFAULT:
        explain_buffer_efault(sb, "pathname");
        break;

    case EIO:
        explain_buffer_eio_path(sb, pathname);
        break;

    case ELOOP:
    case EMLINK: /* BSD */
        explain_buffer_eloop(sb, pathname, "pathname", final_component);
        break;

    case ENAMETOOLONG:
        explain_buffer_enametoolong
        (
            sb,
            pathname,
            "pathname",
            final_component
        );
        break;

    case ENOENT:
        explain_buffer_enoent(sb, pathname, "pathname", final_component);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", final_component);
        break;

    case EPERM:
        if
        (
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                pathname,
                "pathname",
                final_component
            )
        )
        {
            explain_buffer_does_not_have_inode_modify_permission_fd_st
            (
                sb,
                (struct stat *)0,
                "pathname",
                &final_component->id
            );
        }
        break;

    case EROFS:
        explain_buffer_erofs(sb, pathname, "pathname");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_chmod(explain_string_buffer_t *sb, int errnum,
    const char *pathname, int mode)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_chmod_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        mode
    );
    explain_buffer_errno_chmod_explanation
    (
        &exp.explanation_sb,
        errnum,
        pathname,
        mode
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
