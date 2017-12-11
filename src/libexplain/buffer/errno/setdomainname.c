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

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setdomainname.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/buffer/string_n.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_setdomainname_system_call(explain_string_buffer_t *sb, int
    errnum, const char *data, size_t data_size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setdomainname(data = ");
    explain_buffer_string_n(sb, data, data_size);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_size_t(sb, data_size);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_setdomainname_explanation(explain_string_buffer_t *sb,
    int errnum, const char *data, size_t data_size)
{
    (void)data;
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINVAL:
        if ((int)data_size < 0)
            explain_buffer_einval_too_small(sb, "data_size", data_size);
        else
            explain_buffer_einval_too_large(sb, "data_size");
        break;

    case EPERM:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: this error message is issued when a process
             * attempts to set the domain name without sufficient privilege.
             */
            i18n("the process does not have permission to set the domain name")
        );
        explain_buffer_dac_sys_admin(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "setdomainname");
        break;
    }
}


void
explain_buffer_errno_setdomainname(explain_string_buffer_t *sb, int errnum,
    const char *data, size_t data_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setdomainname_system_call(&exp.system_call_sb, errnum,
        data, data_size);
    explain_buffer_errno_setdomainname_explanation(&exp.explanation_sb, errnum,
        data, data_size);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
