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

#include <libexplain/ac/arpa/inet.h>

#include <libexplain/buffer/in6_addr.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef AF_INET6

void
explain_buffer_in6_addr(explain_string_buffer_t *sb,
    const struct in6_addr *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        char            straddr[200];

        inet_ntop(AF_INET6, data, straddr, sizeof(straddr));
        explain_string_buffer_puts(sb, straddr);
    }
}

#else

void
explain_buffer_in6_addr(explain_string_buffer_t *sb,
    const struct in6_addr *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
