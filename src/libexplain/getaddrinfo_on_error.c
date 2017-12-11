/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2012, 2013 Peter Miller
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

#include <libexplain/ac/assert.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/netdb.h>

#include <libexplain/getaddrinfo.h>
#include <libexplain/output.h>


int
explain_getaddrinfo_on_error(const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **res)
{
    int             result;

    result = getaddrinfo(node, service, hints, res);
    assert(EAI_SYSTEM < 0);
    if (result == EAI_SYSTEM)
        result = errno;
    if (result)
    {
        /* don't need hold_errno */
        explain_output_message
        (
            explain_errcode_getaddrinfo(result, node, service, hints, res)
        );
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
