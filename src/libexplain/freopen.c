/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/errno/freopen.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/freopen.h>


const char *
explain_freopen(const char *pathname, const char *flags, FILE *fp)
{
    return explain_errno_freopen(errno, pathname, flags, fp);
}


const char *
explain_errno_freopen(int errnum, const char *pathname, const char *flags,
    FILE *fp)
{
    explain_message_errno_freopen
    (
        explain_common_message_buffer,
        explain_common_message_buffer_size,
        errnum,
        pathname,
        flags,
        fp
    );
    return explain_common_message_buffer;
}


void
explain_message_freopen(char *message, int message_size,
    const char *pathname, const char *flags, FILE *fp)
{
    explain_message_errno_freopen
    (
        message,
        message_size,
        errno,
        pathname,
        flags,
        fp
    );
}


void
explain_message_errno_freopen(char *message, int message_size, int errnum,
    const char *pathname, const char *flags, FILE *fp)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_freopen(&sb, errnum, pathname, flags, fp);
}


/* vim: set ts=8 sw=4 et : */
