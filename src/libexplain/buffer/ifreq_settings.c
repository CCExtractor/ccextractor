/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#include <libexplain/ac/sys/socket.h>
#include <libexplain/ac/linux/if.h>
#ifndef HAVE_LINUX_IF_H
#include <libexplain/ac/net/if.h>
#endif

#include <libexplain/buffer/ifreq_settings.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_IF_H

static void
explain_buffer_if_settings(explain_string_buffer_t *sb,
    const struct if_settings *data)
{
    explain_string_buffer_printf
    (
        sb,
        "{ type = %u, size = %u }",
        data->type,
        data->size
    );
}

#endif

void
explain_buffer_ifreq_settings(explain_string_buffer_t *sb,
    const struct ifreq *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        const struct ifreq *ifr;

        /*
         * This is actually a huge big sucky union.  This specific case
         * is given the interface name and the settings.
         */
        ifr = data;
        explain_string_buffer_puts(sb, "{ ifr_name = ");
        explain_string_buffer_puts_quoted_n
        (
            sb,
            ifr->ifr_name,
            sizeof(ifr->ifr_name)
        );
#ifdef HAVE_LINUX_IF_H
        explain_string_buffer_puts(sb, ", ifr_settings = ");
#if 0
        explain_buffer_if_settings(sb, &ifr->ifr_settings);
#else
        /*
         * Mysteriously, not all <net/if.h> files have irf->ifr_settings.
         * This is happening on Linux 32-bits vs 64-bits.  FIIK.
         */
        explain_buffer_if_settings
        (
            sb,
            (const struct if_settings *)&ifr->ifr_ifru
        );
#endif
#endif
        explain_string_buffer_puts(sb, " }");
    }
}


/* vim: set ts=8 sw=4 et : */
