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

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/utime.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/utimbuf.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_utime_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, const struct utimbuf *times)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "utime(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", times = ");
    explain_buffer_utimbuf(sb, times);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_utime_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname,
    const struct utimbuf *times)
{
    explain_final_t final_component;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/utime.html
     */
    explain_final_init(&final_component);
    if (times)
        final_component.want_to_modify_inode = 1;
    else
        final_component.want_to_write = 1;
    switch (errnum)
    {
    case EACCES:
        /*
         * if time is NULL, change to "now",
         * but need write permission
         */
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EFAULT:
        if (explain_is_efault_path(pathname))
            explain_buffer_efault(sb, "pathname");
        else
            explain_buffer_efault(sb, "times");
        break;

    case ELOOP:
    case EMLINK:
        explain_buffer_eloop(sb, pathname, "pathname", &final_component);
        break;

    case ENAMETOOLONG:
        explain_buffer_enametoolong(sb, pathname, "pathname", &final_component);
        break;

    case ENOENT:
        explain_buffer_enoent(sb, pathname, "pathname", &final_component);
        break;

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    case EPERM:
        /*
         * If times is not NULL, change as given,
         * but need inode modify permission
         */
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
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
explain_buffer_errno_utime(explain_string_buffer_t *sb, int errnum,
    const char *pathname, const struct utimbuf *times)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_utime_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        times
    );
    explain_buffer_errno_utime_explanation
    (
        &exp.explanation_sb,
        errnum,
        "utime",
        pathname,
        times
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
