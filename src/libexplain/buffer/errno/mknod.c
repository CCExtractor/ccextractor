/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/dev_t.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enospc.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/mknod.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/stat_mode.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_mknod_system_call(explain_string_buffer_t *sb, int errnum,
    const char *pathname, mode_t mode, dev_t dev)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "mknod(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", mode = ");
    explain_buffer_stat_mode(sb, mode);
    explain_string_buffer_puts(sb, ", dev = ");
    explain_buffer_dev_t(sb, dev);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_mknod_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, const char *pathname, mode_t mode, dev_t dev)
{
    explain_final_t fc;

    (void)dev;
    explain_final_init(&fc);
    fc.must_exist = 0;
    fc.must_not_exist = 1;
    fc.want_to_create = 1;
    fc.follow_symlink = 0;
    fc.st_mode = mode;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/mknod.html
     */
    switch (errnum)
    {
    case EEXIST:
        explain_buffer_eexist(sb, pathname);
        break;

    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &fc);
        break;

    case EFAULT:
        explain_buffer_efault(sb, "pathname");
        break;

    case ELOOP:
        explain_buffer_eloop(sb, pathname, "pathname", &fc);
        break;

    case ENAMETOOLONG:
        explain_buffer_enametoolong(sb, pathname, "pathname", &fc);
        break;

    case ENOENT:
        explain_buffer_enoent(sb, pathname, "pathname", &fc);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOSPC:
        explain_buffer_enospc(sb, pathname, "pathname");
        break;

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", &fc);
        break;

    case EPERM:
        explain_buffer_eperm_mknod(sb, pathname, "pathname", mode);
        break;

    case EROFS:
        explain_buffer_erofs(sb, pathname, "pathname");
        break;

    case EINVAL:
        explain_buffer_einval_mknod(sb, mode, "mode", syscall_name);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_mknod(explain_string_buffer_t *sb, int errnum,
    const char *pathname, mode_t mode, dev_t dev)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_mknod_system_call(&exp.system_call_sb, errnum,
        pathname, mode, dev);
    explain_buffer_errno_mknod_explanation(&exp.explanation_sb, errnum, "mknod",
        pathname, mode, dev);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
