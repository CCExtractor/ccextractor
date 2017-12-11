/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2012, 2013 Peter Miller
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

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setgid.h>
#include <libexplain/buffer/gid.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_setgid_system_call(explain_string_buffer_t *sb, int errnum,
    gid_t gid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setgid(gid = ");
    explain_buffer_gid(sb, gid);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_setgid_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, gid_t gid)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setgid.html
     */
    switch (errnum)
    {
    case EPERM:
        /*
         * gid does not match the real group ID or saved group ID of the
         * calling process, and the user is not privileged (Linux: does
         * not have the CAP_SETGID capability).
         */
        {
            gid_t rgid;
            gid_t egid;
            gid_t sgid;
#ifdef HAVE_GETRESUID
            if (getresgid(&rgid, &egid, &sgid) < 0)
            {
                /* unlikely, only possible error is EFAULT */
                rgid = getgid();
                egid = rgid;
                sgid = rgid;
            }
#elif defined(GETREUID)
            if (getregid(&rgid, &egid) < 0)
            {
                /* unlikely, only possible error is EFAULT */
                rgid = getgid();
                egid = rgid;
            }
            sgid = rgid;
#else
            rgid = getgid();
            egid = rgid;
            sgid = rgid;
#endif
            if (gid != rgid && gid != sgid)
            {
                char rgid_str[100];
                explain_string_buffer_t rgid_sb;
                char sgid_str[100];
                explain_string_buffer_t sgid_sb;

                explain_string_buffer_init(&rgid_sb, rgid_str,
                    sizeof(rgid_str));
                explain_buffer_gid(&rgid_sb, rgid);
                explain_string_buffer_init(&sgid_sb, sgid_str,
                    sizeof(sgid_str));
                explain_buffer_gid(&sgid_sb, sgid);

                explain_string_buffer_printf_gettext
                (
                    sb,
                    i18n
                    (
                        /*
                         * xgettext: This error message is used to explain an
                         * EPERM error reported by the setgid system call, in
                         * the case where gid does not match the real group ID
                         * or saved group ID of the calling process, and the
                         * user is not privileged (Linux: does not have the
                         * CAP_SETUID capability)
                         */
                        "gid does not match the real group ID (%1$s) or "
                        "the saved group ID (%2$s) of the calling process"
                    ),
                    rgid_str,
                    sgid_str
                );
                explain_buffer_dac_setgid(sb);
                break;
            }
        }
        /* Fall through... */

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_setgid(explain_string_buffer_t *sb, int errnum, gid_t gid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setgid_system_call(&exp.system_call_sb, errnum, gid);
    explain_buffer_errno_setgid_explanation(&exp.explanation_sb, errnum,
        "setgid", gid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
