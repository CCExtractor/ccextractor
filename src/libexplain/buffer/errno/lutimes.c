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
#include <libexplain/ac/sys/time.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/lutimes.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/timeval.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_lutimes_system_call(explain_string_buffer_t *sb, int
    errnum, const char *pathname, const struct timeval *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "lutimes(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_timeval_array(sb, data, 2);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_lutimes_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *pathname,
    const struct timeval *data)
{
    explain_final_t final_component;

    explain_final_init(&final_component);
    if (data)
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
        if (!pathname)
        {
            explain_buffer_is_the_null_pointer(sb, "pathname");
            break;
        }
        if (explain_is_efault_path(pathname))
        {
            explain_buffer_efault(sb, "pathname");
            break;
        }
        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            break;
        }
        if (explain_is_efault_pointer(data, sizeof(*data) * 2))
        {
            explain_buffer_efault(sb, "data");
            break;
        }
        goto generic;

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

    case EINVAL:
        if (!pathname)
        {
            explain_buffer_is_the_null_pointer(sb, "pathname");
            break;
        }
        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            break;
        }
        if (data[0].tv_usec == UTIME_OMIT && data[1].tv_usec == UTIME_OMIT)
        {
            explain_buffer_gettext
            (
                sb,
                /* FIXME: i18n */
                "both tv_usec vlues are UTIME_OMIT"
            );
        }
        goto generic;

    case ENOSYS:
#if defined(EOPNOTSUPP) && ENOSYS != EOPNOTSUPP
    case EOPNOTSUPP:
#endif
#if defined(ENOTSUP) && (ENOTSUP != EOPNOTSUPP)
    case ENOTSUP:
#endif
        explain_buffer_enosys_pathname(sb, pathname, "pathname", syscall_name);
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_lutimes(explain_string_buffer_t *sb, int errnum,
    const char *pathname, const struct timeval *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_lutimes_system_call(&exp.system_call_sb, errnum,
        pathname, data);
    explain_buffer_errno_lutimes_explanation(&exp.explanation_sb, errnum,
        "lutimes", pathname, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
