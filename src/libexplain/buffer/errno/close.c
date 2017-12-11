/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/eio.h>
#include <libexplain/buffer/errno/close.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/explanation.h>
#include <libexplain/string_buffer.h>


static void
explain_buffer_errno_close_system_call(explain_string_buffer_t *sb,
    int errnum, int fildes)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "close(fildes = %d", fildes);
    explain_buffer_fildes_to_pathname(sb, fildes);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_errno_close_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, int fildes)
{
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EINTR:
        explain_buffer_eintr(sb, syscall_name);
        break;

    case EIO:
        explain_buffer_eio_fildes(sb, fildes);
        break;

    case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
#endif
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This error message is used when trying to close
             * a non-blocking file descriptor that is stillactive.
             */
            i18n("the O_NONBLOCK flag was specified, and an operation "
                "has yet to complete")
        );
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, syscall_name);
        break;
    }

    /*
     * See of the file is still open, and tell the user if it is.
     */
    if (fcntl(fildes, F_GETFL) >= 0)
    {
        explain_string_buffer_puts(sb->footnotes, "; ");
        explain_buffer_gettext
        (
            sb->footnotes,
            /*
             * xgettext:  This error message is used when a close(2)
             * system call fails and the file descriptor remains open.
             */
            i18n("note that the file descriptor is still open")
        );
    }
}


void
explain_buffer_errno_close(explain_string_buffer_t *sb, int errnum,
    int fildes)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_close_system_call
    (
        &exp.system_call_sb,
        errnum,
        fildes
    );
    explain_buffer_errno_close_explanation
    (
        &exp.explanation_sb,
        errnum,
        "close",
        fildes
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
