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

#include <libexplain/ac/fcntl.h>

#include <libexplain/buffer/flock.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/lseek_whence.h>
#include <libexplain/buffer/off_t.h>
#include <libexplain/buffer/pid_t_star.h>
#include <libexplain/parse_bits.h>
#include <libexplain/string_buffer.h>


static void
explain_buffer_flock_type(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "F_RDLCK", F_RDLCK },
        { "F_WRLCK", F_WRLCK },
        { "F_UNLCK", F_UNLCK },
#ifdef F_UNLKSYS
        { "F_UNLKSYS", F_UNLKSYS },
#endif
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_flock(explain_string_buffer_t *sb, const struct flock *flp)
{
    explain_string_buffer_puts(sb, "{ l_type = ");
    explain_buffer_flock_type(sb, flp->l_type);
    explain_string_buffer_puts(sb, ", l_whence = ");
    explain_buffer_lseek_whence(sb, flp->l_whence);
    explain_string_buffer_puts(sb, ", l_start = ");
    explain_buffer_off_t(sb, flp->l_whence);
    explain_string_buffer_puts(sb, ", l_len = ");
    explain_buffer_off_t(sb, flp->l_len);
#ifdef F_UNLKSYS
    if (flp->l_pid < 0)
    {
        explain_string_buffer_puts(sb, ", l_sysid = ");
        explain_buffer_pid_t(sb, flp->l_sysid);
    }
#endif
    explain_string_buffer_puts(sb, ", l_pid = ");
    if (flp->l_pid > 0)
        explain_buffer_pid_t(sb, flp->l_pid);
    else
        explain_buffer_int(sb, (int)flp->l_pid);
    explain_string_buffer_puts(sb, " }");
}


#ifdef F_GETLK64
#if F_GETLK64 != F_GETLK

void
explain_buffer_flock64(explain_string_buffer_t *sb,
    const struct flock64 *flp)
{
    explain_string_buffer_puts(sb, "{ l_type = ");
    explain_buffer_flock_type(sb, flp->l_type);
    explain_string_buffer_puts(sb, ", l_whence = ");
    explain_buffer_lseek_whence(sb, flp->l_whence);
    explain_string_buffer_puts(sb, ", l_start = ");
    explain_buffer_off_t(sb, flp->l_whence);
    explain_string_buffer_puts(sb, ", l_len = ");
    explain_buffer_off_t(sb, flp->l_len);
#ifdef F_UNLKSYS
    explain_string_buffer_puts(sb, ", l_sysid = ");
    explain_buffer_pid_t(sb, flp->l_sysid);
#endif
    explain_string_buffer_puts(sb, ", l_pid = ");
    if (flp->l_pid > 0)
        explain_buffer_pid_t(sb, flp->l_pid);
    else
        explain_buffer_int(sb, (int)flp->l_pid);
    explain_string_buffer_puts(sb, " }");
}

#endif
#endif


/* vim: set ts=8 sw=4 et : */
