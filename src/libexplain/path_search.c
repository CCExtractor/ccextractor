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
 *
 *
 * Loosely based on code from [e]glibc marked
 *     Copyright (C) 1991-2001, 2006, 2007 Free Software Foundation, Inc.
 *     The GNU C Library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU Lesser General Public
 *     License as published by the Free Software Foundation; either
 *     version 2.1 of the License, or (at your option) any later version.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/stat.h>

#include <libexplain/path_search.h>
#include <libexplain/program_name.h>
#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/software_error.h>


static int
direxists(const char *pathname)
{
    struct stat     st;

    return
        (
            pathname
        &&
            pathname[0]
        &&
            stat(pathname, &st) >= 0
        &&
            S_ISDIR(st.st_mode)
        );
}


int
explain_path_search(char *pathname, size_t pathname_size, const char *dir,
    const char *prefix, int try_tmpdir)
{
    const char      *where;
    size_t          where_size;
    size_t          prefix_size;

    where = NULL;
    if (try_tmpdir)
    {
        const char      *d;

        /*
         * Strangley, [e]glibc tries TMPDIR before the directory in
         * out argument list.  But the comments in the [e]glibc code
         * indicate that it is the other way around.  Huh?!?
         */
        d = getenv("TMPDIR");
        if (direxists(d))
            where = d;
    }
    if (where == NULL && direxists(dir))
        where = dir;
    if (where == NULL && direxists(P_tmpdir))
        where = P_tmpdir;
    if (where == NULL)
    {
        const char      *d;

        d = "/tmp";
        if (direxists(d))
            where = d;
    }
    if (where == NULL)
    {
        errno = ENOENT;
        return -1;
    }

    where_size = strlen(where);
    /* remove trailing slashes */
    while (where_size > 1 && where[where_size - 1] == '/')
        --where_size;

    /*
     * Make sure the prefix is sensable.
     */
    if (!prefix || !prefix[0])
        prefix = explain_program_name_get();
    if (!prefix || !prefix[0])
        prefix = "file";
    prefix_size = strlen(prefix);
    if (prefix_size > 5)
    {
        /*
         * This is the same logic that [e]glibc does.
         *
         * Only use up to the first 5 characters of prefix, to ensure
         * that file name length is <= 11 characters.  But why?  FAT
         * only has 8, V7 Unix had 14, and modern ones have about 255.
         * So why 11?
         *
         * Woudn't it be better to use pathconf(PC_NAMEMAX)?
         */
        prefix_size = 5;
    }

    /*
     * Check we have room for "${where}/${prefix}XXXXXX\0"
     */
    if (pathname_size < where_size + prefix_size + 8)
    {
        errno = EINVAL;
        return -1;
    }

    /*
     * Build the path.
     */
    snprintf
    (
        pathname,
        pathname_size,
        "%.*s/%.*sXXXXXX",
        (int)where_size,
        where,
        (int)prefix_size,
        prefix
    );

    /*
     * Report success.
     */
    return 0;
}


static void
paths_push(const char **paths, size_t *paths_count, const char *path)
{
    size_t          j;

    if (!path)
        return;
    if (!*path)
        return;
    /*
     * Note: we do not check that the path actually is a directory,
     * this is for the error message, where we are complaining that we
     * couldn't find a directory.
     */
    for (j = 0; j < *paths_count; ++j)
        if (strcmp(paths[j], path) == 0)
            return;
    paths[*paths_count] = path;
    ++*paths_count;
}


int
explain_path_search_explanation(explain_string_buffer_t *sb, int errnum,
    const char *dir, int try_tmpdir)
{
    switch (errnum)
    {
    case ENOENT:
        {
            const char      *paths[4];
            size_t          paths_count;
            size_t          j;
            char            paths_text[1000];
            explain_string_buffer_t paths_text_sb;

            /*
             * Build a text string describing the paths searched to
             * find a temporary directory.
             */
            paths_count = 0;
            if (try_tmpdir)
                paths_push(paths, &paths_count, getenv("TMPDIR"));
            paths_push(paths, &paths_count, dir);
            paths_push(paths, &paths_count, P_tmpdir);
            paths_push(paths, &paths_count, "/tmp");
            explain_string_buffer_init(&paths_text_sb, paths_text,
                sizeof(paths_text_sb));
            explain_string_buffer_printf(&paths_text_sb, "%d: ",
                (int)paths_count);
            for (j = 0; j < paths_count; ++j)
            {
                if (j)
                    explain_string_buffer_puts(&paths_text_sb, ", ");
                explain_string_buffer_puts_quoted(&paths_text_sb, paths[j]);
            }

            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext:  This error message is issued when we
                 * are unable to locate a temporary directory in
                 * which to create temporary files (ENOENT).
                 *
                 * %1$s => The list of directories tried, already quoted.
                 */
                i18n("the system was unable to find a temporary directory, "
                    "tried %s"),
                paths_text
            );
        };
        return 0;

    case EINVAL:
        /* FIXME: i18n */
        explain_string_buffer_puts
        (
            sb,
            "the buffer allocated for template file name was too small"
        );
        explain_buffer_software_error(sb);
        return 0;

    case EEXIST:
        {
            const char      *where;

            where = 0;
            if (try_tmpdir)
            {
                const char *d;

                d = getenv("TMPDIR");
                if (direxists(d))
                    where = d;
            }
            if (!where && direxists(dir))
                where = dir;
            if (!where && direxists(P_tmpdir))
                where = P_tmpdir;
            if (!where)
                where = "/tmp";

            explain_buffer_eexist_tempname(sb, where);
        }
        return 0;

    default:
        return -1;
    }
}


/* vim: set ts=8 sw=4 et : */
