/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#include <libexplain/ac/dlfcn.h>

#include <libexplain/buffer/pointer.h>
#include <libexplain/option.h>


void
explain_buffer_pointer(explain_string_buffer_t *sb, const void *ptr)
{
    /*
     * FIXME: use process's symbol table (if available) to find symbol name
     * matching this pointer value. This will only get lucky some of the time,
     * but it will definitely be more informative.
     */
    if (ptr == NULL)
    {
        explain_string_buffer_puts(sb, "NULL");
        return;
    }

    /*
     * If it is a pointer to a relocation symbol, we may be able to
     * locate it using the dynamic library support functions.
     */
#ifdef HAVE_DLADDR
    if (explain_option_dialect_specific())
    {
        Dl_info         dli;

        dli.dli_sname = 0;
        /* We have to cast const-ness away, because Solaris is stupid. */
        if (dladdr((void *)ptr, &dli) != 0 && dli.dli_sname)
        {
            explain_string_buffer_printf(sb, "&%s", dli.dli_sname);
            if (dli.dli_saddr != ptr)
            {
                explain_string_buffer_printf
                (
                    sb,
                    " + %ld",
                    (long)((const char *)ptr - (const char *)dli.dli_saddr)
                );
            }
            return;
        }
    }
#endif

    /*
     * Some old systems don't have %p, and some new systems format
     * %p differently than glibc.  Use consistent formatting, it
     * makes the automated tests easier to write.
     */
#if SIZEOF_VOID_P > SIZEOF_LONG
    explain_string_buffer_printf(sb, "0x%08llX", (unsigned long long)ptr);
#else
    explain_string_buffer_printf(sb, "0x%08lX", (unsigned long)ptr);
#endif
}


/* vim: set ts=8 sw=4 et : */
