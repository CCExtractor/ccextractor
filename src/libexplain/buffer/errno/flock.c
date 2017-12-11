/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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
#include <libexplain/ac/sys/file.h>

#include <libexplain/buffer/ebadf.h>
#include <libexplain/buffer/eintr.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/ewouldblock.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/flock.h>
#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/explanation.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


/*
 * On Solaris, even though flock(2) says these defines are available in
 * <sys/file.h>, the defines do not appear to be actually there.
 */
static const explain_parse_bits_table_t table[] =
{
#ifdef LOCK_SH
    { "LOCK_SH", LOCK_SH },
#endif
#ifdef LOCK_EX
    { "LOCK_EX", LOCK_EX },
#endif
#ifdef LOCK_UN
    { "LOCK_UN", LOCK_UN },
#endif
#ifdef LOCK_NB
    { "LOCK_NB", LOCK_NB },
#endif
};


int
explain_flock_command_parse_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, sizeof(table), caption);
}


static void
explain_buffer_flock_command(explain_string_buffer_t *sb, int command)
{
    explain_parse_bits_print(sb, command, table, SIZEOF(table));
}


static void
explain_buffer_errno_flock_system_call(explain_string_buffer_t *sb, int errnum,
    int fildes, int command)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "flock(fildes = ");
    explain_buffer_fildes(sb, fildes);
    explain_string_buffer_puts(sb, ", command = ");
    explain_buffer_flock_command(sb, command);
    explain_string_buffer_putc(sb, ')');
}


static void
explain_buffer_errno_flock_explanation(explain_string_buffer_t *sb, int errnum,
    int fildes, int command)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/flock.html
     */
    (void)command;
    switch (errnum)
    {
    case EBADF:
        explain_buffer_ebadf(sb, fildes, "fildes");
        break;

    case EINTR:
        explain_buffer_eintr(sb, "flock");
        break;

    case EINVAL:
        explain_buffer_einval_vague(sb, "command");
        break;

    case ENOLCK:
        explain_buffer_enomem_kernel(sb);
        break;

    case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
#endif
        explain_buffer_ewouldblock(sb, "flock");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "flock");
        break;
    }
}


void
explain_buffer_errno_flock(explain_string_buffer_t *sb, int errnum, int fildes,
    int command)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_flock_system_call(&exp.system_call_sb, errnum, fildes,
        command);
    explain_buffer_errno_flock_explanation(&exp.explanation_sb, errnum, fildes,
        command);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
