/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/dev_t.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/ustat.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_ustat_system_call(explain_string_buffer_t *sb, int errnum,
    dev_t dev, struct ustat *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "ustat(dev = ");
    explain_buffer_dev_t(sb, dev);
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_ustat_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, dev_t dev, struct ustat *data)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/ustat.html
     */
    (void)dev;
    (void)data;
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINVAL:
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This error message is issued to explain an EINVAL error
             * reported by the ustat syatem call, in the case where the devive
             * specified does not contain a mounted file system.
             *
             * %1$s => the name of the offending system call argument
             */
            i18n("the %s argument does not refer to a device containing "
                "a mounted file system"),
            "dev"
        );
        break;

    case ENOSYS:
#if defined(EOPNOTSUPP) && ENOSYS != EOPNOTSUPP
    case EOPNOTSUPP:
#endif
        explain_buffer_enosys_vague(sb, syscall_name);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_ustat(explain_string_buffer_t *sb, int errnum, dev_t dev,
    struct ustat *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_ustat_system_call(&exp.system_call_sb, errnum, dev,
        data);
    explain_buffer_errno_ustat_explanation(&exp.explanation_sb, errnum, "ustat",
        dev, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
