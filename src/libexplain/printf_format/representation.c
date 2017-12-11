/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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

#include <libexplain/ac/stdint.h>
#include <libexplain/ac/wchar.h>

#include <libexplain/buffer/double.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/long_double.h>
#include <libexplain/buffer/long_long.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/buffer/ssize_t.h>
#include <libexplain/printf_format.h>


#define PAIR(a, b) \
    (((a) << 8) | (b))


void
explain_printf_format_representation(explain_string_buffer_t *sb,
    const char *format, va_list ap)
{
    explain_printf_format_list_t specs;
    size_t          errpos;

    explain_printf_format_list_constructor(&specs);
    errpos = explain_printf_format(format, &specs);
    if (errpos == 0)
    {
        int             cur_idx;
        size_t          j;

        explain_printf_format_list_sort(&specs);
        /* duplicates are OK, holes are not */
        cur_idx = 0;
        for (j = 0; j < specs.length; ++j)
        {
            const explain_printf_format_t *p;

            p = &specs.list[j];
            explain_string_buffer_puts(sb, ", ");
            if (p->index > cur_idx)
            {
                /* we found a hole */
                break;
            }
            /* duplicates are OK */
            if (p->index == cur_idx)
            {
                switch (PAIR(p->modifier, p->specifier))
                {
                case PAIR('H', 'o'):
                    explain_string_buffer_printf
                    (
                        sb,
                        "%#o",
                        (unsigned char)va_arg(ap, int)
                    );
                    break;

                case PAIR('H', 'd'):
                case PAIR('H', 'i'):
                    explain_buffer_int(sb, (char)va_arg(ap, int));
                    break;

                case PAIR('H', 'u'):
                    explain_buffer_uint(sb, (unsigned char)va_arg(ap, int));
                    break;

                case PAIR('H', 'x'):
                case PAIR('H', 'X'):
                    explain_string_buffer_printf
                    (
                        sb,
                        "0x%02X",
                        (unsigned char)va_arg(ap, int)
                    );
                    break;

                case PAIR(0, 'c'):
                    explain_string_buffer_putc_quoted(sb, va_arg(ap, int));
                    break;

                case PAIR('h', 'o'):
                    explain_string_buffer_printf
                    (
                        sb,
                        "%#o",
                        (unsigned short)va_arg(ap, int)
                    );
                    break;

                case PAIR('h', 'd'):
                case PAIR('h', 'i'):
                    explain_buffer_int(sb, (short)va_arg(ap, int));
                    break;

                case PAIR('h', 'u'):
                    explain_buffer_uint(sb, (unsigned short)va_arg(ap, int));
                    break;

                case PAIR('h', 'x'):
                case PAIR('h', 'X'):
                    explain_string_buffer_printf
                    (
                        sb,
                        "0x%04X",
                        (unsigned short)va_arg(ap, int)
                    );
                    break;

                case PAIR(0, 'o'):
                    explain_string_buffer_printf
                    (
                        sb,
                        "%#o",
                        va_arg(ap, unsigned int)
                    );
                    break;

                case PAIR(0, 'd'):
                case PAIR(0, 'i'):
                    explain_buffer_int(sb, va_arg(ap, int));
                    break;

                case PAIR(0, 'u'):
                    explain_buffer_uint(sb, va_arg(ap, unsigned int));
                    break;

                case PAIR(0, 'x'):
                case PAIR(0, 'X'):
                    explain_string_buffer_printf
                    (
                        sb,
                        "0x%04X",
                        va_arg(ap, unsigned int)
                    );
                    break;

                case PAIR('l', 'o'):
                    explain_string_buffer_printf
                    (
                        sb,
                        "%#lo",
                        va_arg(ap, unsigned long int)
                    );
                    break;

                case PAIR('l', 'd'):
                case PAIR('l', 'i'):
                    explain_buffer_long(sb, va_arg(ap, long int));
                    break;

                case PAIR('l', 'u'):
                    explain_buffer_ulong(sb, va_arg(ap, unsigned long int));
                    break;

                case PAIR('l', 'x'):
                case PAIR('l', 'X'):
                    explain_string_buffer_printf
                    (
                        sb,
                        "0x%08lX",
                        va_arg(ap, unsigned long int)
                    );
                    break;

                case PAIR('l', 'c'):
                    /* FIXME: print the equivalent multi-byte char? */
                    explain_buffer_long(sb, va_arg(ap, wint_t));
                    break;

                case PAIR('L', 'o'):
                case PAIR('q', 'o'):
                    explain_string_buffer_printf
                    (
                        sb,
                        "%#llo",
                        va_arg(ap, unsigned long long int)
                    );
                    break;

                case PAIR('L', 'u'):
                case PAIR('q', 'u'):
                    explain_buffer_unsigned_long_long
                    (
                        sb,
                        va_arg(ap, unsigned long long int)
                    );
                    break;

                case PAIR('L', 'd'):
                case PAIR('q', 'd'):
                case PAIR('L', 'i'):
                case PAIR('q', 'i'):
                    explain_buffer_long_long(sb, va_arg(ap, long long int));
                    break;

                case PAIR('L', 'x'):
                case PAIR('L', 'X'):
                case PAIR('q', 'x'):
                case PAIR('q', 'X'):
                    explain_string_buffer_printf
                    (
                        sb,
                        "0x%08llX",
                        va_arg(ap, unsigned long long int)
                    );
                    break;
                    break;

                case PAIR('j', 'd'):
                case PAIR('j', 'i'):
                    explain_buffer_long_long(sb, va_arg(ap, intmax_t));
                    break;

                case PAIR('j', 'o'):
                case PAIR('j', 'x'):
                case PAIR('j', 'X'):
                case PAIR('j', 'u'):
                    explain_buffer_unsigned_long_long(sb, va_arg(ap, intmax_t));
                    break;

                case PAIR('t', 'd'):
                case PAIR('t', 'i'):
                    explain_buffer_long_long(sb, va_arg(ap, ptrdiff_t));
                    break;

                case PAIR('t', 'o'):
                case PAIR('t', 'x'):
                case PAIR('t', 'X'):
                case PAIR('t', 'u'):
                    explain_buffer_unsigned_long_long
                    (
                        sb,
                        va_arg(ap, ptrdiff_t)
                    );
                    break;

                case PAIR('Z', 'd'):
                case PAIR('Z', 'i'):
                case PAIR('z', 'd'):
                case PAIR('z', 'i'):
                    explain_buffer_ssize_t(sb, va_arg(ap, size_t));
                    break;

                case PAIR('Z', 'X'):
                case PAIR('Z', 'o'):
                case PAIR('Z', 'u'):
                case PAIR('Z', 'x'):
                case PAIR('z', 'X'):
                case PAIR('z', 'o'):
                case PAIR('z', 'u'):
                case PAIR('z', 'x'):
                    explain_buffer_size_t(sb, va_arg(ap, size_t));
                    break;

                case PAIR(0, 'n'):
                    explain_buffer_pointer(sb, va_arg(ap, int *));
                    break;

                case PAIR('l', 'n'):
                    explain_buffer_pointer(sb, va_arg(ap, long *));
                    break;

                case PAIR('L', 'n'):
                    explain_buffer_pointer(sb, va_arg(ap, long long *));
                    break;

                case PAIR('h', 'n'):
                    explain_buffer_pointer(sb, va_arg(ap, short *));
                    break;

                case PAIR('H', 'n'):
                    explain_buffer_pointer(sb, va_arg(ap, char *));
                    break;

                case PAIR(0, 's'):
                    explain_buffer_pathname(sb, va_arg(ap, const char *));
                    break;

                case PAIR('l', 's'):
                    explain_buffer_pointer(sb, va_arg(ap, const wchar_t *));
                    break;

                case PAIR(0, 'p'):
                    explain_buffer_pointer(sb, va_arg(ap, void *));
                    break;

                case PAIR(0, 'a'):
                case PAIR(0, 'A'):
                case PAIR(0, 'e'):
                case PAIR(0, 'f'):
                case PAIR(0, 'g'):
                    explain_buffer_double(sb, va_arg(ap, double));
                    break;

                case PAIR('l', 'a'):
                case PAIR('l', 'A'):
                case PAIR('l', 'e'):
                case PAIR('L', 'e'):
                case PAIR('l', 'f'):
                case PAIR('L', 'f'):
                case PAIR('l', 'g'):
                case PAIR('L', 'g'):
                    explain_buffer_long_double(sb, va_arg(ap, long double));
                    break;

                default:
                    /* Help the poor person who has to debug this one. */
                    explain_string_buffer_putc(sb, '%');
                    if (p->modifier)
                        explain_string_buffer_putc(sb, p->modifier);
                    explain_string_buffer_putc(sb, p->specifier);
                    break;
                }
                ++cur_idx;
            }
        }
    }
    explain_printf_format_list_destructor(&specs);
}


/* vim: set ts=8 sw=4 et : */
