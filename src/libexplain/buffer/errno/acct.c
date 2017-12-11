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

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/eisdir.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enfile.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/erofs.h>
#include <libexplain/buffer/errno/acct.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_acct_system_call(explain_string_buffer_t *sb, int errnum,
    const char *pathname)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "acct(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_acct_explanation(explain_string_buffer_t *sb, int errnum,
    const char *pathname)
{
    explain_final_t final_component;

    explain_final_init(&final_component);

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/acct.html
     */
    switch (errnum)
    {
    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EFAULT:
        explain_buffer_efault(sb, pathname);
        break;

    case EIO:
        explain_buffer_eio(sb);
        break;

    case EISDIR:
        if (explain_buffer_eisdir(sb, pathname, "pathname"))
            break;
        goto generic;

    case ELOOP:
        explain_buffer_eloop(sb, pathname, "pathname", &final_component);
        break;

    case ENAMETOOLONG:
        explain_buffer_enametoolong(sb, pathname, "pathname", &final_component);
        break;

    case ENFILE:
        explain_buffer_enfile(sb);
        break;

    case ENOENT:
        explain_buffer_enoent(sb, pathname, "pathname", &final_component);
        break;

    case ENOMEM:
#ifdef EUSERS
    case EUSERS:
#endif
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOSYS:
#if defined(EOPNOTSUPP) && EOPNOTSUPP != ENOSYS
    case EOPNOTSUPP:
#endif
#ifdef __linux__
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: this error message is issued when the acct(2)
             * system call is used against a Linux kernel that does not
             * have BSD process accounting compiled in.
             */
            i18n("BSD process accounting has not been enabled when the "
            "operating system kernel was compiled (the kernel configuration "
            "parameter controlling this feature is CONFIG_BSD_PROCESS_ACCT)")
        );
        break;
#else
        explain_buffer_enosys_pathname(sb, pathname, "pathname", "acct");
        break;
#endif

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    case EPERM:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued when a process
             * attempts to enable or disable process accounting without
             * sufficient privilege.
             */
            i18n("the process has insufficient privilege to control process "
                "accounting")
        );
        explain_buffer_dac_sys_pacct(sb);
        break;

    case EROFS:
        explain_buffer_erofs(sb, pathname, "pathname");
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, "acct");
        break;
    }
}


void
explain_buffer_errno_acct(explain_string_buffer_t *sb, int errnum, const char
    *pathname)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_acct_system_call(&exp.system_call_sb, errnum,
        pathname);
    explain_buffer_errno_acct_explanation(&exp.explanation_sb, errnum,
        pathname);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
