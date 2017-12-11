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

#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/long_long.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/usleep.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_usleep_system_call(explain_string_buffer_t *sb, int errnum,
    long long usec)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "usleep(usec = ");
    explain_buffer_long_long(sb, usec);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_usleep_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, long long usec)
{
    switch (errnum)
    {
    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        break;

    case EINVAL:
        if (usec >= 1000000)
        {
            explain_buffer_einval_out_of_range(sb, "usec", 0, 999999);
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
explain_buffer_errno_usleep(explain_string_buffer_t *sb, int errnum, long long
    usec)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_usleep_system_call(&exp.system_call_sb, errnum, usec);
    explain_buffer_errno_usleep_explanation(&exp.explanation_sb, errnum,
        "usleep", usec);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
