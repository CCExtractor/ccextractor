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
#include <libexplain/ac/unistd.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/efbig.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/truncate.h>
#include <libexplain/buffer/etxtbsy.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/off_t.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_truncate_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, off_t length)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "truncate(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", length = ");
    explain_buffer_off_t(sb, length);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_truncate_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname, off_t length)
{
    explain_final_t final_component;

    explain_final_init(&final_component);
    final_component.want_to_write = 1;

    switch (errnum)
    {
    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EFAULT:
        explain_buffer_efault(sb, "pathname");
        break;

    case EFBIG:
        explain_buffer_efbig(sb, pathname, length, "length");
        break;

    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        break;

    case EINVAL:
        if (length < 0)
        {
            /* FIXME: i18n */
            explain_string_buffer_puts(sb, "length is negative");
            break;
        }

        {
            struct stat     st;

            /* FIXME: explain_buffer_wrong_file_type */
            if (stat(pathname, &st) >= 0 && !S_ISREG(st.st_mode))
            {
                explain_string_buffer_puts(sb, "pathname is a ");
                explain_buffer_file_type_st(sb, &st);
                explain_string_buffer_puts(sb, ", not a ");
                explain_buffer_file_type(sb, S_IFREG);
                break;
            }
        }

        explain_buffer_efbig(sb, pathname, length, "length");
        break;

    case EIO:
        explain_buffer_eio_path(sb, pathname);
        break;

    case EISDIR:
        explain_string_buffer_puts
        (
            sb,
            /*
             * xgettext:  This message is used to explain an EISDIR error
             * reported by the truncate(2) system call, in the case where the
             * named file is a directory.
             */
            i18n("the named file is a directory; directories may not be "
            "truncated, use rmdir(2) or remove(3) instead")
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

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    case EPERM:
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the underlying file system does not support extending a "
            "file beyond its current size"
        );
        explain_buffer_mount_point(sb, pathname);
        break;

    case EROFS:
        explain_buffer_erofs(sb, pathname, "pathname");
        break;

    case ETXTBSY:
        explain_buffer_etxtbsy(sb, pathname);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_truncate(explain_string_buffer_t *sb, int errnum,
    const char *pathname, off_t length)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_truncate_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        length
    );
    explain_buffer_errno_truncate_explanation
    (
        &exp.explanation_sb,
        errnum,
        "truncate",
        pathname,
        length
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
