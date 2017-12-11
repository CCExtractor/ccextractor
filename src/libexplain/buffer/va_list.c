/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2010, 2013 Peter Miller
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

#include <libexplain/buffer/va_list.h>
#include <libexplain/buffer/pointer.h>


void
explain_buffer_va_list(explain_string_buffer_t *sb, va_list data)
{
#if VA_LIST_VOID_STAR
    explain_buffer_pointer(sb, data);
#else
    /*
     * This string is what the automatic tests turn the address into,
     * when va_list is compatable with void *. This prevents false
     * negatives, while being no less informative for the user.
     */
    (void)data;
    explain_string_buffer_puts(sb, "0xNNNNNNNN");
#endif
}


/* vim: set ts=8 sw=4 et : */
