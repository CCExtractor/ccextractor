/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/ac/sys/ptrace.h>

#include <libexplain/buffer/ptrace_options.h>
#include <libexplain/parse_bits.h>


static const explain_parse_bits_table_t table[] =
{
    /* On linux these are enums, not #defines */
#if defined(PTRACE_O_TRACESYSGOOD) || \
        (defined(__linux__) && defined(PT_SETOPTIONS))
    { "PTRACE_O_TRACESYSGOOD", PTRACE_O_TRACESYSGOOD },
#endif
#if defined(PTRACE_O_TRACEFORK) || \
        (defined(__linux__) && defined(PT_SETOPTIONS))
    { "PTRACE_O_TRACEFORK", PTRACE_O_TRACEFORK },
#endif
#if defined(PTRACE_O_TRACEVFORK) || \
        (defined(__linux__) && defined(PT_SETOPTIONS))
    { "PTRACE_O_TRACEVFORK", PTRACE_O_TRACEVFORK },
#endif
#if defined(PTRACE_O_TRACECLONE) || \
        (defined(__linux__) && defined(PT_SETOPTIONS))
    { "PTRACE_O_TRACECLONE", PTRACE_O_TRACECLONE },
#endif
#if defined(PTRACE_O_TRACEEXEC) || \
        (defined(__linux__) && defined(PT_SETOPTIONS))
    { "PTRACE_O_TRACEEXEC", PTRACE_O_TRACEEXEC },
#endif
#if defined(PTRACE_O_TRACEVFORKDONE) || \
        (defined(__linux__) && defined(PT_SETOPTIONS))
    { "PTRACE_O_TRACEVFORKDONE", PTRACE_O_TRACEVFORKDONE },
#endif
#if defined(PTRACE_O_TRACEEXIT) || \
        (defined(__linux__) && defined(PT_SETOPTIONS))
    { "PTRACE_O_TRACEEXIT", PTRACE_O_TRACEEXIT },
#endif
#if defined(PTRACE_O_MASK) || (defined(__linux__) && defined(PT_SETOPTIONS))
    { "PTRACE_O_MASK", PTRACE_O_MASK },
#endif
};


void
explain_buffer_ptrace_options(explain_string_buffer_t *sb, long value)
{
    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


long
explain_parse_ptrace_options_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
