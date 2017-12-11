/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/sys/mman.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/is_efault.h>


int
explain_is_efault_pointer(const void *data, size_t data_size)
{
#ifdef HAVE_MINCORE
    /* mincore doesn't seem to work as expected on 64-bit Linux */
    static ssize_t  page_size;
    size_t          lo_pages;
    size_t          hi_pages;
    void            *lo;
    void            *hi;
    size_t          bsize;
    size_t          vec_bytes;

    /*
     * From the mincore(2) man page...
     *
     *     "The vec argument must point to an array containing at least
     *     (length+PAGE_SIZE-1) / PAGE_SIZE bytes.  On return, the least
     *     significant bit of each byte will be set if the corresponding
     *     page is currently resident in memory, and be clear otherwise.
     *     (The settings of the other bits in each byte are undefined;
     *     these bits are reserved for possible later use.)  Of course
     *     the information returned in vec is only a snapshot: pages that
     *     are not locked in memory can come and go at any moment, and
     *     the contents of vec may already be stale by the time this call
     *     returns."
     *
     * Of course, we don't give a damn whether the pages are in physical
     * memory or not, we are after the presence or absence of the ENOMEM
     * error, defined as
     *
     *    "ENOMEM: addr to addr + length contained unmapped memory."
     *
     * to tell us if the pointer points somewhere broken.
     */
    unsigned char   stack_vec[512];

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
    lo_pages = (size_t)data / page_size;
    hi_pages = ((size_t)data + data_size + page_size - 1) / page_size;
    lo = (void *)(lo_pages * page_size);
    hi = (void *)(hi_pages * page_size);
    bsize = (char *)hi - (char *)lo;
    vec_bytes = hi_pages - lo_pages;
    if (vec_bytes > sizeof(stack_vec))
    {
        unsigned char   *heap_vec;

        /*
         * We can't hang on to the array and avoid the malloc next time,
         * because that wouldn't be thread safe.
         */
        heap_vec = malloc(vec_bytes);
        if (!heap_vec)
            goto plan_b;
        if (mincore(lo, bsize, (void *)heap_vec) < 0)
        {
            int             err;

            err = errno;
            free(heap_vec);
            if (err == ENOMEM)
                return 1;
            goto plan_b;
        }
        free(heap_vec);
        return 0;
    }

    if (mincore(lo, bsize, (void *)stack_vec) < 0)
    {
        if (errno == ENOMEM)
            return 1;
        goto plan_b;
    }
    return 0;

#else
    (void)data_size;
    if (!data)
        return 1;
    return explain_is_efault_path(data);
#endif
}


/* vim: set ts=8 sw=4 et : */
