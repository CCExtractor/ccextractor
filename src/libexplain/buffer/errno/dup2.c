/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013, 2014 Peter Miller
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
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/check_fildes_range.h>
#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/emfile.h>
#include <libexplain/buffer/errno/dup2.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/explanation.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_dup2_system_call(explain_string_buffer_t *sb,
    int errnum, int oldfd, int newfd)
{
    (void)errnum;
    explain_string_buffer_printf(sb, "dup2(oldfd = %d", oldfd);
    explain_buffer_fildes_to_pathname(sb, oldfd);
    explain_string_buffer_printf(sb, ", newfd = %d", newfd);
    explain_buffer_fildes_to_pathname(sb, newfd);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_dup2_explanation(explain_string_buffer_t *sb,
    int errnum, int oldfd, int newfd)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/dup2.html
     */
    switch (errnum)
    {
    case EBADF:
        if (fcntl(oldfd, F_GETFL) < 0)
        {
            /* oldfd is not an open file */
            explain_buffer_ebadf(sb, oldfd, "oldfd");
            return;
        }
        if (explain_buffer_check_fildes_range(sb, newfd, "newfd") >= 0)
        {
            /* newfd is out of range */
            return;
        }
        explain_string_buffer_puts
        (
            sb,
            /* FIXME: i18n */
            "the oldfd argument isn't an open file descriptor; or, "
            "the newf argument is outside the allowed range for file "
            "descriptors"
        );
        break;

    case EINVAL:
        /* not on Linux */
        if (explain_buffer_check_fildes_range(sb, newfd, "newfd") >= 0)
        {
            /* newfd is out of range */
            return;
        }
        explain_buffer_einval_vague(sb, "data");
        break;

    case EBUSY:
        explain_string_buffer_puts
        (
            sb,
            "a race condition was detected between open(2) and dup(2)"
        );
        break;

    case EINTR:
        explain_buffer_eintr(sb, "dup2");
        break;

    case EMFILE:
        if (explain_buffer_check_fildes_range(sb, newfd, "newfd"))
            explain_buffer_emfile(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "dup2");
        break;
    }
    if (fcntl(newfd, F_GETFL) >= 0)
    {
        explain_string_buffer_puts(sb->footnotes, "; ");
        explain_buffer_gettext
        (
            sb->footnotes,
            /*
             * xgettext:  This error message is used when dup2(2) system
             * call fails, the destination file descriptor may or may
             * not be closed.
             */
            i18n("note that any errors that would have been reported by "
            "close(newfd) are lost, a careful programmer will not use "
            "dup2() without closing newfd first")
        );
    }
}


void
explain_buffer_errno_dup2(explain_string_buffer_t *sb, int errnum,
    int oldfd, int newfd)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_dup2_system_call
    (
        &exp.system_call_sb,
        errnum,
        oldfd,
        newfd
    );
    explain_buffer_errno_dup2_explanation
    (
        &exp.explanation_sb,
        errnum,
        oldfd,
        newfd
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
