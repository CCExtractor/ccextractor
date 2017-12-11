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
#include <libexplain/ac/sys/signalfd.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/emfile.h>
#include <libexplain/buffer/enfile.h>
#include <libexplain/buffer/enodev.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/signalfd.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/signalfd_flags.h>
#include <libexplain/buffer/sigset_t.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_signalfd_system_call(explain_string_buffer_t *sb, int
    errnum, int fildes, const sigset_t *mask, int flags)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "signalfd(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", mask = ");
    explain_buffer_sigset_t(sb, mask);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_signalfd_flags(sb, flags);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_signalfd_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int fildes, const sigset_t *mask,
    int flags)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/signalfd.html
     */
    (void)mask;
    (void)flags;
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EFAULT:
        explain_buffer_efault(sb, "mask");
        break;

    case EINVAL:
        if (fildes >= 0)
            explain_buffer_einval_signalfd(sb, "fildes");
        else
            explain_buffer_einval_bits(sb, "flags");
        break;

    case EMFILE:
        explain_buffer_emfile(sb);
        break;

    case ENFILE:
        explain_buffer_enfile(sb);
        break;

    case ENODEV:
        explain_buffer_enodev_anon_inodes(sb, syscall_name);
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOSYS:
        explain_buffer_enosys_vague(sb, syscall_name);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_signalfd(explain_string_buffer_t *sb, int errnum, int
    fildes, const sigset_t *mask, int flags)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_signalfd_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        mask,
        flags
    );
    explain_buffer_errno_signalfd_explanation
    (
        &exp.explanation_sb,
        errnum,
        "signalfd",
        fildes,
        mask,
        flags
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
