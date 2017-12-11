/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/ac/poll.h>

#include <libexplain/buffer/fildes.h>
#include <libexplain/buffer/pollfd.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_pollfd_events(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef POLLIN
        { "POLLIN", POLLIN },
#endif
#ifdef POLLPRI
        { "POLLPRI", POLLPRI },
#endif
#ifdef POLLOUT
        { "POLLOUT", POLLOUT },
#endif
#ifdef POLLRDNORM
        { "POLLRDNORM", POLLRDNORM },
#endif
#ifdef POLLRDBAND
        { "POLLRDBAND", POLLRDBAND },
#endif
#ifdef POLLWRNORM
        { "POLLWRNORM", POLLWRNORM },
#endif
#ifdef POLLWRBAND
        { "POLLWRBAND", POLLWRBAND },
#endif
#ifdef POLLMSG
        { "POLLMSG", POLLMSG },
#endif
#ifdef POLLREMOVE
        { "POLLREMOVE", POLLREMOVE },
#endif
#ifdef POLLRDHUP
        { "POLLRDHUP", POLLRDHUP },
#endif
#ifdef POLLERR
        { "POLLERR", POLLERR },
#endif
#ifdef POLLHUP
        { "POLLHUP", POLLHUP },
#endif
#ifdef POLLNVAL
        { "POLLNVAL", POLLNVAL },
#endif
    };
    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_pollfd(explain_string_buffer_t *sb,
    const struct pollfd *data, int include_revents)
{
#ifdef HAVE_POLL_H
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ fd = ");
    explain_buffer_fildes(sb, data->fd);
    explain_string_buffer_puts(sb, ", events = ");
    explain_buffer_pollfd_events(sb, data->events);
    if (include_revents)
    {
        explain_string_buffer_puts(sb, ", revents = ");
        explain_buffer_pollfd_events(sb, data->revents);
    }
    explain_string_buffer_puts(sb, " }");
#else
    (void)include_revents;
    explain_buffer_pointer(sb, data);
#endif
}


void
explain_buffer_pollfd_array(explain_string_buffer_t *sb,
    const struct pollfd *data, int data_size, int include_revents)
{
#ifdef HAVE_POLL_H
    int             j;

    if
    (
        data_size <= 0
    ||
        explain_is_efault_pointer(data, sizeof(*data) * data_size)
    )
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_putc(sb, '{');
    for (j = 0; j < data_size; ++j)
    {
        if (j)
            explain_string_buffer_putc(sb, ',');
        explain_string_buffer_putc(sb, ' ');
        explain_buffer_pollfd(sb, data + j, include_revents);
    }
    explain_string_buffer_puts(sb, " }");
#else
    (void)data_size;
    (void)include_revents;
    explain_buffer_pointer(sb, data);
#endif
}


/* vim: set ts=8 sw=4 et : */
