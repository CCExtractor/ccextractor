/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2013 Peter Miller
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

#include <libexplain/ac/limits.h>
#include <libexplain/ac/math.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/stat.h> /* for UTIME_NOW, etc */

#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/time_t.h>
#include <libexplain/buffer/timespec.h>
#include <libexplain/is_efault.h>
#include <libexplain/option.h>


/**
  * The print_longish function is used to print a value from within
  * a struct timespec, taking into account the weird values use by
  * futimens and utimensat
  */
static void
print_longish(explain_string_buffer_t *sb, long x)
{
    if (x == UTIME_NOW)
    {
        explain_string_buffer_puts(sb, "UTIME_NOW");
        return;
    }
    if (x == UTIME_OMIT)
    {
        explain_string_buffer_puts(sb, "UTIME_OMIT");
        return;
    }
    explain_buffer_long(sb, x);
}


static void
print(explain_string_buffer_t *sb, const struct timespec *data)
{
    /**
      * Quoting utimensat(2):
      * "If the tv_nsec field of one of the timespec structures has the
      * special value UTIME_NOW, then the corresponding file timestamp is set
      * to the current time.  If the tv_nsec field of one of the timespec
      * structures has the special value UTIME_OMIT, then the corresponding
      * file timestamp is left unchanged.  In both of these cases, the value of
      * the corresponding tv_sec field is ignored."
      */
    if (data->tv_nsec < 0 || data->tv_nsec >= 1000000000L)
    {
        explain_string_buffer_puts(sb, "{ tv_sec = ");
        if
        (
            data->tv_sec == 0
        &&
            (data->tv_nsec == UTIME_OMIT || data->tv_nsec == UTIME_NOW)
        )
            explain_string_buffer_puts(sb, "0");
        else
            explain_buffer_time_t(sb, data->tv_sec);
        explain_string_buffer_puts(sb, ", tv_nsec = ");
        print_longish(sb, data->tv_nsec);
        explain_string_buffer_puts(sb, " }");
    }
    else
    {
        /*
         * This is a heuristic to distinguish between event dalta times and
         * high-precision system time and date usage.  It may need tuning.
         */
        if (data->tv_sec < 1000)
        {
            explain_string_buffer_printf
            (
                sb,
                "{ %.11g seconds }",
                data->tv_sec + 1e-9 * data->tv_nsec
            );
        }
        else
        {
            explain_string_buffer_printf
            (
                sb,
                "{ %11.9f seconds",
                data->tv_sec + 1e-9 * data->tv_nsec
            );
            if (explain_option_dialect_specific())
            {
                struct tm       *tmp;

                tmp = localtime(&data->tv_sec);
                if (tmp)
                {
                    char            buffer[200];

                    strftime
                    (
                        buffer,
                        sizeof(buffer),
                        " \"%a, %Y-%b-%d %H:%M:%S %z\"",
                        tmp
                    );
                    explain_string_buffer_puts(sb, buffer);
                }
            }
            explain_string_buffer_puts(sb, " }");
        }
    }
}


void
explain_buffer_timespec(explain_string_buffer_t *sb,
    const struct timespec *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
        print(sb, data);
}


void
explain_buffer_timespec_array(explain_string_buffer_t *sb,
    const struct timespec *data, unsigned data_size)
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
explain_parse_timespec_or_die(const char *text, const char *caption,
    struct timespec *result)
{
    if (0 == strcmp(text, "UTIME_NOW"))
    {
        result->tv_sec = 0;
        result->tv_nsec = UTIME_NOW;
        return;
    }
    if (0 == strcmp(text, "UTIME_OMIT"))
    {
        result->tv_sec = 0;
        result->tv_nsec = UTIME_OMIT;
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
                result->tv_nsec = 0;
                return;
            }

            /* cope with underflow */
            if (n - 0.5 <= MIN_TIME_T)
            {
                result->tv_sec = MIN_TIME_T;
                result->tv_nsec = 0;
                return;
            }

            /*
             * Note: tv_sec is signed, but tv_nsec is required to be in
             * the range 0..<1e9, because negative values mean something
             * else.  The floor() function will push negative tv_sec
             * values more negative, which means adding the positive
             * tv_nsec part brings them back up to the appropriate
             * floating point value.
             */
            result->tv_sec = floor(n);
            result->tv_nsec = floor((n - result->tv_sec) * 1e9 + 0.5);
            if (result->tv_nsec >= 1000000000)
            {
                /* could have overflowed on rounding */
                result->tv_sec++;
                result->tv_nsec = 0;
            }
            return;
        }
    }

    {
        time_t n = explain_parse_time_t_or_die(text, caption);
        result->tv_sec = n;
        result->tv_nsec = 0;
    }
}


/* vim: set ts=8 sw=4 et : */
