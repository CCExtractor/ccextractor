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

#include <libexplain/ac/sys/resource.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/eagain.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/rlimit.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/option.h>


static int
explain_buffer_rlimit_nproc(explain_string_buffer_t *sb)
{
    struct rlimit   rlim;

    if (getrlimit(RLIMIT_NPROC, &rlim) >= 0)
    {
        explain_string_buffer_puts(sb, " (RLIMIT_NPROC: ");
        explain_buffer_rlimit(sb, &rlim);
        explain_string_buffer_putc(sb, ')');
        return 1;
    }
    return 0;
}


void
explain_buffer_eagain_setuid(explain_string_buffer_t *sb, const char *caption)
{
    explain_string_buffer_t uid_sb;
    char            buid[100];

    explain_string_buffer_init(&uid_sb, buid, sizeof(buid));
    explain_buffer_uid(&uid_sb, getuid());

    explain_string_buffer_printf_gettext
    (
        sb,
        i18n
        (
            /*
             * xgettext: This error message is used to explain an EAGAIN
             * error reported by the setuid(2) system call (or similar) in the
             * case where the uid does not match the real user ID and 'uid'
             * would take the process count of the new real user ID
             * 'uid' over its RLIMIT_NPROC resource limit.
             *
             * %1$s => the name of the offending system call argument
             * %2$s => real user ID and user name of current process
             */
            "the %1$s argument does not match the process's real user ID "
            "(%2$s), and would take the new real user ID over its "
            "maximum number of processes/threads that can be created"
        ),
        caption,
        buid
    );
    if (explain_option_dialect_specific())
        explain_buffer_rlimit_nproc(sb);
}


/* vim: set ts=8 sw=4 et : */
