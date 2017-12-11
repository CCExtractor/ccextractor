/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/sys/resource.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/poll.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pollfd.h>
#include <libexplain/explanation.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_poll_system_call(explain_string_buffer_t *sb, int errnum,
    struct pollfd *data, int data_size, int timeout)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "poll(data = ");
    explain_buffer_pollfd_array(sb, data, data_size, 0);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_int(sb, data_size);
    explain_string_buffer_puts(sb, ", timeout = ");
    explain_buffer_int(sb, timeout);
    explain_string_buffer_putc(sb, ')');
}


static int
ebadf(int fildes)
{
    int             flags;

    return (fcntl(fildes, F_GETFD, &flags) < 0 && errno == EBADF);
}


void
explain_buffer_errno_poll_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, struct pollfd *data, int data_size, int timeout)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/poll.html
     */
    (void)data;
    (void)timeout;
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        break;

    case EINVAL:
        {
            struct rlimit   lim;

            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: This error message is used to explain an EINVAL
                 * error reported by the poll(2) system call, in the case where
                 * the data_size value exceeds the RLIMIT_NOFILE value.
                 */
                i18n("the data_size value exceeds the RLIMIT_NOFILE value")
            );
            if (getrlimit(RLIMIT_NOFILE, &lim) >= 0)
            {
                explain_string_buffer_printf
                (
                    sb,
                    " (%d >= %d)",
                    data_size,
                    (int)lim.rlim_cur
                );
            }
        }
        break;

    case ENOMEM:
        explain_buffer_enomem_kernel(sb);
        break;

#ifdef HAVE_POLL_H
    case EBADF:
        if
        (
            data_size > 0
        &&
            !explain_is_efault_pointer(data, data_size * sizeof(struct pollfd))
        )
        {
            int             j;

            for (j = 0; j < data_size; ++j)
            {
                if (ebadf(data[j].fd))
                {
                    char            caption[30];

                    snprintf(caption, sizeof(caption), "data[%d].fd", j);
                    explain_buffer_ebadf(sb, data[j].fd, caption);
                    return;
                }
            }
        }
        /* fall through... */
#endif

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }
}


void
explain_buffer_errno_poll(explain_string_buffer_t *sb, int errnum,
    struct pollfd *data, int data_size, int timeout)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_poll_system_call(&exp.system_call_sb, errnum, data,
        data_size, timeout);
    explain_buffer_errno_poll_explanation(&exp.explanation_sb, errnum, "poll",
        data, data_size, timeout);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
