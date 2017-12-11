/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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
#include <libexplain/ac/sys/time.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/gettimeofday.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_gettimeofday_system_call(
    explain_string_buffer_t *sb, int errnum, struct timeval *tv,
    struct timezone *tz)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "gettimeofday(tv = ");
    explain_buffer_pointer(sb, tv);
    explain_string_buffer_puts(sb, ", tz = ");
    explain_buffer_pointer(sb, tz);
    explain_string_buffer_putc(sb, ')');
}


/*
 * http://www.opengroup.org/onlinepubs/009695399/functions/gettimeofday.html
 */

static void
explain_buffer_errno_gettimeofday_explanation(
    explain_string_buffer_t *sb, int errnum, struct timeval *tv,
    struct timezone *tz)
{
    switch (errnum)
    {
    case EFAULT:
        if (tv && explain_is_efault_pointer(tv, sizeof(*tv)))
        {
            explain_buffer_efault(sb, "tv");
            break;
        }
        if (tz && explain_is_efault_pointer(tz, sizeof(*tz)))
        {
            explain_buffer_efault(sb, "tz");
            break;
        }
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "gettimeofday");
        break;
    }
}


void
explain_buffer_errno_gettimeofday(explain_string_buffer_t *sb,
    int errnum, struct timeval *tv, struct timezone *tz)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_gettimeofday_system_call
    (
        &exp.system_call_sb,
        errnum,
        tv,
        tz
    );
    explain_buffer_errno_gettimeofday_explanation
    (
        &exp.explanation_sb,
        errnum,
        tv,
        tz
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
