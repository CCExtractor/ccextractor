/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#include <libexplain/ac/linux/if_eql.h>
#include <libexplain/ac/net/if.h>

#include <libexplain/buffer/ifreq/master_config.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#if defined(EQL_GETMASTRCFG) || defined(EQL_SETMASTRCFG)

void
explain_buffer_ifreq_master_config(explain_string_buffer_t *sb,
    const struct ifreq *data, int extended_form)
{
    const struct master_config *m;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ifr_data = ");
    m = (const struct master_config *)data->ifr_data;
    if (!extended_form || explain_is_efault_pointer(m, sizeof(*m)))
    {
        explain_buffer_pointer(sb, m);
    }
    else
    {
        explain_string_buffer_puts(sb, "{ master_name = ");
        explain_string_buffer_puts_quoted_n(sb, m->master_name,
            sizeof(m->master_name));
        if (m->max_slaves)
        {
            explain_string_buffer_puts(sb, ", max_slaves = ");
            explain_buffer_int(sb, m->max_slaves);
        }
        if (m->min_slaves)
        {
            explain_string_buffer_puts(sb, ", min_slaves = ");
            explain_buffer_int(sb, m->min_slaves);
        }
        explain_string_buffer_puts(sb, " }");
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_ifreq_master_config(explain_string_buffer_t *sb,
    const struct ifreq *data, int extended_form)
{
    (void)extended_form;
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
