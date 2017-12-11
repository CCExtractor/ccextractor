/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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

#include <libexplain/ac/signal.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/signal.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


static explain_parse_bits_table_t table[] =
{
#ifdef SIGHUP
    { "SIGHUP", SIGHUP },
#endif
#ifdef SIGINT
    { "SIGINT", SIGINT },
#endif
#ifdef SIGQUIT
    { "SIGQUIT", SIGQUIT },
#endif
#ifdef SIGILL
    { "SIGILL", SIGILL },
#endif
#ifdef SIGABRT
    { "SIGABRT", SIGABRT },
#endif
#ifdef SIGFPE
    { "SIGFPE", SIGFPE },
#endif
#ifdef SIGKILL
    { "SIGKILL", SIGKILL },
#endif
#ifdef SIGSEGV
    { "SIGSEGV", SIGSEGV },
#endif
#ifdef SIGPIPE
    { "SIGPIPE", SIGPIPE },
#endif
#ifdef SIGALRM
    { "SIGALRM", SIGALRM },
#endif
#ifdef SIGTERM
    { "SIGTERM", SIGTERM },
#endif
#ifdef SIGUSR1
    { "SIGUSR1", SIGUSR1 },
#endif
#ifdef SIGUSR2
    { "SIGUSR2", SIGUSR2 },
#endif
#ifdef SIGCHLD
    { "SIGCHLD", SIGCHLD },
#endif
#ifdef SIGCONT
    { "SIGCONT", SIGCONT },
#endif
#ifdef SIGSTOP
    { "SIGSTOP", SIGSTOP },
#endif
#ifdef SIGTSTP
    { "SIGTSTP", SIGTSTP },
#endif
#ifdef SIGTTIN
    { "SIGTTIN", SIGTTIN },
#endif
#ifdef SIGTTOU
    { "SIGTTOU", SIGTTOU },
#endif

#ifdef SIGBUS
    { "SIGBUS", SIGBUS },
#endif
#ifdef SIGPOLL
    { "SIGPOLL", SIGPOLL },
#endif
#ifdef SIGPROF
    { "SIGPROF", SIGPROF },
#endif
#ifdef SIGSYS
    { "SIGSYS", SIGSYS },
#endif
#ifdef SIGTRAP
    { "SIGTRAP", SIGTRAP },
#endif
#ifdef SIGURG
    { "SIGURG", SIGURG },
#endif
#ifdef SIGVTALRM
    { "SIGVTALRM", SIGVTALRM },
#endif
#ifdef SIGXCPU
    { "SIGXCPU", SIGXCPU },
#endif
#ifdef SIGXFSZ
    { "SIGXFSZ", SIGXFSZ },
#endif

#ifdef SIGIOT
    { "SIGIOT", SIGIOT }, /* synonym for SIGABRT, must be in table after */
#endif
#ifdef SIGEMT
    { "SIGEMT", SIGEMT },
#endif
#ifdef SIGSTKFLT
    { "SIGSTKFLT", SIGSTKFLT },
#endif
#ifdef SIGIO
    { "SIGIO", SIGIO },
#endif
#ifdef SIGCLD
    { "SIGCLD", SIGCLD }, /* synonym for SIGCHLD, must be in table after */
#endif
#ifdef SIGPWR
    { "SIGPWR", SIGPWR },
#endif
#ifdef SIGLOST
    { "SIGLOST", SIGLOST }, /* synonym for SIGIO, must be after in table */
#endif
#ifdef SIGWINCH
    { "SIGWINCH", SIGWINCH },
#endif
#ifdef SIGUNUSED
    { "SIGUNUSED", SIGUNUSED }, /* may be synonym for SIGSYS, so table after */
#endif

#ifdef _NSIG
    { "_NSIG", _NSIG },
#endif

#if !defined(_SIGRTMIN) && !defined(__SIGRTMIN)
    /* sometimes this is a non-constant macro: sysconf(_SC_SIGRTMIN) */
    { "SIGRTMIN", SIGRTMIN },
#endif
#ifdef __SIGRTMIN
    { "SIGRTMIN", __SIGRTMIN },
    { "__SIGRTMIN", __SIGRTMIN },
#endif
#ifdef _SIGRTMIN
    { "SIGRTMIN", _SIGRTMIN },
    { "_SIGRTMIN", _SIGRTMIN },
#endif

#if !defined(_SIGRTMAX) && !defined(__SIGRTMAX)
    /* sometimes this is a non-constant macro: sysconf(_SC_SIGRTMAX) */
    { "SIGRTMAX", SIGRTMAX },
#endif
#ifdef _SIGRTMAX
    { "SIGRTMAX", _SIGRTMAX },
    { "_SIGRTMAX", _SIGRTMAX },
#endif
#ifdef __SIGRTMAX
    { "SIGRTMAX", __SIGRTMAX },
    { "__SIGRTMAX", __SIGRTMAX },
#endif
};


static void
fix_real_time_values(void)
{
#if defined(__SIGRTMIN) || defined(_SIGRTMIN)
    explain_parse_bits_table_t *tp;
    static int      done;

    if (done)
        return;
    done = 1;

    /*
     * This is needed if SIGRTMIN et al are actually macros calling
     * sysconf(_SC_SIGRTMIN), etc.
     */
    for (tp = table; tp < ENDOF(table); ++tp)
    {
        if (0 == strcmp(tp->name, "SIGRTMIN"))
            tp->value = SIGRTMIN;
        if (0 == strcmp(tp->name, "SIGRTMAX"))
            tp->value = SIGRTMAX;
    }
#endif
}


void
explain_buffer_signal(explain_string_buffer_t *sb, int signum)
{
    const explain_parse_bits_table_t *tp;

    fix_real_time_values();
    tp = explain_parse_bits_find_by_value(signum, table, SIZEOF(table));
    if (tp)
    {
        explain_string_buffer_puts(sb, tp->name);
        return;
    }

#if defined(SIGRTMIN) && defined(SIGRTMAX)
    {
        int             rtmin;
        int             rtmax;

        /*
         * Don't worry about the SIGRTMIN+0 case being ugly, because the
         * table already dealt with that case.
         */
        rtmin = SIGRTMIN;
        rtmax = SIGRTMAX;
        if (rtmin <= signum && signum <= rtmax)
        {
            explain_string_buffer_printf
            (
                sb,
                "SIGRTMIN + %d",
                signum - rtmin
            );
            return;
        }
    }
#endif

    explain_string_buffer_printf(sb, "%d", signum);
}


int
explain_signal_parse_or_die(const char *text, const char *caption)
{
    fix_real_time_values();
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
