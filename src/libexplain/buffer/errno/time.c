/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/time.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_time_system_call(explain_string_buffer_t *sb, int errnum,
    time_t *t)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "time(t = ");
    explain_buffer_pointer(sb, t);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_time_explanation(explain_string_buffer_t *sb, int errnum,
    time_t *t)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/time.html
     */
    (void)t;
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "t");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "time");
        break;
    }
}


void
explain_buffer_errno_time(explain_string_buffer_t *sb, int errnum, time_t *t)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_time_system_call(&exp.system_call_sb, errnum, t);
    explain_buffer_errno_time_explanation(&exp.explanation_sb, errnum, t);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
