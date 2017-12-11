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
#include <libexplain/ac/sys/param.h>
#include <libexplain/ac/sys/timex.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/adjtimex.h>
#include <libexplain/buffer/timex.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_adjtimex_system_call(explain_string_buffer_t *sb, int
    errnum, struct timex *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "adjtimex(data = ");
    explain_buffer_timex(sb, data);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_adjtimex_explanation(explain_string_buffer_t *sb, int
    errnum, struct timex *data)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/adjtimex.html
     */
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINVAL:
#ifdef HAVE_SYS_TIMEX_H
        if (explain_is_efault_pointer(data, sizeof(*data)))
        {
            explain_buffer_einval_vague(sb, "data");
            break;
        }
#ifdef ADJ_OFFSET
        if (data->modes & ADJ_OFFSET)
        {
            long lo = -131071;
            long hi = 131071;
            if (data->offset < lo || data->offset > hi)
            {
                explain_buffer_einval_out_of_range(sb, "data->offset", lo, hi);
                break;
            }
        }
#endif
#ifdef ADJ_STATUS
        if (data->modes & ADJ_STATUS)
        {
            switch (data->status)
            {
            case TIME_OK:
            case TIME_INS:
            case TIME_DEL:
            case TIME_OOP:
            case TIME_WAIT:
#ifdef TIME_BAD
            case TIME_BAD:
#endif
                break;

            default:
                explain_buffer_einval_vague(sb, "data->status");
                return;
            }
        }
#endif
#ifdef ADJ_TICK
        if (data->modes & ADJ_TICK)
        {
            long lo = 900000/HZ;
            long hi = 1100000/HZ;
            if (data->tick < lo || data->tick > hi)
            {
                explain_buffer_einval_out_of_range(sb, "data->tick", lo, hi);
                break;
            }
        }
#endif
#endif
        explain_buffer_einval_vague(sb, "data");
        break;

    case ENOSYS:
        explain_buffer_enosys_vague(sb, "adjtimex");
        break;

    case EPERM:
        explain_buffer_eperm_sys_time(sb, "adjtimex");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "adjtimex");
        break;
    }
}


void
explain_buffer_errno_adjtimex(explain_string_buffer_t *sb, int errnum, struct
    timex *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_adjtimex_system_call(&exp.system_call_sb, errnum,
        data);
    explain_buffer_errno_adjtimex_explanation(&exp.explanation_sb, errnum,
        data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
