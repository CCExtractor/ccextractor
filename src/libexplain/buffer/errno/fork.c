/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/fork.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_fork_system_call(explain_string_buffer_t *sb,
    int errnum)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "fork()");
}


void
explain_buffer_errno_fork_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/fork.html
     */
    switch (errnum)
    {
    case EAGAIN:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain fork(2) errors,
             * when no more specific cause can be determined.
             */
            i18n("the system lacked the necessary resources to create "
            "another process; or, the system-imposed limit on the total "
            "number of processes under execution system-wide would be "
            "exceeded; or, the system-imposed limit on the total number "
            "of processes under execution by a single user {CHILD_MAX} "
            "would be exceeded")
        );

        /*
         *  FIXME: distinguish the 4 cases
         *
         *  sysconf(_SC_CHILD_MAX)
         *      The max number of simultaneous processes per user ID.  Must
         *      not be less than _POSIX_CHILD_MAX (25).
         *
         *  getrlimit(RLIMIT_NPROC)
         *      The maximum number of processes (or, more precisely on
         *      Linux, threads) that can be created for the real user ID of
         *      the calling process.  Upon encountering this limit, fork(2)
         *      fails with the error EAGAIN.
         */
#ifdef HAVE_SYS_CAPABILITY_H
        if (explain_option_dialect_specific())
        {
            explain_string_buffer_puts(sb, ", ");
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: This message is used when explaining
                 * the capabilities required to exceed system limits
                 * on the number of processes a user may execute
                 * simultaniously.
                 */
                i18n("to exceed the limit on the number of processes, "
                "the process must have either the CAP_SYS_ADMIN or the "
                "CAP_SYS_RESOURCE capability")
            );
        }
#endif
        break;

    case ENOMEM:
        /*
         * The Open Group Base Specifications Issue 6
         * IEEE Std 1003.1, 2004 Edition
         *
         * fork() Rationale states
         *    "The [ENOMEM] error value is reserved for those
         *    implementations that detect and distinguish such a
         *    condition. This condition occurs when an implementation
         *    detects that there is not enough memory to create the
         *    process. This is intended to be returned when [EAGAIN] is
         *    inappropriate because there can never be enough memory
         *    (either primary or secondary storage) to perform the
         *    operation. Since fork() duplicates an existing process,
         *    this must be a condition where there is sufficient memory
         *    for one such process, but not for two. Many historical
         *    implementations actually return [ENOMEM] due to temporary
         *    lack of memory, a case that is not generally distinct from
         *    [EAGAIN] from the perspective of a conforming application.
         *
         *    "Part of the reason for including the optional error
         *    [ENOMEM] is because the SVID specifies it and it should
         *    be reserved for the error condition specified there. The
         *    condition is not applicable on many implementations."
         */
        explain_buffer_enomem_kernel(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_fork(explain_string_buffer_t *sb, int errnum)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_fork_system_call(&exp.system_call_sb, errnum);
    explain_buffer_errno_fork_explanation(&exp.explanation_sb, errnum, "fork");
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
