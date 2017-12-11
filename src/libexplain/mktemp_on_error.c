/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010-2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but
 * WITHOUT ANY WARRANTY; without even the implied warranty
 * ofMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNULesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdlib.h>

#include <libexplain/mktemp.h>
#include <libexplain/output.h>


char *
explain_mktemp_on_error(char *templat)
{
    char            first;
    char            *result;

    /*
     * From mktemp(3)...
     *
     *     "The mktemp() function always returns template.  If a unique name
     *     was created, the last six bytes of template will have been modified
     *     in such a way that the resulting name is unique (i.e., does not
     *     exist already).  If a unique name could not be created, template is
     *     made an empty string."
     *
     * So, instead of returning a NULL pointer like the vast majority of libc
     * functions, this one returns the orginal string, but sets the first byte
     * to '\0'.  Sheesh!  This makes our job much harder.
     */
    first = templat[0];
    result = mktemp(templat);
    /* assert(result == templat); */
    if (result[0] == '\0')
    {
        int             hold_errno;

        hold_errno = errno;
        /* assert(templat[0] == '\0'); */
        templat[0] = first;
        explain_output_error("%s", explain_errno_mktemp(hold_errno, templat));
        templat[0] = '\0';
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
