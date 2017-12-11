/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2012, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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

#include <libexplain/ac/sys/resource.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/long_long.h>
#include <libexplain/buffer/rlimit.h>
#include <libexplain/string_buffer.h>


void
explain_buffer_rlim_t(explain_string_buffer_t *sb, rlim_t rlim)
{
    if (rlim == RLIM_INFINITY)
        explain_string_buffer_puts(sb, "RLIM_INFINITY");
    else if (sizeof(rlim) <= sizeof(int))
        explain_buffer_int(sb, rlim);
    else if (sizeof(rlim) <= sizeof(long))
        explain_buffer_long(sb, rlim);
    else
        explain_buffer_long_long(sb, rlim);
}


void
explain_buffer_rlimit(explain_string_buffer_t *sb, const struct rlimit *p)
{
    explain_string_buffer_puts(sb, "{ rlim_cur = ");
    explain_buffer_rlim_t(sb, p->rlim_cur);
    if (p->rlim_cur != p->rlim_max)
    {
        explain_string_buffer_puts(sb, ", rlim_max = ");
        explain_buffer_rlim_t(sb, p->rlim_max);
    }
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
