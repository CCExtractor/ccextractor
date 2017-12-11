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

#include <libexplain/ac/net/if.h>

#include <libexplain/buffer/ifconf.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_STRUCT_IFCONF

void
explain_buffer_ifconf(explain_string_buffer_t *sb,
    const struct ifconf *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        const struct ifconf *p;

        p = data;
        explain_string_buffer_printf
        (
            sb,
            "{ ifc_len = %d, ifc_req = ",
            p->ifc_len
        );
        explain_buffer_pointer(sb, p->ifc_buf);
        explain_string_buffer_puts(sb, " }");
    }
}

#else

void
explain_buffer_ifconf(explain_string_buffer_t *sb,
    const struct ifconf *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
