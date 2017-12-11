/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/sys/resource.h>

#include <libexplain/buffer/resource.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef RLIMIT_CPU
    { "RLIMIT_CPU", RLIMIT_CPU },
#endif
#ifdef RLIMIT_FSIZE
    { "RLIMIT_FSIZE", RLIMIT_FSIZE },
#endif
#ifdef RLIMIT_DATA
    { "RLIMIT_DATA", RLIMIT_DATA },
#endif
#ifdef RLIMIT_STACK
    { "RLIMIT_STACK", RLIMIT_STACK },
#endif
#ifdef RLIMIT_CORE
    { "RLIMIT_CORE", RLIMIT_CORE },
#endif
#ifdef __RLIMIT_RSS
    { "__RLIMIT_RSS", __RLIMIT_RSS },
#endif
#ifdef RLIMIT_RSS
    { "RLIMIT_RSS", RLIMIT_RSS },
#endif
#ifdef __RLIMIT_NPROC
    { "__RLIMIT_NPROC", __RLIMIT_NPROC },
#endif
#ifdef RLIMIT_NPROC
    { "RLIMIT_NPROC", RLIMIT_NPROC },
#endif
#ifdef RLIMIT_NOFILE
    { "RLIMIT_NOFILE", RLIMIT_NOFILE },
#endif
#ifdef RLIMIT_OFILE
    { "RLIMIT_OFILE", RLIMIT_OFILE },
#endif
#ifdef __RLIMIT_MEMLOCK
    { "__RLIMIT_MEMLOCK", __RLIMIT_MEMLOCK },
#endif
#ifdef RLIMIT_MEMLOCK
    { "RLIMIT_MEMLOCK", RLIMIT_MEMLOCK },
#endif
#ifdef RLIMIT_AS
    { "RLIMIT_AS", RLIMIT_AS },
#endif
#ifdef __RLIMIT_LOCKS
    { "__RLIMIT_LOCKS", __RLIMIT_LOCKS },
#endif
#ifdef RLIMIT_LOCKS
    { "RLIMIT_LOCKS", RLIMIT_LOCKS },
#endif
#ifdef __RLIMIT_SIGPENDING
    { "__RLIMIT_SIGPENDING", __RLIMIT_SIGPENDING },
#endif
#ifdef RLIMIT_SIGPENDING
    { "RLIMIT_SIGPENDING", RLIMIT_SIGPENDING },
#endif
#ifdef __RLIMIT_MSGQUEUE
    { "__RLIMIT_MSGQUEUE", __RLIMIT_MSGQUEUE },
#endif
#ifdef RLIMIT_MSGQUEUE
    { "RLIMIT_MSGQUEUE", RLIMIT_MSGQUEUE },
#endif
#ifdef __RLIMIT_NICE
    { "__RLIMIT_NICE", __RLIMIT_NICE },
#endif
#ifdef RLIMIT_NICE
    { "RLIMIT_NICE", RLIMIT_NICE },
#endif
#ifdef __RLIMIT_RTPRIO
    { "__RLIMIT_RTPRIO", __RLIMIT_RTPRIO },
#endif
#ifdef RLIMIT_RTPRIO
    { "RLIMIT_RTPRIO", RLIMIT_RTPRIO },
#endif
#ifdef __RLIMIT_NLIMITS
    { "__RLIMIT_NLIMITS", __RLIMIT_NLIMITS },
#endif
#ifdef __RLIM_NLIMITS
    { "__RLIM_NLIMITS", __RLIM_NLIMITS },
#endif
#ifdef RLIMIT_NLIMITS
    { "RLIMIT_NLIMITS", RLIMIT_NLIMITS },
#endif
#ifdef RLIM_NLIMITS
    { "RLIM_NLIMITS", RLIM_NLIMITS },
#endif
};


void
explain_buffer_resource(explain_string_buffer_t *sb, int resource)
{
    const explain_parse_bits_table_t *tp;

    tp = explain_parse_bits_find_by_value(resource, table, SIZEOF(table));
    if (tp)
        explain_string_buffer_puts(sb, tp->name);
    else
        explain_string_buffer_printf(sb, "%d", resource);
}


int
explain_parse_resource_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
