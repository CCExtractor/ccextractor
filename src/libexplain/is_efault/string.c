/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/ac/errno.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/mman.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/is_efault.h>


int
explain_is_efault_string(const char *data)
{
#ifdef HAVE_MINCORE
    /* mincore doesn't seem to work as expected on 64-bit Linux */
    static ssize_t  page_size;
    size_t          page_num;
    const char      *page_begin;
    const char      *page_end;

    if (!data)
        return 1;
    if (!page_size)
    {
#ifdef HAVE_GETPAGESIZE
        page_size = getpagesize();
#else
        page_size = sysconf(_SC_PAGESIZE);
#endif
    }
    if (page_size <= 0)
    {
        plan_b:
        return explain_is_efault_path(data);
    }
    page_num = (size_t)data / page_size;
    page_begin = (const char *)(page_num * page_size);
    page_end = page_begin + page_size;
    for (;;)
    {
        unsigned char   vec[1];
        const char      *p;

        if (mincore((void *)page_begin, page_size, vec) < 0)
        {
            if (errno == ENOMEM)
                return 1;
            goto plan_b;
        }
        p = memchr(data, '\0', page_end - data);
        if (p)
            return 0;
        data = page_begin;
        page_begin += page_size;
        page_end += page_size;
    }

#else
    if (!data)
        return 1;
    return explain_is_efault_path(data);
#endif
}


/* vim: set ts=8 sw=4 et : */
