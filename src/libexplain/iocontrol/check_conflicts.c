/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/net/if.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/table.h>
#include <libexplain/string_buffer.h>


static int
bogus_request_number(int request)
{
    if (request == 0)
        return 1;
#ifdef _IOC_SIZE
    return (_IOC_SIZE(request) == _IOC_SIZEMASK);
#else
    return (request == -1);
#endif
}


static const char *
explode_hash_define(int request)
{
    explain_string_buffer_t sb;
    static char     buffer[100];

    explain_string_buffer_init(&sb, buffer, sizeof(buffer));
    explain_iocontrol_generic_print_hash_define(&sb, request);
    return buffer;
}


static int
looks_like_a_pointer(const char *s)
{
    return (strchr(s, '*') || strchr(s, '['));
}


void
explain_iocontrol_check_conflicts(void)
{
    const explain_iocontrol_t *const *tpp1;
    const explain_iocontrol_t *const *end;
    unsigned        number_of_errors;

    end = explain_iocontrol_table + explain_iocontrol_table_size;
    number_of_errors = 0;
    for (tpp1 = explain_iocontrol_table; tpp1 < end; ++tpp1)
    {
        const explain_iocontrol_t *tp1;
        const explain_iocontrol_t *const *tpp2;
        int             disambiguate_already_mentioned;

        disambiguate_already_mentioned = 0;
        tp1 = *tpp1;
        if (!tp1->name)
        {
            if (tp1->number)
            {
                fprintf
                (
                    stderr,
                    "%s: %d: request number not zero\n",
                    tp1->file,
                    tp1->line - 9
                );
                ++number_of_errors;
            }
            continue;
        }
        if (bogus_request_number(tp1->number))
        {
            fprintf
            (
                stderr,
                "%s: %d: bogus request number 0x%04X\n",
                tp1->file,
                tp1->line - 9,
                tp1->number
            );
            ++number_of_errors;
            continue;
        }
        if (!(tp1->flags & IOCONTROL_FLAG_NON_META))
        {
            unsigned        ioc_dir;
            unsigned        ioc_size;

#ifdef _IOC_DIR
            ioc_dir = _IOC_DIR(tp1->number);
#else
#ifdef IOC_DIRMASK
            ioc_dir = tp1->number & IOC_DIRMASK;
#else
            ioc_dir = 0;
#endif
#endif
#ifdef _IOC_SIZE
            ioc_size = _IOC_SIZE(tp1->number);
#else
#ifdef IOCPARM_LEN
            ioc_size = IOCPARM_LEN(tp1->number);
#else
            ioc_size = 0;
#endif
#endif
            switch (ioc_dir)
            {
#ifdef _IOC_NONE
            case _IOC_NONE:
#endif
#ifdef IOC_VOID
            case IOC_VOID:
#endif
#if !defined(_IOC_NONE) && !defined(IOC_VOID)
            case 0:
#endif
                if
                (
                    tp1->data_size != NOT_A_POINTER
                &&
                    !(tp1->flags & IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE)
                )
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: %s indicates that data_size should be "
                            "NOT_A_POINTER\n",
                        tp1->file,
                        tp1->line - 4,
                        explode_hash_define(tp1->number)
                    );
                    ++number_of_errors;
                }
                break;

#ifdef _IOC_READ
            case _IOC_READ:
#endif
#ifdef IOC_OUT
            case IOC_OUT:
#endif
                if
                (
                    ioc_size != tp1->data_size
                &&
                    !(tp1->flags & IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE)
                )
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: %s indicates that data_size should be %d "
                            "(not %d)\n",
                        tp1->file,
                        tp1->line - 4,
                        explode_hash_define(tp1->number),
                        ioc_size,
                        tp1->data_size
                    );
                    ++number_of_errors;
                }
                if
                (
                    tp1->print_data !=
                        explain_iocontrol_generic_print_data_pointer
                &&
                    !(tp1->flags & IOCONTROL_FLAG_RW)
                )
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: %s indicates that print_data should be "
                            "explain_iocontrol_generic_print_data_pointer "
                            "(%s: %d)\n",
                        tp1->file,
                        tp1->line - 7,
                        explode_hash_define(tp1->number),
                        __FILE__,
                        __LINE__
                    );
                    ++number_of_errors;
                }
                if (tp1->print_data_returned == NULL)
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: %s indicates that print_data_returned should "
                            "not be NULL (%s: %d)\n",
                        tp1->file,
                        tp1->line - 5,
                        explode_hash_define(tp1->number),
                        __FILE__,
                        __LINE__
                    );
                    ++number_of_errors;
                }
                break;

