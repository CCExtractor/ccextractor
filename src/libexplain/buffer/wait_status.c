/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/signal.h>
#include <libexplain/ac/sys/wait.h>

#include <libexplain/buffer/wait_status.h>
#include <libexplain/buffer/strsignal.h>


void
explain_buffer_wait_status(explain_string_buffer_t *sb, int status)
{
    if (WIFEXITED(status))
    {
        char            name[30];
        int             es;

        es = WEXITSTATUS(status);
        if (es == EXIT_SUCCESS)
            snprintf(name, sizeof(name), "EXIT_SUCCESS (%d)", es);
        else if (es == EXIT_FAILURE)
            snprintf(name, sizeof(name), "EXIT_FAILURE (%d)", es);
        else
            snprintf(name, sizeof(name), "%d", es);
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext:  This message is used when a child process
             * terminates normally.  The exist status is reported.
             */
            i18n("the child process terminated with exit status %s"),
            name
        );
    }
    else if (WIFSIGNALED(status))
    {
        char            name[100];
        explain_string_buffer_t name_sb;

        explain_string_buffer_init(&name_sb, name, sizeof(name));
        explain_buffer_strsignal(&name_sb, WTERMSIG(status));
        if (WCOREDUMP(status))
        {
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used when a child process
                 * is terminated by the delivery of an uncaught signal.
                 * This also resulted in a core dump.
                 */
                i18n("the child process was terminated by the %s signal, "
                    "core dumped"),
                name
            );
        }
        else
        {
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used when a child process
                 * is terminated by the delivery of an uncaught signal.
                 */
                i18n("the child process was terminated by the %s signal"),
                name
            );
        }
    }
    else if (WIFSTOPPED(status))
    {
        char            name[100];
        explain_string_buffer_t name_sb;

        explain_string_buffer_init(&name_sb, name, sizeof(name));
        explain_buffer_strsignal(&name_sb, WSTOPSIG(status));
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used when a child process is
             * stopped by delivery of a signal.  The process is still
             * there, is is stopped, not terminated.
             */
            i18n("the child process was stopped by delivery of the %s signal"),
            name
        );
    }
    else if (WIFCONTINUED(status))
    {
        char            name[100];
        explain_string_buffer_t name_sb;

        explain_string_buffer_init(&name_sb, name, sizeof(name));
        explain_buffer_strsignal(&name_sb, SIGCONT);
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext:  This message is used when a child process is
             * resumed by delivering a signal (SIGCONT).
             */
            i18n("the child process was resumed by delivery of the %s signal"),
            name
        );
    }
    else
    {
        explain_string_buffer_printf
        (
            sb,
            /* This is supposed to be impossible, not xgettext msg needed. */
            "the child process terminated with an indecipherable status 0x%04X",
            status
        );
    }
}


/* vim: set ts=8 sw=4 et : */
