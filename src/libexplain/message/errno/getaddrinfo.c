/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/buffer/errno/getaddrinfo.h>
#include <libexplain/getaddrinfo.h>
#include <libexplain/string_buffer.h>


void
explain_message_errcode_getaddrinfo(char *message, int message_size,
    int errcode, const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **res)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errcode_getaddrinfo
    (
        &sb,
        errcode,
        node,
        service,
        hints,
        res
    );
}


/* vim: set ts=8 sw=4 et : */
