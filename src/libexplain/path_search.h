/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_PATH_SEARCH_H
#define LIBEXPLAIN_PATH_SEARCH_H

#include <libexplain/ac/stddef.h>

/**
  * The explain_path_search function may be used
  * to determine where to place temporary files.
  *
  * The following are searched for, in order
  *  1. Uses the first of $TMPDIR (if try_tmpdir)
  *  2. the dir argument
  *  3. P_tmpdir
  *  4. /tmp
  *
  * Copies into tmpl a template suitable for use with mkstemp(), et al.
  *
  * @param tmpl
  *     Where to write the pathname template.
  * @param tmpl_len
  *     The maximum available size of the pathname template.
  * @param dir
  *     The preferred temporary directory.
  * @param pfx
  *     file name prefix, must not contail any slash (/) characters
  * @param try_tmpdir
  *     use the $TMPDIR environment variable, if set and exists.
  * @returns
  *     0 on success,or -1 on error (with errno set).
  *
  *     EINVAL
  *         The supplied buffer for tmpl is too small.
  *     ENOENT
  *         None of the directories in the search path exist.
  */
int explain_path_search(char *tmpl, size_t tmpl_len, const char *dir,
    const char *pfx, int try_tmpdir);

struct explain_string_buffer_t;

/**
  * The explain_path_search_explanation function may be used to
  *
  * @param sb
  *     buffer to write into
  * @param errnum
  *     the error to be explained
  * @param dir
  *     directory to search, as passwd to path_search
  * @param try_tmpdir
  *     use the $TMPDIR environment variable, if set and exists.
  * @returns
  *     0 if printed something, -1 if couldn't explain error
  */
int explain_path_search_explanation(struct explain_string_buffer_t *sb,
    int errnum, const char *dir, int try_tmpdir);

#endif /* LIBEXPLAIN_PATH_SEARCH_H */
/* vim: set ts=8 sw=4 et : */
