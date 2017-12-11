/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enoprotoopt.h>
#include <libexplain/buffer/enotsock.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/getsockopt.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/socklen.h>
#include <libexplain/buffer/sockopt_level.h>
#include <libexplain/buffer/sockopt_name.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_getsockopt_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, int level, int name, void *data,
    socklen_t *data_size)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "getsockopt(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", level = ");
    explain_buffer_sockopt_level(sb, level);
    explain_string_buffer_puts(sb, ", name = ");
    explain_buffer_sockopt_name(sb, level, name);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_socklen_star(sb, data_size);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_getsockopt_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes, int level, int name, void *data,
    socklen_t *data_size)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/getsockopt.html
     */
    (void)level;
    (void)name;
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EFAULT:
        if (explain_is_efault_pointer(data_size, sizeof(*data_size)))
        {
            explain_buffer_efault(sb, "data_size");
            break;
        }
        if (explain_is_efault_pointer(data, *data_size))
        {
            explain_buffer_efault(sb, "data");
            break;
        }
        goto dunno;

    case EINVAL:
        if (explain_is_efault_pointer(data_size, sizeof(*data_size)))
            goto dunno;
        explain_buffer_einval_too_small(sb, "data_size", *data_size);
        break;

    case ENOPROTOOPT:
        explain_buffer_enoprotoopt(sb, "name");
        break;

    case ENOTSOCK:
        explain_buffer_enotsock(sb, fildes, "fildes");
        break;

    default:
        dunno:
        explain_buffer_errno_generic(sb, errnum, "getsockopt");
        break;
    }
}


void
explain_buffer_errno_getsockopt(explain_string_buffer_t *sb, int errnum,
    int fildes, int level, int name, void *data, socklen_t *data_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_getsockopt_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        level,
        name,
        data,
        data_size
    );
    explain_buffer_errno_getsockopt_explanation
    (
        &exp.explanation_sb,
        errnum,
        fildes,
        level,
        name,
        data,
        data_size
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
