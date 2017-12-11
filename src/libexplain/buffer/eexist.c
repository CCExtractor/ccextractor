/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/caption_name_type.h>
#include <libexplain/buffer/eexist.h>
#include <libexplain/dirname.h>
#include <libexplain/name_max.h>


void
explain_buffer_eexist(explain_string_buffer_t *sb, const char *pathname)
{
    struct stat     pathname_st;
    struct stat     dirname_st;
    char            basename[NAME_MAX + 1];
    char            dirname[PATH_MAX];

    if (lstat(pathname, &pathname_st) < 0)
        pathname_st.st_mode = -1;
    explain_basename(basename, pathname, sizeof(basename));

    explain_dirname(dirname, pathname, sizeof(dirname));
    if (stat(dirname, &dirname_st) < 0)
        dirname_st.st_mode = S_IFDIR;

    explain_buffer_eexist5
    (
        sb,
        basename,
        pathname_st.st_mode,
        dirname,
        dirname_st.st_mode
    );
}


void
explain_buffer_eexist5(explain_string_buffer_t *sb, const char *base_name,
    int base_mode, const char *dir_name, int dir_mode)
{
    explain_string_buffer_t base_cnt_sb;
    explain_string_buffer_t dir_cnt_sb;
    char            base_cnt[NAME_MAX + 50];
    char            dir_cnt[PATH_MAX + 50];

    explain_string_buffer_init(&base_cnt_sb, base_cnt, sizeof(base_cnt));
    explain_buffer_caption_name_type(&base_cnt_sb, 0, base_name, base_mode);

    explain_string_buffer_init(&dir_cnt_sb, dir_cnt, sizeof(dir_cnt));
    explain_buffer_caption_name_type(&dir_cnt_sb, 0, dir_name, dir_mode);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is issued when a system call
         * reports an EEXIST error, in the case where the directory
         * entry to be created already exists, although possibly not the
         * intended type.
         *
         * %1$s => the name and type of the file, the last path component
         * %2$s => the name and type of the containing directory, all but the
         *         last path component.
         */
        i18n("there is already a %s in the %s"),
        base_cnt,
        dir_cnt
    );
}


/* vim: set ts=8 sw=4 et : */
