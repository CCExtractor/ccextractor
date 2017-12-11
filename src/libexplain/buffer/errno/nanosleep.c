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

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/nanosleep.h>
#include <libexplain/buffer/timespec.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_nanosleep_system_call(explain_string_buffer_t *sb, int
    errnum, const struct timespec *req, struct timespec *rem)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "nanosleep(req = ");
    explain_buffer_timespec(sb, req);
    explain_string_buffer_puts(sb, ", rem = ");
    explain_buffer_timespec(sb, rem);
    explain_string_buffer_putc(sb, ')');
}


static int
is_valid_timespec(const struct timespec *ts)
{
    return (ts->tv_nsec >= 0 && ts->tv_nsec < 1000000000);
}


void
explain_buffer_errno_nanosleep_explanation(explain_string_buffer_t *sb, int
    errnum, const char *syscall_name, const struct timespec *req, struct
    timespec *rem)
{
    switch (errnum)
    {
    case EFAULT:
        if (explain_is_efault_pointer(req, sizeof(*req)))
        {
            explain_buffer_efault(sb, "req");
            break;
        }

        if (explain_is_efault_pointer(rem, sizeof(*rem)))
        {
            explain_buffer_efault(sb, "rem");
            break;
        }

        goto generic;

    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        break;

    case EINVAL:
        if (!is_valid_timespec(req))
        {
            explain_buffer_einval_vague(sb, "req");
            break;
        }
        goto generic;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_nanosleep(explain_string_buffer_t *sb, int errnum, const
    struct timespec *req, struct timespec *rem)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_nanosleep_system_call(&exp.system_call_sb, errnum, req,
        rem);
    explain_buffer_errno_nanosleep_explanation(&exp.explanation_sb, errnum,
        "nanosleep", req, rem);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
