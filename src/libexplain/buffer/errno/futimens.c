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
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/errno/futimens.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/timespec.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_futimens_system_call(explain_string_buffer_t *sb, int
    errnum, int fildes, const struct timespec *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "futimens(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_timespec_array(sb, data, 2);
    explain_string_buffer_putc(sb, ')');
}


static int
ok_tv_nsec(long x)
{
    return ((x >= 0 && x < 100000000) || x == UTIME_NOW || x == UTIME_OMIT);
}


void
explain_buffer_errno_futimens_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, int fildes, const struct timespec *data)
{
    switch (errnum)
    {
    case EACCES:
        if (!data)
        {
             explain_buffer_is_the_null_pointer(sb, "data");
             break;
        }
        if (data[0].tv_nsec == UTIME_NOW)
        {
            explain_buffer_gettext
            (
                sb,
                /* FIXME: i18n */
                "both tv_nsec values are UTIME_NOW"
            );
            break;
        }
        explain_buffer_eacces_syscall(sb, syscall_name);
        break;

    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINVAL:
        if (!data)
        {
             explain_buffer_is_the_null_pointer(sb, "data");
             break;
        }
        if (!ok_tv_nsec(data[0].tv_nsec))
        {
            explain_buffer_einval_out_of_range(sb, "data[0].tv_nsec",
                0, 999999999);
            break;
        }
        if (!ok_tv_nsec(data[1].tv_nsec))
        {
            explain_buffer_einval_out_of_range(sb, "data[1].tv_nsec",
                0, 999999999);
            break;
        }
        explain_buffer_einval_vague(sb, "data");
        break;

    case EPERM:
        explain_buffer_eperm_vague(sb, syscall_name);
        explain_buffer_dac_fowner(sb);
        break;

    case EROFS:
        explain_buffer_erofs_fildes(sb, fildes, "fildes");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_futimens(explain_string_buffer_t *sb, int errnum, int
    fildes, const struct timespec *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_futimens_system_call(&exp.system_call_sb, errnum,
        fildes, data);
    explain_buffer_errno_futimens_explanation(&exp.explanation_sb, errnum,
        "futimens", fildes, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
