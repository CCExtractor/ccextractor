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

#include <libexplain/ac/dlfcn.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/string.h>

#include <libexplain/string_to_thing.h>


void *
explain_parse_pointer_or_die(const char *text)
{
    if (0 == strcasecmp(text, "null"))
        return NULL;
    if (0 == strcasecmp(text, "(null)"))
        return NULL;
    if (0 == strcasecmp(text, "nul"))
        return NULL;
    if (0 == strcasecmp(text, "nil"))
        return NULL;
    if (0 == strcasecmp(text, "(nil)"))
        return NULL;
    if (0 == strcmp(text, "stdin"))
        return stdin;
    if (0 == strcmp(text, "stdout"))
        return stdout;
    if (0 == strcmp(text, "stderr"))
        return stderr;

#ifdef HAVE_DLSYM
    if (text[0] == '&')
    {
        void            *handle;

        handle = dlopen(NULL, RTLD_NOW);
        if (handle)
        {
            void            *p;

            /* clear any previous error */
            dlerror();

            p = dlsym(handle, text + 1);
            if (dlerror() == NULL)
            {
                dlclose(handle);
                return p;
            }
            dlclose(handle);
        }
    }
#endif

    /*
     * Pointers are strange beasts, and NULL pointers even stranger.  The ANSI C
     * standard says that the NULL pointer need not be all-bits-zero, but that
     * a NULL pointer *constant* may be *written* as "0".  This does _not_ mean
     * that an arithmetic value of all-bits-zero, when cast to (void *) will
     * actually have the same value as the NULL pointer.  Hence the code below:
     * it uses an explicit constant NULL pointer.
     *
     * I have used such a machine, BTW, and I sincerely hope none of them are
     * still in use today.  Zero was a valid code and data address (and actually
     * used!), so the operating system guaranteed that another page of memory
     * would never be mapped, and the NULL pointer, while written as 0, had a
     * value of the base address of that page of memory.
     */
    if (0 == strcmp(text, "0"))
        return NULL;

#if SIZEOF_VOID_P > SIZEOF_LONG
    return (void *)explain_parse_longlong_or_die(text);
#else
    return (void *)explain_parse_long_or_die(text);
#endif
}


/* vim: set ts=8 sw=4 et : */