#ifdef _IOC_WRITE
            case _IOC_WRITE:
#endif
#ifdef IOC_IN
            case IOC_IN:
#endif
                if (!(tp1->flags & IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE))
                {
                    if (ioc_size != tp1->data_size)
                    {
                        fprintf
                        (
                            stderr,
                            "%s: %d: %s indicates that data_size should be "
                                "%d (not %d)\n",
                            tp1->file,
                            tp1->line - 4,
                            explode_hash_define(tp1->number),
                            ioc_size,
                            tp1->data_size
                        );
                        ++number_of_errors;
                    }
                }
                if
                (
                    tp1->print_data ==
                        explain_iocontrol_generic_print_data_pointer
                )
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: %s indicates that print_data should not be "
                            "explain_iocontrol_generic_print_data_pointer "
                            "(%s: %d)\n",
                        tp1->file,
                        tp1->line - 7,
                        explode_hash_define(tp1->number),
                        __FILE__,
                        __LINE__
                    );
                    ++number_of_errors;
                }
                if
                (
                    tp1->print_data_returned != NULL
                &&
                    !(tp1->flags & IOCONTROL_FLAG_RW)
                )
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: %s indicates that print_data_returned should "
                            "be NULL (%s: %d)\n",
                        tp1->file,
                        tp1->line - 5,
                        explode_hash_define(tp1->number),
                        __FILE__,
                        __LINE__
                    );
                    ++number_of_errors;
                }
                break;

#ifdef IOC_INOUT
            case IOC_INOUT:
#endif
#if defined(_IOC_READ) && defined(_IOC_WRITE)
            case _IOC_READ | _IOC_WRITE:
#endif
                if
                (
                    ioc_size != tp1->data_size
                &&
                    !(tp1->flags & IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE)
                )
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: %s indicates that data_size should be %d "
                            "(not %d)\n",
                        tp1->file,
                        tp1->line - 4,
                        explode_hash_define(tp1->number),
                        ioc_size,
                        tp1->data_size
                    );
                    ++number_of_errors;
                }
                if
                (
                    tp1->print_data ==
                        explain_iocontrol_generic_print_data_pointer
                )
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: %s indicates that print_data should not be "
                            "explain_iocontrol_generic_print_data_pointer "
                            "(%s: %d)\n",
                        tp1->file,
                        tp1->line - 7,
                        explode_hash_define(tp1->number),
                        __FILE__,
                        __LINE__
                    );
                    ++number_of_errors;
                }
                if (tp1->print_data_returned == NULL)
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: %s indicates that print_data_returned should "
                            "not be NULL (%s: %d)\n",
                        tp1->file,
                        tp1->line - 5,
                        explode_hash_define(tp1->number),
                        __FILE__,
                        __LINE__
                    );
                    ++number_of_errors;
                }
                break;

            default:
                break;
            }
        }

        if (!tp1->data_type || !*tp1->data_type)
        {
            fprintf
            (
                stderr,
                "%s: %d: must set the data_type\n",
                tp1->file,
                tp1->line - 3
            );
            ++number_of_errors;
            continue;
        }

#ifdef SIOCDEVPRIVATE
        if
        (
            tp1->number >= SIOCDEVPRIVATE
        &&
            tp1->number < SIOCDEVPRIVATE + 16
        &&
            !(tp1->flags & IOCONTROL_FLAG_SIZE_DOES_NOT_AGREE)
        )
        {
            if (tp1->data_size != sizeof(struct ifreq))
            {
                fprintf
                (
                    stderr,
                    "%s: %d: the data_size must be sizeof(struct ifreq)\n",
                    tp1->file,
                    tp1->line - 4
                );
                ++number_of_errors;
            }
            if (0 != strcmp(tp1->data_type, "struct ifreq *"))
            {
                fprintf
                (
                    stderr,
                    "%s: %d: the data_type must be \"struct ifreq *\"\n",
                    tp1->file,
                    tp1->line - 3
                );
                ++number_of_errors;
            }
        }
