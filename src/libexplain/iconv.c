/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013 Peter Miller
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
#include <libexplain/ac/iconv.h>

#include <libexplain/buffer/errno/iconv.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/iconv.h>


const char *
explain_iconv(iconv_t cd, char **inbuf, size_t *inbytesleft, char **outbuf,
    size_t *outbytesleft)
{
    return explain_errno_iconv(errno, cd, inbuf, inbytesleft, outbuf,
        outbytesleft);
}


const char *
explain_errno_iconv(int errnum, iconv_t cd, char **inbuf, size_t *inbytesleft,
    char **outbuf, size_t *outbytesleft)
{
    explain_message_errno_iconv(explain_common_message_buffer,
        explain_common_message_buffer_size, errnum, cd, inbuf, inbytesleft,
        outbuf, outbytesleft);
    return explain_common_message_buffer;
}


void
explain_message_iconv(char *message, int message_size, iconv_t cd, char **inbuf,
    size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
    explain_message_errno_iconv(message, message_size, errno, cd, inbuf,
        inbytesleft, outbuf, outbytesleft);
}


void
explain_message_errno_iconv(char *message, int message_size, int errnum, iconv_t
    cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
    explain_string_buffer_t sb;

    explain_string_buffer_init(&sb, message, message_size);
    explain_buffer_errno_iconv(&sb, errnum, cd, inbuf, inbytesleft, outbuf,
        outbytesleft);
}


/* vim: set ts=8 sw=4 et : */
