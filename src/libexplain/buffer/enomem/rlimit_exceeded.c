/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013, 2014 Peter Miller
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

#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/rlimit.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/option.h>


/*
 * return 0 if print nothing
 * return 1 if print anything
 */
int
explain_buffer_enomem_rlimit_exceeded(explain_string_buffer_t *sb, size_t size)
{
    struct rlimit rlim;
    struct rusage ru;

    /*
     * The resource argument must be one of:
     *
     * RLIMIT_AS
     *     The maximum size of the process's virtual memory
     *     (address space) in bytes.  This limit affects calls to
     *     brk(2), mmap(2) and mremap(2), which fail with the error
     *     ENOMEM upon exceeding this limit.  Also automatic stack
     *     expansion will fail (and generate a SIGSEGV that kills
     *     the process if no alternate stack has been made available
     *     via sigaltstack(2)).  Since the value is a long, on
     *     machines with a 32-bit long ei
     *
     * RLIMIT_DATA
     *     The maximum size of the process's data segment (initialized
     *     data, uninitialized data, and heap).  This limit affects
     *     calls to brk(2) and sbrk(2), which fail with the error ENOMEM
     *     upon encountering the soft limit of this resource.  Compare
     *     with rusage::ru_idrsss (integral unshared data size)?!?
     */
    if (getrlimit(RLIMIT_AS, &rlim) <= 0)
        return 0;
    if (rlim.rlim_cur == RLIM_INFINITY)
    {
        explain_buffer_enomem_user(sb, rlim.rlim_cur);
        return 1;
    }

    if (getrusage(RUSAGE_SELF, &ru) < 0)
        return 0;

    /*  signed long  < rlim_t == unigned longs */
    if (ru.ru_maxrss < (long)rlim.rlim_cur)
        return 0;

    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext:  This message is used to explain an
         * ENOMEM error reported by a system call (e.g.
         * mmap or shmat), in the case where the virtual
         * memory size of the process would have been
         * exceeded.  The relevant getrlimit values will be
         * printed separately.
         */
        i18n("the virtual memory size limit of the process "
            "would have been exceeded")
    );

    if (explain_option_dialect_specific())
    {
        explain_string_buffer_puts(sb, " (");
        explain_buffer_long(sb, ru.ru_maxrss);
        if (size == 0)
        {
            explain_string_buffer_puts(sb, " + ");
            explain_buffer_size_t(sb, size);
        }
        explain_string_buffer_puts(sb, " >= ");
        explain_buffer_rlim_t(sb, rlim.rlim_cur);
        explain_string_buffer_putc(sb, ')');
    }

    if (RLIM_INFINITY == rlim.rlim_max || ru.ru_maxrss < (long)rlim.rlim_max)
    {
        explain_string_buffer_puts(sb->footnotes, "; ");
        /* FIXME: i18n */
        explain_string_buffer_puts
        (
            sb->footnotes,
            /*
             * xgettext:  This supplemntary explanation is given when the user
             * can expand one of the limits to possdibly satisfy the memory
             * request.
             */
            i18n("you have some head room in the resource allocation, it may "
            "help to run the command " "\"ulimit -m hard\" and retry")
        );
    }
    return 1;
}


/* vim: set ts=8 sw=4 et : */
