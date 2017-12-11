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

#include <libexplain/buffer/mmap_prot.h>
#include <libexplain/parse_bits.h>


static const struct explain_parse_bits_table_t table[] =
{
#ifdef PROT_READ
    { "PROT_READ", PROT_READ },
#endif
#ifdef PROT_WRITE
    { "PROT_WRITE", PROT_WRITE },
#endif
#ifdef PROT_EXEC
    { "PROT_EXEC", PROT_EXEC },
#endif
#ifdef PROT_NONE
    { "PROT_NONE", PROT_NONE },
#endif
#ifdef PROT_GROWSDOWN
    { "PROT_GROWSDOWN", PROT_GROWSDOWN },
#endif
};


void
explain_buffer_mmap_prot(explain_string_buffer_t *sb, int prot)
{
    explain_parse_bits_print(sb, prot, table, SIZEOF(table));
}


int
explain_parse_mmap_prot_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
