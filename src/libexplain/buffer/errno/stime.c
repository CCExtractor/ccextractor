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

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/stime.h>
#include <libexplain/buffer/time_t.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_stime_system_call(explain_string_buffer_t *sb, int errnum,
    const time_t *t)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "stime(t = ");
    explain_buffer_time_t_star(sb, t);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_stime_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, const time_t *t)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/stime.html
     */
    (void)t;

    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "t");
        break;

    case EPERM:
        explain_buffer_eperm_sys_time(sb, "stime");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_stime(explain_string_buffer_t *sb, int errnum,
    const time_t *t)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_stime_system_call(&exp.system_call_sb, errnum, t);
    explain_buffer_errno_stime_explanation(&exp.explanation_sb, errnum, "stime",
        t);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
