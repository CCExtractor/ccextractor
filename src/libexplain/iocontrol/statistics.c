/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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

#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/iocontrol/table.h>


void
explain_iocontrol_statistics(int *total, int *active)
{
    if (total)
        *total = explain_iocontrol_table_size;
    if (active)
    {
        const explain_iocontrol_t *const *tpp;
        const explain_iocontrol_t *const *end;

        end = explain_iocontrol_table + explain_iocontrol_table_size;
        *active = 0;
        for (tpp = explain_iocontrol_table; tpp < end; ++tpp)
        {
            const explain_iocontrol_t *tp;

            tp = *tpp;
            if (tp->print_name || tp->name)
                ++*active;
        }
    }
}


/* vim: set ts=8 sw=4 et : */
