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
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eagain.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setuid.h>
#include <libexplain/buffer/rlimit.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_setuid_system_call(explain_string_buffer_t *sb, int errnum,
    uid_t uid)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setuid(uid = ");
    explain_buffer_uid(sb, uid);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_setuid_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, uid_t uid)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setuid.html
     */
    switch (errnum)
    {
    case EAGAIN:
#if defined(EWOULDBLOCK) && EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
        if (uid != getuid())
        {
            explain_buffer_eagain_setuid(sb, "uid");
            break;
        }
        explain_buffer_eagain(sb, syscall_name);
        break;

    case EPERM:
        /*
         * uid does not match the real user ID or saved user ID of the
         * calling process, and the user is not privileged (Linux: does
         * not have the CAP_SETUID capability).
         */
        {
            uid_t ruid;
            uid_t euid;
            uid_t suid;
#ifdef HAVE_GETRESUID
            if (getresuid(&ruid, &euid, &suid) < 0)
            {
                /* unlikely, only possible error is EFAULT */
                ruid = getuid();
                euid = ruid;
                suid = ruid;
            }
#elif defined(GETREUID)
            if (getreuid(&ruid, &euid) < 0)
            {
                /* unlikely, only possible error is EFAULT */
                ruid = getuid();
                euid = ruid;
            }
            suid = ruid;
#else
            ruid = getuid();
            euid = ruid;
            suid = ruid;
#endif
            if (uid != ruid && uid != suid)
            {
                char ruid_str[100];
                explain_string_buffer_t ruid_sb;
                char suid_str[100];
                explain_string_buffer_t suid_sb;

                explain_string_buffer_init(&ruid_sb, ruid_str,
                    sizeof(ruid_str));
                explain_buffer_uid(&ruid_sb, ruid);
                explain_string_buffer_init(&suid_sb, suid_str,
                    sizeof(suid_str));
                explain_buffer_uid(&suid_sb, suid);

                explain_string_buffer_printf_gettext
                (
                    sb,
                    i18n
                    (
                        /*
                         * xgettext: This error message is used to explain an
                         * EPERM error reported by the setuid system call, in
                         * the case where uid does not match the real ser ID or
                         * saved user ID of the calling process, and the user is
                         * not privileged (Linux: does not have the CAP_SETUID
                         * capability)
                         */
                        "uid does not match the real user ID (%1$s) or "
                        "the saved user ID (%2$s) of the calling process"
                    ),
                    ruid_str,
                    suid_str
                );
                explain_buffer_dac_setuid(sb);
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
explain_buffer_errno_setuid(explain_string_buffer_t *sb, int errnum, uid_t uid)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setuid_system_call(&exp.system_call_sb, errnum, uid);
    explain_buffer_errno_setuid_explanation(&exp.explanation_sb, errnum,
        "setuid", uid);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
