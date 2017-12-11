/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setsid.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_setsid_system_call(explain_string_buffer_t *sb, int errnum)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setsid()");
}


void
explain_buffer_errno_setsid_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setsid.html
     */
    switch (errnum)
    {
    case EPERM:
        /*
         * The process group ID of any process equals the PID of the
         * calling process.  Thus, in particular, setsid() fails if the
         * calling process is already a process group leader.
         */
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: this error message is used to explain an EPERM error
             * returned by the setsid system call, in the case where the calling
             * process is already a process group leader.
             */
            i18n("the process is already a process group leader")
        );
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_setsid(explain_string_buffer_t *sb, int errnum)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setsid_system_call(&exp.system_call_sb, errnum);
    explain_buffer_errno_setsid_explanation(&exp.explanation_sb, errnum,
        "setsid");
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
