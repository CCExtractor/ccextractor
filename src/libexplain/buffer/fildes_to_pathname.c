/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
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

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/socket.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/fildes_to_pathname.h>
#include <libexplain/buffer/sockaddr.h>
#include <libexplain/fileinfo.h>
#include <libexplain/string_buffer.h>


static void
explain_buffer_fildes_to_path(explain_string_buffer_t *sb, int fildes)
{
    char            path[PATH_MAX + 1];

    if (explain_fileinfo_self_fd_n(fildes, path, sizeof(path)))
    {
        explain_string_buffer_putc(sb, ' ');
        explain_string_buffer_puts_quoted(sb, path);
    }
}


static int
explain_buffer_fildes_to_sockaddr(explain_string_buffer_t *sb, int fildes)
{
    struct sockaddr_storage sa;
    socklen_t       sas;

    /*
     * Print the address of the local end of the connection.
     */
    sas = sizeof(sa);
    if (getsockname(fildes, (struct sockaddr *)&sa, &sas) < 0)
        return -1;
    explain_string_buffer_putc(sb, ' ');
    explain_buffer_sockaddr(sb, (struct sockaddr *)&sa, sas);

    /*
     * If available, also
     * print the address of the remote end of the connection.
     */
    sas = sizeof(sa);
    if (getpeername(fildes, (struct sockaddr *)&sa, &sas) < 0)
        return 0;
    explain_string_buffer_puts(sb, " => ");
    explain_buffer_sockaddr(sb, (struct sockaddr *)&sa, sas);
    return 0;
}


void
explain_buffer_fildes_to_pathname(explain_string_buffer_t *sb, int fildes)
{
    struct stat     st;

    if (fstat(fildes, &st) < 0)
        return;
    switch (st.st_mode & S_IFMT)
    {
    case S_IFSOCK:
        if (explain_buffer_fildes_to_sockaddr(sb, fildes))
            goto oops;
        break;

    default:
        oops:
        explain_buffer_fildes_to_path(sb, fildes);
        break;
    }
}


/* vim: set ts=8 sw=4 et : */
