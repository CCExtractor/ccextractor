/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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
#include <libexplain/ac/limits.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/adjtime.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/timeval.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_adjtime_system_call(explain_string_buffer_t *sb, int
    errnum, const struct timeval *delta, struct timeval *olddelta)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "adjtime(delta = ");
    explain_buffer_timeval(sb, delta);
    explain_string_buffer_puts(sb, ", olddelta = ");
    explain_buffer_pointer(sb, olddelta);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_adjtime_explanation(explain_string_buffer_t *sb, int
    errnum, const struct timeval *delta, struct timeval *olddelta)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/adjtime.html
     */
    switch (errnum)
    {
    case EFAULT:
        if (explain_is_efault_pointer(delta, sizeof(*delta)))
            explain_buffer_efault(sb, "delta");
        if (olddelta && explain_is_efault_pointer(olddelta, sizeof(*olddelta)))
            explain_buffer_efault(sb, "olddelta");
        break;

    case EINVAL:
        if (!explain_is_efault_pointer(delta, sizeof(*delta)))
        {
            long lo = (INT_MIN / 1000000 + 2);
            long hi = (INT_MAX / 1000000 - 2);
            if (delta->tv_sec < lo || delta->tv_sec > hi)
            {
                explain_buffer_einval_out_of_range(sb, "delta->tv_sec", lo, hi);
                break;
            }
        }
        explain_buffer_einval_vague(sb, "delta");
        break;

    case EPERM:
        explain_buffer_eperm_sys_time(sb, "adjtime");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "adjtime");
        break;
    }
}


void
explain_buffer_errno_adjtime(explain_string_buffer_t *sb, int errnum, const
    struct timeval *delta, struct timeval *olddelta)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_adjtime_system_call(&exp.system_call_sb, errnum, delta,
        olddelta);
    explain_buffer_errno_adjtime_explanation(&exp.explanation_sb, errnum, delta,
        olddelta);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
