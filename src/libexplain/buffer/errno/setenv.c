/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/setenv.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_setenv_system_call(explain_string_buffer_t *sb, int errnum,
    const char *name, const char *value, int overwrite)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "setenv(name = ");
    explain_buffer_pathname(sb, name);
    explain_string_buffer_puts(sb, ", value = ");
    explain_buffer_pathname(sb, value);
    explain_string_buffer_puts(sb, ", overwrite = ");
    explain_buffer_int(sb, overwrite);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_setenv_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, const char *name, const char *value,
    int overwrite)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/setenv.html
     */
    (void)value;
    (void)overwrite;
    switch (errnum)
    {
    case EINVAL:
        explain_buffer_einval_setenv(sb, name, "name");
        break;

    case ENOMEM:
        explain_buffer_enomem_user(sb, 0);
        break;

    case EFAULT:
        if (!name)
        {
            explain_buffer_is_the_null_pointer(sb, "name");
            break;
        }
        if (explain_is_efault_string(name))
        {
            explain_buffer_efault(sb, "name");
            break;
        }
        if (!value)
        {
            /*
             * While glibc treats value==NULL as meaning unsetenv, this is not
             * documented, and other systems may be less forgiving.
             */
            explain_buffer_is_the_null_pointer(sb, "value");
            break;
        }
        if (explain_is_efault_string(value))
        {
            explain_buffer_efault(sb, "value");
            break;
        }
        /* Fall through... */

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_setenv(explain_string_buffer_t *sb, int errnum, const char
    *name, const char *value, int overwrite)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_setenv_system_call(&exp.system_call_sb, errnum, name,
        value, overwrite);
    explain_buffer_errno_setenv_explanation(&exp.explanation_sb, errnum,
        "setenv", name, value, overwrite);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
