/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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

#include <libexplain/ac/sys/time.h>
#include <libexplain/ac/sys/resource.h>

#include <libexplain/buffer/prio_which.h>
#include <libexplain/parse_bits.h>


static const struct explain_parse_bits_table_t table[] =
{
#ifdef PRIO_PROCESS
    { "PRIO_PROCESS", PRIO_PROCESS },
#endif
#ifdef PRIO_PGRP
    { "PRIO_PGRP", PRIO_PGRP },
#endif
#ifdef PRIO_USER
    { "PRIO_USER", PRIO_USER },
#endif
};


void
explain_buffer_prio_which(explain_string_buffer_t *sb, int value)
{
    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


int
explain_parse_prio_which_or_die(const char *text, const char *caption)
{
    return
        explain_parse_bits_or_die
        (
            text,
            table,
            SIZEOF(table),
            caption
        );
}


/* vim: set ts=8 sw=4 et : */
