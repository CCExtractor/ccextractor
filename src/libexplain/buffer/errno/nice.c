/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/nice.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_nice_system_call(explain_string_buffer_t *sb, int errnum,
    int inc)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "nice(inc = ");
    explain_string_buffer_printf(sb, "%d", inc);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_nice_explanation(explain_string_buffer_t *sb, int errnum,
    int inc)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/nice.html
     */
    (void)inc;
    switch (errnum)
    {
    case EPERM:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgetetxt:  This error message is issued to explain an EPERM error
             * of they nice system call, in the case where the calling process
             * attempted to increase its priority by supplying a negative value
             * but has insufficient privileges.
             */
            i18n("the process does not have permission to increase its "
                "priority")
        );
        break;

    case EINVAL:
        explain_buffer_einval_vague(sb, "inc");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "nice");
        break;
    }
}


void
explain_buffer_errno_nice(explain_string_buffer_t *sb, int errnum, int inc)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_nice_system_call(&exp.system_call_sb, errnum, inc);
    explain_buffer_errno_nice_explanation(&exp.explanation_sb, errnum, inc);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
