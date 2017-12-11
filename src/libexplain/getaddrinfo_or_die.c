/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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


void
explain_getaddrinfo_or_die(const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **res)
{
    if (explain_getaddrinfo_on_error(node, service, hints, res))
    {
        explain_output_exit_failure();
    }
}


/* vim: set ts=8 sw=4 et : */
