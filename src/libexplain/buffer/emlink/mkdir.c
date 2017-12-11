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

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/emlink.h>
#include <libexplain/get_link_max.h>
#include <libexplain/option.h>


void
explain_buffer_emlink_mkdir(explain_string_buffer_t *sb, const char *parent,
    const char *pathname_caption)
{
    explain_string_buffer_t parentq_sb;
    char            parentq[PATH_MAX];

    explain_string_buffer_init(&parentq_sb, parentq, sizeof(parentq));
    explain_string_buffer_puts_quoted(&parentq_sb, parent);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining an EMLINK
         * error, in the case where mkdir fails because a directory
         * already has too many hand links.
         *
         * Note that this message may be followed by the actual
         * limit in parentheses, so it helps of the last phrase
         * can be sensably followed by it.
         *
         * %1$s => The name of the offending syscall argument.
         * %2$s => The name (already quoted) of the parent directory
         *         that has the problem.
         */
        i18n("the %s parent directory %s already has the "
            "maximum number of links"),
        pathname_caption,
        parentq
    );
    if (explain_option_dialect_specific())
    {
        struct stat     st;
        long            link_max;

        /*
         * If possible, display the actual value in the directory,
         * rather than the value returned by pathconf().
         */
        if (stat(parent, &st) >= 0)
            link_max = st.st_nlink;
        else
            link_max = explain_get_link_max(parent);
        explain_string_buffer_printf(sb, " (%ld)", link_max);
    }
}


/* vim: set ts=8 sw=4 et : */
