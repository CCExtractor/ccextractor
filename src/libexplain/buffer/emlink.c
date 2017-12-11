/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
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

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/caption_name_type.h>
#include <libexplain/buffer/emlink.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/dirname.h>
#include <libexplain/option.h>


void
explain_buffer_emlink(explain_string_buffer_t *sb, const char *oldpath,
    const char *newpath)
{
    struct stat     oldpath_st;

    if (stat(oldpath, &oldpath_st) >= 0)
    {
        if (S_ISDIR(oldpath_st.st_mode))
        {
            explain_string_buffer_t qnpdir_sb;
            char            npdir[PATH_MAX + 1];
            char            qnpdir[PATH_MAX + 1];

            explain_dirname(npdir, newpath, sizeof(npdir));
            explain_string_buffer_init(&qnpdir_sb, qnpdir, sizeof(qnpdir));
            explain_buffer_caption_name_type
            (
                &qnpdir_sb,
                "newpath",
                npdir,
                S_IFDIR
            );
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used when explaining an
                 * EMLINK error, in the case where a directory needs to
                 * re-write its ".." directory entry, and the new ".."
                 * would thereby exceed the link limit.
                 *
                 * Note that this message may be followed by the actual
                 * limit in parentheses, so it helps of the last phrase
                 * can be sensably followed by it.
                 *
                 * %1$s => The name (already quoted) and file type
                 *         (already translated) of the directory of
                 *         newpath that has the problem.
                 */
                i18n("oldpath is a directory and the %s already has the "
                    "maximum number of links"),
                qnpdir
            );
        }
        else
        {
            explain_string_buffer_t ftype_sb;
            char            ftype[FILE_TYPE_BUFFER_SIZE_MIN];

            explain_string_buffer_init(&ftype_sb, ftype, sizeof(ftype));
            explain_buffer_file_type_st(&ftype_sb, &oldpath_st);
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used when explaining an
                 * EMLINK error, in the non-directory case where a file
                 * already has the maximum number of links.
                 *
                 * Note that this message may be followed by the actual
                 * limit in parentheses, so it helps of the last phrase
                 * can be sensably followed by it.
                 *
                 * %1$s => the file type of the problem file
                 *         (already translated)
                 */
                i18n("oldpath is a %s and already has the maximum number "
                    "of links"),
                ftype
            );
        }
    }
    else
    {
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: This message is used when explaining an EMLINK
             * error, in the case where a specific cause could not be
             * determined.
             */
            i18n("oldpath already has the maximum number of links to "
            "it, or oldpath is a directory and the directory "
            "containing newpath has the maximum number of links")
        );
    }
    if (explain_option_dialect_specific())
    {
        long            link_max;

        /*
         * By definition, oldpath and newpath are on the same file
         * system to get this error, so we don't need to call
         * pathconf twice.
         */
        link_max = pathconf(oldpath, _PC_LINK_MAX);

        if (link_max > 0)
            explain_string_buffer_printf(sb, " (%ld)", link_max);
    }
}


/* vim: set ts=8 sw=4 et : */
