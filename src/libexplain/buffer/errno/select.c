/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/select.h>
#include <libexplain/buffer/fd_set.h>
#include <libexplain/buffer/timeval.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_select_system_call(explain_string_buffer_t *sb,
    int errnum, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
    struct timeval *timeout)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "select(nfds = %d", nfds);
    explain_string_buffer_puts(sb, ", readfds = ");
    explain_buffer_fd_set(sb, nfds, readfds);
    explain_string_buffer_puts(sb, ", writefds = ");
    explain_buffer_fd_set(sb, nfds, writefds);
    explain_string_buffer_puts(sb, ", exceptfds = ");
    explain_buffer_fd_set(sb, nfds, exceptfds);
    explain_string_buffer_puts(sb, ", timeout = ");
    explain_buffer_timeval(sb, timeout);
    explain_string_buffer_putc(sb, ')');
}


static int
file_descriptor_is_open(int fildes)
{
    int             err;

    err = fcntl(fildes, F_GETFL);
    return (err >= 0);
}


static void
explain_buffer_errno_select_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int nfds, fd_set *readfds,
    fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/select.html
     */
    switch (errnum)
    {
    case EBADF:
        {
            int             fildes;

            for (fildes = 0; fildes < nfds; ++fildes)
            {
                if
                (
                    (
                        (readfds && FD_ISSET(fildes, readfds))
                    ||
                        (writefds && FD_ISSET(fildes, writefds))
                    ||
                        (exceptfds && FD_ISSET(fildes, exceptfds))
                    )
                &&
                    !file_descriptor_is_open(fildes)
                )
                    break;
            }
            if (fildes < nfds)
            {
                char            caption[40];

                snprintf(caption, sizeof(caption), "fildes %d", fildes);
                explain_buffer_ebadf(sb, fildes, caption);
            }
            else
            {
                explain_string_buffer_puts
                (
                    sb,
                    "an invalid file descriptor was given in one of the sets; "
                    "perhaps a file descriptor that was already closed, or one "
                    "on which an error has occurred"
                );
            }
        }
        break;

    case EFAULT:
        if (readfds && explain_is_efault_pointer(readfds, sizeof(*readfds)))
        {
            explain_buffer_efault(sb, "readfds");
            break;
        }
        if
        (
            writefds
        &&
            explain_is_efault_pointer(writefds, sizeof(*writefds))
        )
        {
            explain_buffer_efault(sb, "writefds");
            break;
        }
        if
        (
            exceptfds
        &&
            explain_is_efault_pointer(exceptfds, sizeof(*exceptfds))
        )
        {
            explain_buffer_efault(sb, "exceptfds");
            break;
        }
        if (timeout && explain_is_efault_pointer(timeout, sizeof(*timeout)))
        {
            explain_buffer_efault(sb, "timeout");
            break;
        }
        break;

    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        break;

    case EINVAL:
        if (nfds < 0)
        {
            explain_string_buffer_puts(sb, "nfds is negative");
            break;
        }
        if ((unsigned)nfds > (unsigned)FD_SETSIZE)
        {
            explain_string_buffer_puts
            (
                sb,
                "nfds is greater than FD_SETSIZE"
            );
            if (explain_option_dialect_specific())
                explain_string_buffer_printf(sb, " (%d)", FD_SETSIZE);
            break;
        }
        if (timeout && (timeout->tv_sec < 0 || timeout->tv_usec < 0))
        {
            explain_string_buffer_puts(sb, "timeout is invalid");
            break;
        }
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_select(explain_string_buffer_t *sb, int errnum,
    int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
    struct timeval *timeout)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_select_system_call
    (
        &exp.system_call_sb,
        errnum,
        nfds,
        readfds,
        writefds,
        exceptfds,
        timeout
    );
    explain_buffer_errno_select_explanation
    (
        &exp.explanation_sb,
        errnum,
        "select",
        nfds,
        readfds,
        writefds,
        exceptfds,
        timeout
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
