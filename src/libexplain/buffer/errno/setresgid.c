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
#include <libexplain/buffer/eagain.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setresgid.h>
#include <libexplain/buffer/gid.h>
#include <libexplain/explanation.h>


static void
explain_buffer_eperm_setresgid(explain_string_buffer_t *sb, const char *caption,
    const char *rgid_str, const char *egid_str, const char *sgid_str)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        i18n
        (
            /*
             * xgettext: This error message is used to explain an EPERM error
             * reported by the setresgid system call, in the case where gid does
             * not match the real group ID or saved group ID of the calling
             * process, and the process is not privileged (Linux: does not have
             * the CAP_SETGID capability)
             */
            "the %1$s argument does not match the real group ID (%2$s) or the "
            "effective group ID (%3$s) or the saved group ID (%4$s) of the "
            "calling process"
        ),
        caption,
        rgid_str,
        egid_str,
        sgid_str
    );
    explain_buffer_dac_setgid(sb);
}


static void
explain_buffer_errno_setresgid_system_call(explain_string_buffer_t *sb,
    int errnum, gid_t rgid, gid_t egid, gid_t sgid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setresgid(rgid = ");
    explain_buffer_gid(sb, rgid);
    explain_string_buffer_puts(sb, ", egid = ");
    explain_buffer_gid(sb, egid);
    explain_string_buffer_puts(sb, ", sgid = ");
    explain_buffer_gid(sb, sgid);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_setresgid_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, gid_t rgid, gid_t egid, gid_t sgid)
{
    gid_t           crgid = -1;
    char            rgid_str[100];
    gid_t           cegid = -1;
    char            egid_str[100];
    gid_t           csgid = -1;
    char            sgid_str[100];
    const gid_t     minus1 = -1;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setresgid.html
     */
    switch (errnum)
    {
    case EPERM:
        /* build the strings we will need to report the error */
        {
            explain_string_buffer_t rgid_sb;
            explain_string_buffer_t egid_sb;
            explain_string_buffer_t sgid_sb;

            explain_string_buffer_init(&rgid_sb, rgid_str, sizeof(rgid_str));
            explain_buffer_gid(&rgid_sb, crgid);
            explain_string_buffer_init(&egid_sb, egid_str, sizeof(egid_str));
            explain_buffer_gid(&egid_sb, cegid);
            explain_string_buffer_init(&sgid_sb, sgid_str, sizeof(sgid_str));
            explain_buffer_gid(&sgid_sb, csgid);
        }

        if (rgid != minus1 && rgid != crgid && rgid != cegid && rgid != csgid)
        {
            explain_buffer_eperm_setresgid(sb, "rgid", rgid_str, egid_str,
                sgid_str);
            break;
        }
        if (egid != minus1 && egid != crgid && egid != cegid && egid != csgid)
        {
            explain_buffer_eperm_setresgid(sb, "egid", rgid_str, egid_str,
                sgid_str);
            break;
        }
        if (sgid != minus1 && sgid != crgid && sgid != cegid && sgid != csgid)
        {
            explain_buffer_eperm_setresgid(sb, "sgid", rgid_str, egid_str,
                sgid_str);
            break;
        }
        /* Fall through... */

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_setresgid(explain_string_buffer_t *sb, int errnum, gid_t
    rgid, gid_t egid, gid_t sgid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setresgid_system_call(&exp.system_call_sb, errnum,
        rgid, egid, sgid);
    explain_buffer_errno_setresgid_explanation(&exp.explanation_sb, errnum,
        "setresgid", rgid, egid, sgid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
