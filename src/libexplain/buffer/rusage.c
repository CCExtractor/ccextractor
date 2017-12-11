/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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

#include <libexplain/ac/sys/resource.h>

#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/rusage.h>
#include <libexplain/buffer/timeval.h>
#include <libexplain/is_efault.h>


void
explain_buffer_rusage(explain_string_buffer_t *sb,
    const struct rusage *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ru_utime = ");
    explain_buffer_timeval(sb, &data->ru_utime);
    explain_string_buffer_puts(sb, ", ru_stime = ");
    explain_buffer_timeval(sb, &data->ru_stime);
    if (data->ru_maxrss)
    {
        explain_string_buffer_puts(sb, ", ru_maxrss = ");
        explain_buffer_long(sb, data->ru_maxrss);
    }
    if (data->ru_ixrss)
    {
        explain_string_buffer_puts(sb, ", ru_ixrss = ");
        explain_buffer_long(sb, data->ru_ixrss);
    }
    if (data->ru_idrss)
    {
        explain_string_buffer_puts(sb, ", ru_idrss = ");
        explain_buffer_long(sb, data->ru_idrss);
    }
    if (data->ru_isrss)
    {
        explain_string_buffer_puts(sb, ", ru_isrss = ");
        explain_buffer_long(sb, data->ru_isrss);
    }
    if (data->ru_minflt)
    {
        explain_string_buffer_puts(sb, ", ru_minflt = ");
        explain_buffer_long(sb, data->ru_minflt);
    }
    if (data->ru_majflt)
    {
        explain_string_buffer_puts(sb, ", ru_majflt = ");
        explain_buffer_long(sb, data->ru_majflt);
    }
    if (data->ru_nswap)
    {
        explain_string_buffer_puts(sb, ", ru_nswap = ");
        explain_buffer_long(sb, data->ru_nswap);
    }
    if (data->ru_inblock)
    {
        explain_string_buffer_puts(sb, ", ru_inblock = ");
        explain_buffer_long(sb, data->ru_inblock);
    }
    if (data->ru_oublock)
    {
        explain_string_buffer_puts(sb, ", ru_oublock = ");
        explain_buffer_long(sb, data->ru_oublock);
    }
    if (data->ru_msgsnd)
    {
        explain_string_buffer_puts(sb, ", ru_msgsnd = ");
        explain_buffer_long(sb, data->ru_msgsnd);
    }
    if (data->ru_msgrcv)
    {
        explain_string_buffer_puts(sb, ", ru_msgrcv = ");
        explain_buffer_long(sb, data->ru_msgrcv);
    }
    if (data->ru_nsignals)
    {
        explain_string_buffer_puts(sb, ", ru_nsignals = ");
        explain_buffer_long(sb, data->ru_nsignals);
    }
    if (data->ru_nvcsw)
    {
        explain_string_buffer_puts(sb, ", ru_nvcsw = ");
        explain_buffer_long(sb, data->ru_nvcsw);
    }
    if (data->ru_nivcsw)
    {
        explain_string_buffer_puts(sb, ", ru_nivcsw = ");
        explain_buffer_long(sb, data->ru_nivcsw);
    }
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
