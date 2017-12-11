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
#include <libexplain/ac/sys/time.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/settimeofday.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/timeval.h>
#include <libexplain/buffer/timezone.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_settimeofday_system_call(explain_string_buffer_t *sb, int
    errnum, const struct timeval *tv, const struct timezone *tz)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "settimeofday(tv = ");
    explain_buffer_timeval(sb, tv);
    explain_string_buffer_puts(sb, ", tz = ");
    explain_buffer_timezone(sb, tz);
    explain_string_buffer_putc(sb, ')');
}


static int
valid_microseconds(long x)
{
    if (x == UTIME_NOW || x == UTIME_OMIT)
        return 1;
    return (0 <= x && x < 1000000);
}


void
explain_buffer_errno_settimeofday_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const struct timeval *tv,
    const struct timezone *tz)
{
    if (tz)
    {
        explain_string_buffer_puts(sb->footnotes, "; ");
        explain_buffer_gettext
        (
            sb->footnotes,
            /* FIXME: i18n */
            "the use of the timezone structure is obsolete; the tz "
            "argument should normally be specified as NULL"
        );
    }

    switch (errnum)
    {
    case EFAULT:
        if (explain_is_efault_pointer(tv, sizeof(*tv)))
        {
            explain_buffer_efault(sb, "tv");
            return;
        }
        if (explain_is_efault_pointer(tz, sizeof(*tz)))
        {
            explain_buffer_efault(sb, "tz");
            return;
        }
        break;

    case EINVAL:
        if (tv && !valid_microseconds(tv->tv_usec))
        {
            explain_buffer_einval_out_of_range(sb, "tv->tv_usec", 0, 1000000);
            return;
        }
        break;

    case EPERM:
        explain_buffer_eperm_sys_time(sb, syscall_name);
        return;

    default:
        break;
    }
    explain_buffer_errno_generic(sb, errnum, syscall_name);
}


void
explain_buffer_errno_settimeofday(explain_string_buffer_t *sb, int errnum,
    const struct timeval *tv, const struct timezone *tz)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_settimeofday_system_call
    (
        &exp.system_call_sb,
        errnum,
        tv,
        tz
    );
    explain_buffer_errno_settimeofday_explanation
    (
        &exp.explanation_sb,
        errnum,
        "settimeofday",
        tv,
        tz
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
