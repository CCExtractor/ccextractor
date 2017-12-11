/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013, 2014 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/utsname.h>

#include <libexplain/buffer/utsname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/string_buffer.h>
#include <libexplain/is_efault.h>


void
explain_buffer_utsname(explain_string_buffer_t *sb, const struct utsname *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
         explain_buffer_pointer(sb, data);
         return;
    }

    explain_string_buffer_puts(sb, "{ sysname = ");
    explain_string_buffer_puts_quoted_n(sb,
        data->sysname, sizeof(data->sysname));
    explain_string_buffer_puts(sb, ", nodename = ");
    explain_string_buffer_puts_quoted_n(sb,
        data->nodename, sizeof(data->nodename));
    explain_string_buffer_puts(sb, ", release = ");
    explain_string_buffer_puts_quoted_n(sb,
        data->release, sizeof(data->release));
    explain_string_buffer_puts(sb, ", version = ");
    explain_string_buffer_puts_quoted_n(sb,
        data->version, sizeof(data->version));
    explain_string_buffer_puts(sb, ", machine = ");
    explain_string_buffer_puts_quoted_n(sb,
        data->machine, sizeof(data->machine));
#if HAVE_UTSNAME_DOMAINNAME
    explain_string_buffer_puts(sb, ", domainname = ");
    explain_string_buffer_puts_quoted_n(sb,
        data->domainname, sizeof(data->domainname));
#endif
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
