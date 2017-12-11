/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
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
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/stat.h>

#include <libexplain/buffer/caption_name_type.h>
#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/does_not_have_inode_modify_permission.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/dirname.h>
#include <libexplain/name_max.h>


void
explain_buffer_does_not_have_inode_modify_permission1(
    explain_string_buffer_t *sb, const char *pathname,
    const struct stat *pathname_st, const char *caption,
    const explain_have_identity_t *hip)
{
    struct stat     dirname_st;
    char            filename[NAME_MAX + 1];
    char            dirname[PATH_MAX + 1];

    explain_dirname(dirname, pathname, sizeof(dirname));
    if (stat(dirname, &dirname_st) < 0)
    {
        memset(&dirname_st, 0, sizeof(dirname_st));
        dirname_st.st_mode = S_IFDIR;
    }
    explain_basename(filename, pathname, sizeof(filename));

    explain_buffer_does_not_have_inode_modify_permission
    (
        sb,
        filename,
        pathname_st,
        caption,
        dirname,
        &dirname_st,
        hip
    );
}


static void
process_does_not_match_the_owner_uid(explain_string_buffer_t *sb,
    const char *kind_of_uid, const char *puid, const char *caption,
    int st_uid)
{
    char            fuid[40];
    explain_string_buffer_t fuid_sb;

    explain_string_buffer_init(&fuid_sb, fuid, sizeof(fuid));
    explain_buffer_uid(&fuid_sb, st_uid);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message supplements the "no inode modify
         * permission" message, explaining that the process effective UID
         * must match the file owner UID.
         *
         * %1$s => the kind of UID, either "real UID" or "effective UID",
         *         already translated
         * %2$s => the numeric UID of the process, and the corresponding
         *         login name from the password file, if available.
         * %3$s => the name of the offending system call argument,
         *         possibly with some additional file type info
         * %4$s => the numeric UID of the file owner, and the
         *         corresponding login name from the password file, if
         *         available.
         */
        i18n("the process %s %s does not match the %s owner UID %s"),
        kind_of_uid,
        puid,
        caption,
        fuid
    );
    explain_buffer_dac_fowner(sb);
}


void
explain_buffer_does_not_have_inode_modify_permission_fd(
    explain_string_buffer_t *sb, int fildes, const char *fildes_caption)
{
    struct stat     st;
    struct stat     *st_p;
    explain_have_identity_t id;

    explain_have_identity_init(&id);
    st_p = &st;
    if (fstat(fildes, st_p) < 0)
        st_p = 0;
    explain_buffer_does_not_have_inode_modify_permission_fd_st
    (
        sb,
        st_p,
        fildes_caption,
        &id
    );
}


void
explain_buffer_does_not_have_inode_modify_permission_fd_st(
    explain_string_buffer_t *sb, const struct stat *fildes_st,
    const char *fildes_caption, const explain_have_identity_t *hip)
{
    explain_string_buffer_t puid_sb;
    explain_string_buffer_t caption_sb;
    const char      *kind_of_uid;
    char            puid[40];
    char            caption[100];

    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a process does not have
         * inode modification permission to something it attempts to
         * modify); for example, fchmod.
         */
        i18n("the process does not have inode modification permission")
    );
    if (!fildes_st)
    {
        explain_buffer_dac_fowner(sb);
        return;
    }

    kind_of_uid = explain_have_identity_kind_of_uid(hip);

    /*
     * Give more information: tell them who they are (this can be a
     * surprise to users of set-UID programs) and who owns the file.
     */
    explain_string_buffer_init(&puid_sb, puid, sizeof(puid));
    explain_buffer_uid(&puid_sb, hip->uid);

    explain_string_buffer_init(&caption_sb, caption, sizeof(caption));
    explain_buffer_caption_name_type_st
    (
        &caption_sb,
        fildes_caption,
        0,
        fildes_st
    );

    explain_string_buffer_puts(sb, ", ");
    process_does_not_match_the_owner_uid
    (
        sb,
        kind_of_uid,
        puid,
        caption,
        fildes_st->st_uid
    );
}


void
explain_buffer_does_not_have_inode_modify_permission(
    explain_string_buffer_t *sb, const char *comp,
    const struct stat *comp_st, const char *caption, const char *dir,
    const struct stat *dir_st, const explain_have_identity_t *hip)
{
    char            final_part[NAME_MAX * 4 + 100];
    explain_string_buffer_t final_part_sb;
    char            dir_part[PATH_MAX * 4 + 100];
    explain_string_buffer_t dir_part_sb;
    const char      *kind_of_uid;
    char            puid[40];
    explain_string_buffer_t puid_sb;

    explain_string_buffer_init
    (
        &final_part_sb,
        final_part,
        sizeof(final_part)
    );
    explain_buffer_caption_name_type_st
    (
        &final_part_sb,
        0,
        comp,
        comp_st
    );
    explain_string_buffer_init(&dir_part_sb, dir_part, sizeof(dir_part));
    explain_buffer_caption_name_type_st
    (
        &dir_part_sb,
        caption,
        dir,
        dir_st
    );

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a process does not have
         * inode modification permission to something it attempts to
         * modify); for example, chmod.
         * Different language grammars may need to rearrange the parts.
         *
         * %1$s => the name of the final component of the path, the
         *         regular file in question (will never have slashes).
         *         It will in clude the name of the file, and the file
         *         type "regular file".
         * %2$s => the name of the directory that contains the regular
         *         file to be executed; it may have zero, one or more
         *         slashes in it.  Will include the name of the function
         *         call argument, the name of the directory, and the
         *         file type "directory".
         */
        i18n("the process does not have inode modification permission "
            "to the %s in the %s"),
        final_part,
        dir_part
    );

    kind_of_uid = explain_have_identity_kind_of_uid(hip);

    /*
     * Give more information: tell them who they are (this can be a
     * surprise to users of set-UID programs) and who owns the file.
     */
    explain_string_buffer_init(&puid_sb, puid, sizeof(puid));
    explain_buffer_uid(&puid_sb, hip->uid);

    explain_string_buffer_init
    (
        &final_part_sb,
        final_part,
        sizeof(final_part)
    );
    explain_buffer_file_type_st(&final_part_sb, comp_st);

    explain_string_buffer_puts(sb, ", ");
    process_does_not_match_the_owner_uid
    (
        sb,
        kind_of_uid,
        puid,
        final_part,
        comp_st->st_uid
    );
}


/* vim: set ts=8 sw=4 et : */
