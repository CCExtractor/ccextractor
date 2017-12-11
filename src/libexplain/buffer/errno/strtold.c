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
#include <libexplain/ac/stdlib.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/erange.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/strtold.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_strtold_system_call(explain_string_buffer_t *sb,
    int errnum, const char *nptr, char **endptr)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "strtold(nptr = ");
    explain_buffer_pathname(sb, nptr);
    explain_string_buffer_puts(sb, ", endptr = ");
    explain_buffer_pointer(sb, endptr);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_strtold_explanation(explain_string_buffer_t *sb,
    int errnum, const char *nptr, char **endptr)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/strtold.html
     */
    switch (errnum)
    {
    case EINVAL:
        if (endptr && !explain_is_efault_pointer(endptr, sizeof(*endptr)))
        {
            if (*endptr == nptr)
            {
                explain_buffer_einval_not_a_number(sb, "nptr");
                break;
            }
        }
        else
        {
            char            *ep;
            int             err;

            /*
             * Do the conversion again, this time *with* an end pointer,
             * to see if there were any digits recognised.
             */
            ep = 0;
            err = errno;
            if (explain_ac_strtold(nptr, &ep))
            {
                /*
                 * Ordinarily, the warning "ignoring return value of function"
                 * is a great way of discovering bugs.  In this case, however,
                 * we aren't concerned with the actual return value, but with
                 * the error that results from the call.
                 */
            }
            errno = err;
            if (ep == nptr)
            {
                explain_buffer_einval_not_a_number(sb, "nptr");
                break;
            }
        }
        goto generic;

    case ERANGE:
        explain_buffer_erange(sb);
        break;

    default:
        generic:
        explain_buffer_errno_generic(sb, errnum, "strtold");
        break;
    }
}


void
explain_buffer_errno_strtold(explain_string_buffer_t *sb, int errnum,
    const char *nptr, char **endptr)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_strtold_system_call(&exp.system_call_sb, errnum, nptr,
        endptr);
    explain_buffer_errno_strtold_explanation(&exp.explanation_sb, errnum, nptr,
        endptr);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
