/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013, 2014 Peter Miller
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

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/emfile.h>
#include <libexplain/buffer/enfile.h>
#include <libexplain/buffer/enodev.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/errno/eventfd.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/eventfd_flags.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_eventfd_system_call(explain_string_buffer_t *sb, int
    errnum, unsigned initval, int flags)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "eventfd(initval = ");
    explain_string_buffer_printf(sb, "%u", initval);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_eventfd_flags(sb, flags);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_eventfd_explanation(explain_string_buffer_t *sb,
    int errnum, unsigned int initval, int flags)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/eventfd.html
     */
    (void)initval;
    (void)flags;
    switch (errnum)
    {
    case EINVAL:
        /*
         * Flags is invalid; or,
         * in Linux 2.6.26 or earlier, flags is non-zero.
         */
        explain_buffer_einval_bits(sb, "flags");
        break;

    case EMFILE:
        explain_buffer_emfile(sb);
        break;

    case ENFILE:
        explain_buffer_enfile(sb);
        break;

    case ENODEV:
        explain_buffer_enodev_anon_inodes(sb, "eventfd");
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    case ENOSYS:
        explain_buffer_enosys_vague(sb, "eventfd");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "eventfd");
        break;
    }
}


void
explain_buffer_errno_eventfd(explain_string_buffer_t *sb, int errnum,
    unsigned int initval, int flags)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_eventfd_system_call(&exp.system_call_sb, errnum,
        initval, flags);
    explain_buffer_errno_eventfd_explanation(&exp.explanation_sb, errnum,
        initval, flags);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
