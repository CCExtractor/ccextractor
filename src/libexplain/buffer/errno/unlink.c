/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/unlink.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/note/still_exists.h>
#include <libexplain/buffer/path_to_pid.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>
#include <libexplain/pathname_is_a_directory.h>
#include <libexplain/string_buffer.h>


static void
explain_buffer_errno_unlink_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname)
{
    explain_string_buffer_printf(sb, "unlink(pathname = ");
    if (errnum == EFAULT)
        explain_buffer_pointer(sb, pathname);
    else
        explain_string_buffer_puts_quoted(sb, pathname);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_unlink_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname)
{
    explain_final_t final_component;

    explain_final_init(&final_component);
    final_component.want_to_unlink = 1;

    switch (errnum)
    {
    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EBUSY:
        explain_string_buffer_puts
        (
            sb,
            /*
             * xgettext:  This error message is used to explain an
             * unlink EBUSY error, in the case where the pathname is
             * being used by the system or another process and the
             * implementation considers this an error.  (This does not
             * happen on Linux.)
             */
            i18n("the pathname is being used by the system or another "
            "process and the implementation considers this an error")
        );
        explain_buffer_path_to_pid(sb, pathname);
        break;

    case EFAULT:
        explain_buffer_efault(sb, "pathname");
        break;

    case EIO:
        explain_buffer_eio_path(sb, pathname);
        break;

    case EISDIR:
        explain_string_buffer_puts
        (
            sb,
            /*
             * xgettext:  This message is used when explaining an EISDIR error
             * reported by the unlink(2) system call, in the case where the
             * named file is a directory.
             */
            i18n("the named file is a directory; directories may not be "
            "unlinked, use rmdir(2) or remove(3) instead")
        );
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

    case EPERM:
#ifndef __linux__
        /*
         * This code branch is for systems OTHER THAN Linux,
         * because Linux says "EISDIR" in this case.
         */
        if (explain_pathname_is_a_directory(pathname))
        {
            explain_string_buffer_puts
            (
                sb,
                /*
                 * xgettext:  This message is uased to explain an EPERM error
                 * reported by the unlink system call, in the case where the
                 * system does not allow unlinking of directories, or unlinking
                 * of directories requires privileges that the process does not
                 * have.  This case does not happen on Linux.
                 */
                i18n("the system does not allow unlinking of directories, or "
                "unlinking of directories requires privileges that the process "
                "does not have")
            );
            break;
        }
#endif
        if
        (
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                pathname,
                "pathname",
                &final_component
            )
        )
        {
            explain_buffer_eperm_unlink(sb, pathname, "pathname", syscall_name);
            break;
        }
        goto generic;

    case EROFS:
        explain_buffer_erofs(sb, pathname, "pathname");
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }

    explain_buffer_note_if_still_exists(sb, pathname, "pathname");
}


void
explain_buffer_errno_unlink(explain_string_buffer_t *sb, int errnum,
    const char *pathname)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_unlink_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname
    );
    explain_buffer_errno_unlink_explanation
    (
        &exp.explanation_sb,
        errnum,
        "unlink",
        pathname
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
