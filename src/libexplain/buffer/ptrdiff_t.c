/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#include <libexplain/buffer/ptrdiff_t.h>


void
explain_buffer_ptrdiff_t(explain_string_buffer_t *sb, ptrdiff_t value)
{
#if SIZEOF_PTRDIFF_T > SIZEOF_LONG
    explain_string_buffer_printf(sb, "%lld", (long long)value);
#elif SIZEOF_PTRDIFF_T > SIZEOF_INT
    explain_string_buffer_printf(sb, "%ld", (long)value);
#else
    explain_string_buffer_printf(sb, "%d", (int)value);
#endif
}


/* vim: set ts=8 sw=4 et : */
