/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#include <libexplain/ac/string.h>

#include <libexplain/buffer/signal.h>
#include <libexplain/buffer/strsignal.h>
#include <libexplain/option.h>


void
explain_buffer_strsignal(explain_string_buffer_t *sb, int signum)
{
    const char      *cp;
    size_t          len;
    const char      *colon;

    /*
     * On some systems (but not on Linux), a NULL pointer may be
     * returned instead for an invalid signal number.
     */
    cp = strsignal(signum);
    if (!cp)
        cp = "unknown";

    /*
     * FreeDSB (and probably other systems) add the signal number to the string
     * return by strsignal.  If present, do not print it, we have our own way;
     * and it also caled false negatives in the test suite.
     */
    len = strlen(cp);
    colon = memchr(cp, ':', len);
    if (colon)
        len = colon - cp;

    explain_string_buffer_write(sb, cp, len);

    if (explain_option_dialect_specific())
    {
        explain_string_buffer_printf(sb, " (%d)", signum);
    }
}


/* vim: set ts=8 sw=4 et : */
