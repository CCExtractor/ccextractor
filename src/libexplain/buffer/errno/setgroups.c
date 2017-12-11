/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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
#include <libexplain/ac/limits.h> /* for NGROUPS_MAX on Solaris */

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setgroups.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_setgroups_system_call(explain_string_buffer_t *sb, int
    errnum, size_t data_size, const gid_t *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setgroups(data_size = ");
    explain_buffer_size_t(sb, data_size);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_setgroups_explanation(explain_string_buffer_t *sb, int
    errnum, size_t data_size, const gid_t *data)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setgroups.html
     */
    (void)data;
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINVAL:
        explain_buffer_einval_too_large(sb, "data_size");
        if (data_size > NGROUPS_MAX && explain_option_dialect_specific())
        {
            explain_string_buffer_printf
            (
                sb,
                " (%lu > %lu)",
                (unsigned long)data_size,
                (unsigned long)NGROUPS_MAX
            );
        }
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case EPERM:
        explain_buffer_eperm_setgid(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "setgroups");
        break;
    }
}


void
explain_buffer_errno_setgroups(explain_string_buffer_t *sb, int errnum, size_t
    data_size, const gid_t *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setgroups_system_call(&exp.system_call_sb, errnum,
        data_size, data);
    explain_buffer_errno_setgroups_explanation(&exp.explanation_sb, errnum,
        data_size, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
