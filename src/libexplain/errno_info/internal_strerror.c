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

#include <libexplain/ac/string.h>

#include <libexplain/errno_info.h>
#include <libexplain/option.h>


const char *
explain_internal_strerror(int n)
{
    if (explain_option_internal_strerror())
    {
        const explain_errno_info_t *ep;

        ep = explain_errno_info_by_number(n);
        if (ep)
            return ep->description;
    }
    return strerror(n);
}


void
explain_internal_strerror_r(int errnum, char *data, size_t data_size)
{
    if (explain_option_internal_strerror())
    {
        const explain_errno_info_t *ep;

        ep = explain_errno_info_by_number(errnum);
        if (ep)
        {
            explain_strendcpy(data, ep->description, data + data_size);
            return;
        }
    }

#if 0 /* def HAVE_STRERROR_R */
    /*
     * For reasons unknown, strerror_r is broken (for me) on Linux.
     * FIXME: look into this deeper.
     */
# if STRERROR_R_CHAR_P
    {
        const char      *s;

        s = strerror_r(errnum, data, data_size);
        if (!s || memcmp(s, "Unknown error", 13) == 0)
            data[0] = '\0';
    }
# else
    if (strerror_r(errnum, data, data_size) != 0)
        data[0] = '\0';
# endif
#else
    {
        const char      *s;

        s = strerror(errnum);
        if (!s || memcmp(s, "Unknown error", 13) == 0)
            data[0] = '\0';
        else
            explain_strendcpy(data, s, data + data_size);
    }
#endif
}


/* vim: set ts=8 sw=4 et : */
