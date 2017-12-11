/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/buffer/rwx.h>


void
explain_buffer_rwx(explain_string_buffer_t *sb, int mode_bits)
{
    explain_string_buffer_putc(sb, '"');
    explain_string_buffer_putc(sb, ((mode_bits & 0444) ? 'r' : '-'));
    explain_string_buffer_putc(sb, ((mode_bits & 0222) ? 'w' : '-'));
    explain_string_buffer_putc(sb, ((mode_bits & 0111) ? 'x' : '-'));
    explain_string_buffer_putc(sb, '"');
}


/* vim: set ts=8 sw=4 et : */
