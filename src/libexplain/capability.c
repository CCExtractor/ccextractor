/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/sys/capability.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/capability.h>


int
explain_capability(int cap)
{
#ifdef HAVE_CAP_GET_PROC
    cap_t           cap_p;

    cap_p = cap_get_proc();
    if (cap_p)
    {
        cap_flag_t           set;
        cap_flag_value_t     value;
        int                  result;

        set = CAP_EFFECTIVE;
        if (cap_get_flag(cap_p, cap, set, &value) < 0)
            value = CAP_CLEAR;

        /*
         * using a temp variable 'result' in case cap_flag_value_t is
         * implemented as a pointer that is invalid after cap_free()
         */
        result = (value != CAP_CLEAR);

        cap_free(cap_p);
        return result;
    }
#else
    (void)cap;
#endif
    return (geteuid() == 0);
}


/* vim: set ts=8 sw=4 et : */
