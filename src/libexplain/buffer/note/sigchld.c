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

#include <libexplain/ac/signal.h>

#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/note/sigchld.h>


static int
sigchld_ignored(void)
{
    struct sigaction oldact;
    if (sigaction(SIGCHLD, 0, &oldact) < 0)
        return -1;
    if (oldact.sa_flags & SA_NOCLDWAIT)
        return 1;
    if (oldact.sa_flags & SA_SIGINFO)
        return 0;
    return (oldact.sa_handler == SIG_IGN);
}


void
explain_buffer_note_sigchld(explain_string_buffer_t *sb)
{
    if (sigchld_ignored())
    {
        sb = sb->footnotes;
        explain_string_buffer_puts(sb, "; ");
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used by the wait*()
             * explanations to describe the relationship between SIGCHLD
             * and the wait*() functions.
             */
            i18n("the process is ignoring the SIGCHLD signal, this "
                "means that child processes that terminate will not "
                "persist until waited for")
        );
    }
}


/* vim: set ts=8 sw=4 et : */
