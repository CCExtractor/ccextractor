/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013 Peter Miller
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
#include <libexplain/ac/fcntl.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/emfile.h>
#include <libexplain/buffer/enfile.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/pipe2.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/open_flags.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_pipe2_system_call(explain_string_buffer_t *sb, int errnum,
    int *fildes, int flags)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "pipe2(fildes = ");
    explain_buffer_pointer(sb, fildes);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_open_flags(sb, flags);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_pipe2_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, int *fildes, int flags)
{
    switch (errnum)
    {
    case EFAULT:
        if (!fildes)
        {
            explain_buffer_is_the_null_pointer(sb, "fildes");
            break;
        }
        explain_buffer_efault(sb, "fildes");
        break;

    case EINVAL:
        if (flags & ~(O_NONBLOCK | O_CLOEXEC))
        {
            explain_buffer_einval_bits(sb, "flags");
            break;
        }
        goto generic;

    case EMFILE:
        explain_buffer_emfile(sb);
        break;

    case ENFILE:
        explain_buffer_enfile(sb);
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_pipe2(explain_string_buffer_t *sb, int errnum, int *fildes,
    int flags)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_pipe2_system_call(&exp.system_call_sb, errnum, fildes,
        flags);
    explain_buffer_errno_pipe2_explanation(&exp.explanation_sb, errnum, "pipe2",
        fildes, flags);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
