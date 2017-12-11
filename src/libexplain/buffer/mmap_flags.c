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

#include <libexplain/ac/sys/mman.h>

#include <libexplain/buffer/mmap_flags.h>
#include <libexplain/parse_bits.h>


static const struct explain_parse_bits_table_t table[] =
{
#ifdef MAP_SHARED
    { "MAP_SHARED", MAP_SHARED },
#endif
#ifdef MAP_PRIVATE
    { "MAP_PRIVATE", MAP_PRIVATE },
#endif
#ifdef MAP_FIXED
    { "MAP_FIXED", MAP_FIXED },
#endif
#ifdef MAP_FILE
    { "MAP_FILE", MAP_FILE },
#endif
#ifdef MAP_ANONYMOUS
    { "MAP_ANONYMOUS", MAP_ANONYMOUS },
#endif
#ifdef MAP_ANON
    { "MAP_ANON", MAP_ANON },
#endif
#ifdef MAP_32BIT
    { "MAP_32BIT", MAP_32BIT },
#endif
#ifdef MAP_GROWSDOWN
    { "MAP_GROWSDOWN", MAP_GROWSDOWN },
#endif
#ifdef MAP_DENYWRITE
    { "MAP_DENYWRITE", MAP_DENYWRITE },
#endif
#ifdef MAP_EXECUTABLE
    { "MAP_EXECUTABLE", MAP_EXECUTABLE },
#endif
#ifdef MAP_LOCKED
    { "MAP_LOCKED", MAP_LOCKED },
#endif
#ifdef MAP_NORESERVE
    { "MAP_NORESERVE", MAP_NORESERVE },
#endif
#ifdef MAP_POPULATE
    { "MAP_POPULATE", MAP_POPULATE },
#endif
#ifdef MAP_NONBLOCK
    { "MAP_NONBLOCK", MAP_NONBLOCK },
#endif
#ifdef MAP_STACK
    { "MAP_STACK", MAP_STACK },
#endif
};


void
explain_buffer_mmap_flags(explain_string_buffer_t *sb, int flags)
{
    explain_parse_bits_print(sb, flags, table, SIZEOF(table));
}


int
explain_parse_mmap_flags_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
