/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/fpathconf.h>
#include <libexplain/buffer/errno/pathconf.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/pathconf_name.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_fpathconf_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes, int name)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "fpathconf(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_puts(sb, ", name = ");
    explain_buffer_pathconf_name(sb, name);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_fpathconf_explanation(explain_string_buffer_t *sb,
    int errnum, int fildes, int name)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/fpathconf.html
     */
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EINVAL:
        explain_buffer_pathconf_einval(sb, "fildes", name, "name");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "fpathconf");
        break;
    }
}


void
explain_buffer_errno_fpathconf(explain_string_buffer_t *sb, int errnum,
    int fildes, int name)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fpathconf_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes,
        name
    );
    explain_buffer_errno_fpathconf_explanation
    (
        &exp.explanation_sb,
        errnum,
        fildes,
        name
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
