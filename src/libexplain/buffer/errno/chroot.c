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
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/chroot.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_chroot_system_call(explain_string_buffer_t *sb, int errnum,
    const char *pathname)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "chroot(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_chroot_explanation(explain_string_buffer_t *sb, int errnum,
    const char *pathname)
{
    explain_final_t final_component;

    explain_final_init(&final_component);
    final_component.want_to_search = 1;
    final_component.must_be_a_st_mode = 1;
    final_component.st_mode = S_IFDIR;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/chroot.html
     */
    switch (errnum)
    {
    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EFAULT:
        explain_buffer_efault(sb, "pathname");
        break;

    case EIO:
        explain_buffer_eio(sb);
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

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    case EPERM:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued when a
             * process attempts to change its root directory.
             */
            i18n("the process does not have permission to change its root "
                "directory")
        );
        explain_buffer_dac_sys_chroot(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "chroot");
        break;
    }
}


void
explain_buffer_errno_chroot(explain_string_buffer_t *sb, int errnum, const char
    *pathname)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_chroot_system_call(&exp.system_call_sb, errnum,
        pathname);
    explain_buffer_errno_chroot_explanation(&exp.explanation_sb, errnum,
        pathname);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
