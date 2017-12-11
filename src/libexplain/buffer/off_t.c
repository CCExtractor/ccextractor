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

#include <libexplain/buffer/off_t.h>


void
explain_buffer_off_t(explain_string_buffer_t *sb, off_t value)
{
    /*
     * This is tricky.  On BSD* it is always 64 bits.  On Linux the
     * number of bits depends on things discovered by ./configure,
     * usually 64-bits the way libexplain uses it.
     *
     * It is also slightly undefined wherther or not off_t is signed.
     * The code will need to be sanitized if it can ever be unsigned.
     * (Is the a soff_t to go with it, analogous to ssize_t for size_t?)
     *
     * Is there an off_t modifier for printf, analogous to 'z' for size_t?
     */
#if SIZEOF_OFF_T > SIZEOF_LONG
    explain_string_buffer_printf(sb, "%lld", (long long)value);
#else
    explain_string_buffer_printf(sb, "%ld", (long)value);
#endif
}


/* vim: set ts=8 sw=4 et : */