#endif

        if (tp1->data_size == 0)
        {
            fprintf
            (
                stderr,
                "%s: %d: you must define data_size, "
                    "or set it to NOT_A_POINTER\n",
                tp1->file,
                tp1->line - 6
            );
            ++number_of_errors;
        }
        else if (tp1->data_size == NOT_A_POINTER)
        {
            expain_iocontrol_print_func_t f;
            expain_iocontrol_print_func_t rf;

            f = tp1->print_data;
            if (!f)
            {
                fprintf
                (
                    stderr,
                    "%s: %d: you must define print_data, "
                        "e.g. explain_iocontrol_generic_print_data_int\n",
                    tp1->file,
                    tp1->line - 7
                );
                ++number_of_errors;
            }
            else if
            (
                f == explain_iocontrol_generic_print_data_pointer
            ||
                f == explain_iocontrol_generic_print_data_int_star
            ||
                f == explain_iocontrol_generic_print_data_uint_star
            ||
                f == explain_iocontrol_generic_print_data_long_star
            ||
                f == explain_iocontrol_generic_print_data_ulong_star
            )
            {
                fprintf
                (
                    stderr,
                    "%s: %d: weird value for print_data, did you mean"
                        "explain_iocontrol_generic_print_data_int\n",
                    tp1->file,
                    tp1->line - 7
                );
                ++number_of_errors;
            }

            rf = tp1->print_data_returned;
            if (rf)
            {
                fprintf
                (
                    stderr,
                    "%s: %d: you must not define print_data_returned\n",
                    tp1->file,
                    tp1->line - 5
                );
                ++number_of_errors;
            }

            if (0 != strcmp(tp1->data_type, "intptr_t"))
            {
                /*
                 * It must be an integer type that is exactly the same
                 * size as a void*, and the C99 standard only defines
                 * one type with that property: intptr_t
                 *
                 * Many folks wrongly think "int" has this property.
                 */
                fprintf
                (
                    stderr,
                    "%s: %d: you must use \"intptr_t\" for the data_type\n",
                    tp1->file,
                    tp1->line - 3
                );
                ++number_of_errors;
            }
        }
        else
        {
            if (!looks_like_a_pointer(tp1->data_type))
            {
                fprintf
                (
                    stderr,
                    "%s: %d: the data_type must be a pointer\n",
                    tp1->file,
                    tp1->line - 4
                );
                ++number_of_errors;
            }
            if (tp1->print_data == explain_iocontrol_generic_print_data_ignored)
            {
                if (tp1->print_data_returned)
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: should be "
                            "explain_iocontrol_generic_print_data_pointer\n",
                        tp1->file,
                        tp1->line - 7
                    );
                }
                else
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: ignored data should be NOT_A_POINTER\n",
                        tp1->file,
                        tp1->line - 4
                    );
                }
                ++number_of_errors;
            }
            if (!tp1->print_data)
            {
                fprintf
                (
                    stderr,
                    "%s: %d: you must define print_data\n",
                    tp1->file,
                    tp1->line - 7
                );
                ++number_of_errors;
            }
            if (tp1->print_data_returned)
            {
                expain_iocontrol_print_func_t f;

                f = tp1->print_data;
                if
                (
                    !f
                ||
                    f == explain_iocontrol_generic_print_data_ignored
                ||
                    f == explain_iocontrol_generic_print_data_int
                ||
                    f == explain_iocontrol_generic_print_data_uint
                ||
                    f == explain_iocontrol_generic_print_data_long
                ||
                    f == explain_iocontrol_generic_print_data_ulong
                )
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: print_data must be a pointer, did you mean "
                            "explain_iocontrol_generic_print_data_pointer\n",
                        tp1->file,
                        tp1->line - 7
                    );
                    ++number_of_errors;
                }
            }
        }

        for (tpp2 = tpp1 + 1; tpp2 < end; ++tpp2)
        {
            const explain_iocontrol_t *tp2;

            tp2 = *tpp2;
            if (!tp2->name)
                continue;
            if (tp1->number == tp2->number)
            {
                /*
                 * Potential conflict.
                 */
                if (!tp1->disambiguate || !tp2->disambiguate)
                {
                    fprintf
                    (
                        stderr,
                        "%s: %d: conflict: %s vs %s\n",
                        tp1->file,
                        tp1->line - 9,
                        tp1->name,
                        tp2->name
                    );
                    ++number_of_errors;
                    if (!tp1->disambiguate && !disambiguate_already_mentioned)
                    {
                        disambiguate_already_mentioned = 1;
                        fprintf
                        (
                            stderr,
                            "%s: %d: %s has no disambiguate function\n",
                            tp1->file,
                            tp1->line - 7,
                            tp1->name
                        );
                    }
                    if (!tp2->disambiguate)
                    {
                        fprintf
                        (
                            stderr,
                            "%s: %d: %s has no disambiguate function\n",
                            tp2->file,
                            tp2->line - 7,
                            tp2->name
                        );
                    }
                }
            }
        }
    }
    if (number_of_errors > 0)
    {
        fprintf
        (
            stderr,
            "found %u error%s\n",
            number_of_errors,
            (number_of_errors == 1 ? "" : "s")
        );
        exit(1);
    }
}


/* vim: set ts=8 sw=4 et : */
