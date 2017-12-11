/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/fd_set.h>
#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


void
explain_buffer_fd_set(explain_string_buffer_t *sb, int nfds,
    const fd_set *fds)
{
    int             fildes;
    int             first;

    if (explain_is_efault_pointer(fds, sizeof(*fds)))
    {
        explain_buffer_pointer(sb, fds);
        return;
    }
    first = 1;
    for (fildes = 0; fildes < nfds; ++fildes)
    {
        if (FD_ISSET(fildes, fds))
        {
            explain_string_buffer_putc(sb, (first ? '{' : ','));
            explain_string_buffer_printf(sb, " %d", fildes);
            explain_buffer_fildes_to_pathname(sb, fildes);
            first = 0;
        }
    }
    explain_string_buffer_puts(sb, (first ? "{}" : " }"));
}


/* vim: set ts=8 sw=4 et : */
