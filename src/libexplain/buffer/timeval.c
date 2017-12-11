/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
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

#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/sys/time.h>
#include <libexplain/ac/time.h>
#include <libexplain/ac/limits.h>
#include <libexplain/ac/math.h>

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/timeval.h>
#include <libexplain/buffer/time_t.h>
#include <libexplain/is_efault.h>


#if 0
#ifndef MAX_TIME_T
#define MAX_TIME_T ((~(time_t)0) >> 1)
#endif
#ifndef MIN_TIME_T
#define MIN_TIME_T (-MAX_TIME_T - 1)
#endif
#endif


static void
print(explain_string_buffer_t *sb, const struct timeval *data)
{
    if (data->tv_usec == UTIME_NOW)
    {
        explain_string_buffer_puts(sb, "{ tv_usec = UTIME_NOW }");
    }
    else if (data->tv_usec == UTIME_OMIT)
    {
        explain_string_buffer_puts(sb, "{ tv_usec = UTIME_OMIT }");
    }
    else if (data->tv_usec < 0u || data->tv_usec >= 1000000u)
    {
        explain_string_buffer_printf
        (
            sb,
            "{ tv_sec = %ld, tv_usec = %ld }",
            (long)data->tv_sec,
            (long)data->tv_usec
        );
    }
    else
    {
        /* FIXME: localime? */
        explain_string_buffer_printf
        (
            sb,
            "{ %.8g seconds }",
            data->tv_sec + 1e-6 * data->tv_usec
        );
    }
}


void
explain_buffer_timeval(explain_string_buffer_t *sb, const struct timeval *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
        print(sb, data);
}


void
explain_buffer_timeval_array(explain_string_buffer_t *sb,
    const struct timeval *data, unsigned data_size)
{
    unsigned        j;

    if (explain_is_efault_pointer(data, sizeof(*data) * data_size))
    {
        explain_buffer_pointer(sb, data);
        return;
    }
    explain_string_buffer_putc(sb, '{');
    for (j = 0; j < data_size; ++j)
    {
        if (j)
            explain_string_buffer_puts(sb, ", ");
        print(sb, data + j);
    }
    explain_string_buffer_putc(sb, '}');
}


void
explain_parse_timeval_or_die(const char *text, const char *caption,
    struct timeval *result)
{
    if (0 == strcmp(text, "UTIME_NOW"))
    {
        result->tv_sec = 0;
        result->tv_usec = UTIME_NOW;
        return;
    }
    if (0 == strcmp(text, "UTIME_OMIT"))
    {
        result->tv_sec = 0;
        result->tv_usec = UTIME_OMIT;
        return;
    }

    {
        char *ep = 0;
        double n = strtod(text, &ep);
        if (ep != text && !*ep)
        {
            /* cope with overflow */
            if (n + 0.5 >= MAX_TIME_T)
            {
                result->tv_sec = MAX_TIME_T;
                result->tv_usec = 0;
                return;
            }

            /* cope with underflow */
            if (n - 0.5 <= MIN_TIME_T)
            {
                result->tv_sec = MIN_TIME_T;
                result->tv_usec = 0;
                return;
            }

            /*
             * Note: tv_sec is signed, but tv_usec is required to be in
             * the range 0..<1e6, because negative values mean something
             * else.  The floor() function will push negative tv_sec
             * values more negative, which means adding the positive
             * tv_usec part brings them back up to the appropriate
             * floating point value.
             */
            result->tv_sec = floor(n);
            result->tv_usec = floor((n - result->tv_sec) * 1e6 + 0.5);
            if (result->tv_usec >= 1000000)
            {
                /* could have overflowed on rounding */
                result->tv_sec++;
                result->tv_usec = 0;
            }
            return;
        }
    }

    {
        time_t n = explain_parse_time_t_or_die(text, caption);
        result->tv_sec = n;
        result->tv_usec = 0;
    }
}


/* vim: set ts=8 sw=4 et : */
