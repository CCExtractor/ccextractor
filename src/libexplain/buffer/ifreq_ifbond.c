/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/linux/if_bonding.h>

#include <libexplain/buffer/ifreq_ifbond.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_IF_BONDING_H

static void
explain_buffer_ifbond(explain_string_buffer_t *sb,
    const struct ifbond *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        const struct ifbond *p;

        p = data;
        explain_string_buffer_printf
        (
            sb,
            "{ bond_mode = %ld, num_slaves = %ld, miimon = %ld }",
            (long)p->bond_mode,
            (long)p->num_slaves,
            (long)p->miimon
        );
    }
}


void
explain_buffer_ifreq_ifbond(explain_string_buffer_t *sb,
    const struct ifreq *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        const struct ifreq *ifr;

        /*
         * This is actually a huge big sucky union.
         */
        ifr = data;
        explain_string_buffer_puts(sb, "{ ifr_name = ");
        explain_string_buffer_puts_quoted_n
        (
            sb,
            ifr->ifr_name,
            sizeof(ifr->ifr_name)
        );
        explain_string_buffer_puts(sb, ", ifr_data = ");
        explain_buffer_ifbond(sb, ifr->ifr_data);
        explain_string_buffer_puts(sb, " }");
    }
}

#else

void
explain_buffer_ifreq_ifbond(explain_string_buffer_t *sb,
    const struct ifreq *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
