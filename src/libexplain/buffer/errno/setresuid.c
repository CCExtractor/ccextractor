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
#include <libexplain/buffer/errno/setresuid.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_setresuid_system_call(explain_string_buffer_t *sb,
    int errnum, uid_t ruid, uid_t euid, uid_t suid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setresuid(ruid = ");
    explain_buffer_uid(sb, ruid);
    explain_string_buffer_puts(sb, ", euid = ");
    explain_buffer_uid(sb, euid);
    explain_string_buffer_puts(sb, ", suid = ");
    explain_buffer_uid(sb, suid);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_eperm_setresuid(explain_string_buffer_t *sb, const char *caption,
    const char *ruid_str, const char *euid_str, const char *suid_str)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        i18n
        (
            /*
             * xgettext: This error message is used to explain an EPERM error
             * reported by the setresuid system call, in the case where uid does
             * not match the real user ID or effective user ID or saved user ID
             * of the calling process, and the user is not privileged (Linux:
             * does not have the CAP_SETUID capability)
             */
            "the %1$s argument does not match the real user ID (%2$s) or the "
            "effective user ID (%3$s) or the saved user ID (%4$s) of the "
            "calling process"
        ),
        caption,
        ruid_str,
        euid_str,
        suid_str
    );
    explain_buffer_dac_setuid(sb);
}


void
explain_buffer_errno_setresuid_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, uid_t ruid, uid_t euid, uid_t suid)
{
    uid_t           cruid = -1;
    char            ruid_str[100];
    uid_t           ceuid = -1;
    char            euid_str[100];
    uid_t           csuid = -1;
    char            suid_str[100];
    const uid_t     minus1 = -1;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setresuid.html
     */
    switch (errnum)
    {
    case EAGAIN:
#if defined(EWOULDBLOCK) && EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
        if (ruid != getuid())
        {
            explain_buffer_eagain_setuid(sb, "ruid");
            break;
        }
        explain_buffer_eagain(sb, syscall_name);
        break;

    case EPERM:
        getresuid(&cruid, &ceuid, &csuid);

        /* build the strings we will need to report the error */
        {
            explain_string_buffer_t ruid_sb;
            explain_string_buffer_t euid_sb;
            explain_string_buffer_t suid_sb;

            explain_string_buffer_init(&ruid_sb, ruid_str, sizeof(ruid_str));
            explain_buffer_uid(&ruid_sb, cruid);
            explain_string_buffer_init(&euid_sb, euid_str, sizeof(euid_str));
            explain_buffer_uid(&euid_sb, ceuid);
            explain_string_buffer_init(&suid_sb, suid_str, sizeof(suid_str));
            explain_buffer_uid(&suid_sb, csuid);
        }

        if (ruid != minus1 && ruid != cruid && ruid != ceuid && ruid != csuid)
        {
            explain_buffer_eperm_setresuid(sb, "ruid", ruid_str, euid_str,
                suid_str);
            break;
        }
        if (euid != minus1 && euid != cruid && euid != ceuid && euid != csuid)
        {
            explain_buffer_eperm_setresuid(sb, "euid", ruid_str, euid_str,
                suid_str);
            break;
        }
        if (suid != minus1 && suid != cruid && suid != ceuid && suid != csuid)
        {
            explain_buffer_eperm_setresuid(sb, "suid", ruid_str, euid_str,
                suid_str);
            break;
        }
        /* Fall through... */

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_setresuid(explain_string_buffer_t *sb, int errnum,
    uid_t ruid, uid_t euid, uid_t suid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setresuid_system_call(&exp.system_call_sb, errnum,
        ruid, euid, suid);
    explain_buffer_errno_setresuid_explanation(&exp.explanation_sb, errnum,
        "setresuid", ruid, euid, suid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
