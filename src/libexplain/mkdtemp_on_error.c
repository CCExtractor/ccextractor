/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010-2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/stdio.h> /* for TMP_MAX */
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX everywhere else */
#include <libexplain/ac/sys/stat.h>

#include <libexplain/is_efault.h>
#include <libexplain/mkdtemp.h>
#include <libexplain/output.h>


#ifndef HAVE_MKDTEMP

static char *
mkdtemp(char *template)
{
    char            stash[PATH_MAX + 1];
    size_t          template_size;
    int             n;
    int             save_errno;

    save_errno = errno;
    if (explain_is_efault_string(template))
    {
        errno = EFAULT;
        return NULL;
    }
    template_size = strlen(template) + 1;
    if (template_size > sizeof(stash))
    {
        errno = ENAMETOOLONG;
        return NULL;
    }
    memcpy(stash, template, template_size);
    for (n = 0; n < TMP_MAX; ++n)
    {
        if (!mkstemp(template))
            return NULL;
        if (mkdir(template, 0700) >= 0)
        {
            errno = save_errno;
            return template;
        }
        if (errno != EEXIST)
            return NULL;
        memcpy(template, stash, template_size);
    }

    /*
     * This is the defined error in the case where when we run out of
     * combinations to try.  "Everything we tried already existed."
     */
    errno = EEXIST;
    return NULL;
}

#endif

char *
explain_mkdtemp_on_error(char *pathname)
{
    char            *result;

    result = mkdtemp(pathname);
    if (!result)
    {
        int             hold_errno;

        hold_errno = errno;
        explain_output_error("%s", explain_errno_mkdtemp(hold_errno,
            pathname));
        errno = hold_errno;
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
