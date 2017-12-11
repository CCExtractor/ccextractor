/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/rmdir.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/note/still_exists.h>
#include <libexplain/buffer/path_to_pid.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/capability.h>
#include <libexplain/explanation.h>
#include <libexplain/count_directory_entries.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_rmdir_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname)
{
    explain_string_buffer_puts(sb, "rmdir(pathname = ");
    if (errnum == EFAULT)
        explain_buffer_pointer(sb, pathname);
    else
        explain_string_buffer_puts_quoted(sb, pathname);
    explain_string_buffer_putc(sb, ')');
}


static int
last_component_is_dotdot(const char *pathname)
{
    while (*pathname == '/')
        ++pathname;
    for (;;)
    {
        const char      *begin;
        const char      *end;

        begin = pathname;
        ++pathname;
        while (*pathname && *pathname != '/')
            ++pathname;
        end = pathname;
        while (*pathname == '/')
            ++pathname;
        if (!*pathname)
        {
            /* we are at end of pathname */
            return (end - begin == 2 && begin[0] == '.' && begin[1] == '.');
        }
    }
}


static int
last_component_is_dot(const char *pathname)
{
    while (*pathname == '/')
        ++pathname;
    for (;;)
    {
        const char      *begin;
        const char      *end;

        begin = pathname;
        ++pathname;
        while (*pathname && *pathname != '/')
            ++pathname;
        end = pathname;
        while (*pathname == '/')
            ++pathname;
        if (!*pathname)
        {
            /* we are at end of pathname */
            return (end - begin == 1 && begin[0] == '.');
        }
    }
}


void
explain_buffer_errno_rmdir_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname)
{
    explain_final_t final_component;

    explain_final_init(&final_component);
    final_component.want_to_unlink = 1;
    final_component.must_be_a_st_mode = 1;
    final_component.st_mode = S_IFDIR;

    switch (errnum)
    {
    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EBUSY:
        if (last_component_is_dot(pathname))
        {
            /* BSD */
            goto case_einval;
        }
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This error message is used when the rmdir(2)
             * system call returns EBUSY.
             */
            i18n("pathname is currently in use by the system or some process "
            "that prevents its removal")
        );
        explain_buffer_path_to_pid(sb, pathname);
#ifdef __linux__
        if (explain_option_dialect_specific())
        {
            explain_string_buffer_puts(sb->footnotes, "; ");
            explain_buffer_gettext
            (
                sb->footnotes,
                /*
                 * xgettext: This error message is used when the rmdir(2)
                 * system call returns EBUSY, on a Linux system.
                 */
                i18n("note that pathname is currently used as a mount point "
                "or is the root directory of the calling process")
            );
        }
#endif
        break;

    case EFAULT:
        explain_buffer_efault(sb, "pathname");
        break;

    case EINVAL:
        case_einval:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext:  This error message is used when the rmdir(2)
             * system call returns EINVAL, in the case where the final
             * path component is "."
             */
            i18n("pathname has \".\" as last component")
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

    case EEXIST:
    case ENOTEMPTY:
        if (last_component_is_dotdot(pathname))
        {
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext:  This error message is used when the rmdir(2)
                 * system call returns EINVAL, in the case where the final
                 * path component is ".."
                 */
                i18n("pathname has \"..\" as its final component")
            );
        }
        else
        {
            int             count;

            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext:  This error message is used when the
                 * rmdir(2) system call returns EEXIST or ENOTEMPTY, in
                 * the case where pathname is not an empty directory;
                 * that is, it contains entries other than "." and ".."
                 */
                i18n("pathname is not an empty directory; that is, it "
                "contains entries other than \".\" and \"..\"")
            );
            count = explain_count_directory_entries(pathname);
            if (count > 0)
                explain_string_buffer_printf(sb, " (%d)", count);
        }
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
                &final_component
            )
        )
        {
            explain_buffer_eperm_unlink(sb, pathname, "pathname", syscall_name);
        }
        break;

    case EROFS:
        explain_buffer_erofs(sb, pathname, "pathname");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }

    explain_buffer_note_if_still_exists(sb, pathname, "pathname");
}


void
explain_buffer_errno_rmdir(explain_string_buffer_t *sb, int errnum,
    const char *pathname)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_rmdir_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname
    );
    explain_buffer_errno_rmdir_explanation
    (
        &exp.explanation_sb,
        errnum,
        "rmdir",
        pathname
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
