/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2012, 2013 Peter Miller
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

#include <libexplain/ac/fcntl.h> /* for AT_FDCWD */

#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/utimensat_fildes.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>


void
explain_buffer_utimensat_fildes(explain_string_buffer_t *sb, int value)
{
    if (value == AT_FDCWD)
        explain_string_buffer_puts(sb, "AT_FDCWD");
    else
        explain_buffer_fildes(sb, value);
}


int
explain_parse_utimensat_fildes_or_die(const char *text, const char *caption)
{
    /* the explain_parse_bits code already groks the AT_FDCWD constant. */
    return explain_parse_bits_or_die(text, 0, 0, caption);
}


/* vim: set ts=8 sw=4 et : */
