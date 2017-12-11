/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/assert.h>
#include <libexplain/ac/ctype.h>
#include <libexplain/ac/dirent.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/limits.h> /* for PATH_MAX on Solaris */
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/caption_name_type.h>
#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/does_not_have_inode_modify_permission.h>
#include <libexplain/buffer/eexist.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/file_type.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/buffer/wrong_file_type.h>
#include <libexplain/capability.h>
#include <libexplain/fstrcmp.h>
#include <libexplain/getppcwd.h>
#include <libexplain/have_permission.h>
#include <libexplain/name_max.h>
#include <libexplain/option.h>
#include <libexplain/symloopmax.h>


static int
all_slash(const char *s)
{
    while (*s == '/')
        ++s;
    return !*s;
}


static void
look_for_similar(explain_string_buffer_t *sb, const char *lookup_directory,
    const char *component)
{
    DIR             *dp;
    char            best_name[NAME_MAX + 1];
    double          best_weight;
    char            subject[NAME_MAX * 4 + 3];
    explain_string_buffer_t subject_sb;
    struct stat     st;

    dp = opendir(lookup_directory);
    if (!dp)
        return;

    best_name[0] = '\0';
    best_weight = 0.6;
    for (;;)
    {
        struct dirent   *dep;
        double          weight;

        dep = readdir(dp);
        if (!dep)
            break;
        if (0 == strcmp(dep->d_name, "."))
            continue;
        if (0 == strcmp(dep->d_name, ".."))
            continue;
        weight = explain_fstrcasecmp(component, dep->d_name);
        if (best_weight < weight)
        {
            best_weight = weight;
            explain_strendcpy
            (
                best_name,
                dep->d_name,
                best_name + sizeof(best_name)
            );
        }
    }
    closedir(dp);

    if (best_name[0] == '\0')
        return;

    memset(&st, 0, sizeof(st));
    {
        /*
         * see if we can say what kind of file it is
         */
        char            ipath[PATH_MAX + 1];
        char            *ipath_end;
        char            *ip;

        ipath_end = ipath + sizeof(ipath);
        ip = ipath;
        ip = explain_strendcpy(ip, lookup_directory, ipath_end);
        ip = explain_strendcpy(ip, "/", ipath_end);
        ip = explain_strendcpy(ip, best_name, ipath_end);
        lstat(ipath, &st);
    }

    explain_string_buffer_init(&subject_sb, subject, sizeof(subject));
    if (st.st_dev)
        explain_buffer_caption_name_type_st(&subject_sb, 0, best_name, &st);
    else
        explain_buffer_caption_name_type(&subject_sb, 0, best_name, -1);

    explain_string_buffer_puts(sb->footnotes, "; ");
    explain_string_buffer_printf_gettext
    (
        sb->footnotes,
        /*
         * xgettext: This message is issued when a file (or directory
         * component) could not be found, but a sufficiently similar
         * name has been found in the same directory.  This often helps
         * with typographical errors.
         *
         * %1$s => the name (already quoted) and file type (already
         *         translated) of the alternate file found.
         */
        i18n("did you mean the %s instead?"),
        subject
    );
}


static int
command_interpreter_broken(explain_string_buffer_t *sb, const char *pathname)
{
    char            block[512];
    int             fd;
    ssize_t         n;
    char            *end;
    char            *interpreter_pathname;
    explain_final_t final_component;
    char            *bp;

    fd = open(pathname, O_RDONLY);
    if (fd < 0)
        return -1;
    n = read(fd, block, sizeof(block) - 1);
    close(fd);
    if (n < 4)
        return -1;
    end = block + n;
    if (block[0] != '#')
        return -1;
    if (block[1] != '!')
        return -1;
    bp = block + 2;
    while (bp < end && (*bp == ' ' || *bp == '\t'))
        ++bp;
    if (bp >= end)
        return -1;
    interpreter_pathname = bp;
    ++bp;
    while (bp < end && !isspace(*bp))
        ++bp;
    *bp = '\0';

    explain_final_init(&final_component);
    final_component.want_to_execute = 1;
    final_component.follow_interpreter = 0; /* avoid infinite loops */
    return
        explain_buffer_errno_path_resolution
        (
            sb,
            EACCES,
            interpreter_pathname,
            "#!",
            &final_component
        );
}


static int
hash_bang(explain_string_buffer_t *sb, const char *pathname)
{
    int             fd;
    ssize_t         n;
    char            *end;
    char            *interpreter_pathname;
    char            *bp;
    char            block[512];
    char            qintname[PATH_MAX + 1];
    explain_string_buffer_t qintname_sb;

    fd = open(pathname, O_RDONLY);
    if (fd < 0)
        return -1;
    n = read(fd, block, sizeof(block) - 1);
    close(fd);
    if (n < 4)
        return -1;
    end = block + n;
    if (block[0] != '#')
        return -1;
    if (block[1] != '!')
        return -1;
    bp = block + 2;
    while (bp < end && (*bp == ' ' || *bp == '\t'))
        ++bp;
    if (bp >= end)
        return -1;
    interpreter_pathname = bp;
    ++bp;
    while (bp < end && !isspace(*bp))
        ++bp;
    *bp = '\0';

    explain_string_buffer_init(&qintname_sb, qintname, sizeof(qintname));
    explain_string_buffer_puts_quoted(&qintname_sb, interpreter_pathname);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used to explain an EACCES error,
         * where nested #! interpreter files are attempted.
         *
         * %1$s => The quoted pathname of the first file that contains an
         *         interpreter (#!) line, that points at yet another
         *         interpreted file.
         */
        i18n("too many levels of interpreters (%s)"),
        qintname
    );
    return 0;
}


static void
directory_does_not_exist(explain_string_buffer_t *sb, const char *caption,
    const char *dirname)
{
    char            subject[PATH_MAX + 50];
    explain_string_buffer_t subject_sb;

    explain_string_buffer_init(&subject_sb, subject, sizeof(subject));
    explain_buffer_caption_name_type(&subject_sb, caption, dirname, S_IFDIR);
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a path is being used as a
         * directory, when does not exist.
         *
         * %1$s => the name of the system call argument, the quoted path
         *         and the expected file type ("directory", already translated).
         */
        i18n("%s does not exist"),
        subject
    );
}


static void
not_a_directory(explain_string_buffer_t *sb, const char *caption,
    const char *dir, const struct stat *st)
{
    char            subject[PATH_MAX + 50];
    explain_string_buffer_t subject_sb;

    explain_string_buffer_init(&subject_sb, subject, sizeof(subject));
    explain_buffer_caption_name_type(&subject_sb, caption, dir, -1);
    explain_buffer_wrong_file_type_st(sb, st, subject, S_IFDIR);
}


static void
no_such_directory_entry(explain_string_buffer_t *sb, const char *comp,
    int comp_st_mode, const char *dir_caption, const char *dir, int dir_st_mode)
{
    char part1[NAME_MAX * 4 + 100];
    explain_string_buffer_t part1_sb;
    char part2[PATH_MAX * 4 + 100];
    explain_string_buffer_t part2_sb;

    explain_string_buffer_init(&part1_sb, part1, sizeof(part1));
    explain_buffer_caption_name_type(&part1_sb, 0, comp, comp_st_mode);
    explain_string_buffer_init(&part2_sb, part2, sizeof(part2));
    explain_buffer_caption_name_type
    (
        &part2_sb,
        dir_caption,
        dir,
        dir_st_mode
    );

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when directory does not have a
         * directory entry for the named component.
         *
         * Different language grammars may need to rearrange the parts.
         *
         * %1$s => The name of the offending path component (will never have
         *         slashes).  It will be quoted.
         * %2$s => The name of the directory that contains the problematic
         *         component; it may have zero, one or more slashes in it.  Will
         *         include the name of the function call argument, the name of
         *         the directory, and the file type "directory".
         */
        i18n("there is no %s in the %s"),
        part1,
        part2
    );
}


static void
not_possible_to_execute(explain_string_buffer_t *sb, const char *caption,
    const char *name, int st_mode)
{
    char            ftype[NAME_MAX * 4 + 50];
    explain_string_buffer_t ftype_sb;

    explain_string_buffer_init(&ftype_sb, ftype, sizeof(ftype));
    explain_buffer_caption_name_type(&ftype_sb, caption, name, st_mode);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is issued when a user attempts to
         * execute something that is not a file, such as a block special
         * device.
         *
         * %1$s => the name of the system call argument, the name of the
         *         final path component and the type of the file.
         */
        i18n("it is not possible to execute the %s, "
            "only regular files can be executed"),
        ftype
    );
}


static void
does_not_have_search_permission(explain_string_buffer_t *sb,
    const char *comp, const struct stat *comp_st, const char *caption,
    const char *dir, const struct stat *dir_st,
    const explain_have_identity_t *hip)
{
    char part1[NAME_MAX * 4 + 100];
    explain_string_buffer_t part1_sb;
    char part2[PATH_MAX * 4 + 100];
    explain_string_buffer_t part2_sb;

    explain_string_buffer_init(&part1_sb, part1, sizeof(part1));
    explain_buffer_caption_name_type_st(&part1_sb, 0, comp, comp_st);
    explain_string_buffer_init(&part2_sb, part2, sizeof(part2));
    explain_buffer_caption_name_type_st
    (
        &part2_sb,
        caption,
        dir,
        dir_st
    );

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a process does not have
         * search permission to a directory it attempts to traverse.
         * Different language grammars may need to rearrange the parts.
         *
         * %1$s => The name of the component of the path, the subdirectory in
         *         question (will never have slashes).  It will in clude the
         *         name of the file, and the file type "directory".
         * %2$s => The name of the directory that contains the subdirectory in
         *         question; it may have zero, one or more slashes in it.  Will
         *         include the name of the function call argument, the name of
         *         the directory, and the file type "directory".
         */
        i18n("the process does not have search permission to the %s in "
            "the %s"),
        part1,
        part2
    );
    explain_explain_search_permission(sb, comp_st, hip);
}


static void
does_not_have_search_permission1(explain_string_buffer_t *sb,
    const char *caption, const char *dir, const struct stat *dir_st,
    const explain_have_identity_t *hip)
{
    char            dir_part[PATH_MAX + 100];
    explain_string_buffer_t dir_part_sb;

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
         * search permission to a directory it attempts to traverse.
         * (Only used for problems with "." and "/".)
         * Different language grammars may need to rearrange the parts.
         *
         * %1$s => The pathname, the directory in question.  It will
         *         include the name of the function call argument, the
         *         name of the directory, file type "directory".
         */
        i18n("the process does not have search permission to the %s"),
        dir_part
    );
    explain_explain_search_permission(sb, dir_st, hip);
}


static void
does_not_have_execute_permission(explain_string_buffer_t *sb,
    const char *comp, const struct stat *comp_st, const char *caption,
    const char *dir, const struct stat *dir_st,
    const explain_have_identity_t *hip)
{
    char            final_part[NAME_MAX * 4 + 100];
    explain_string_buffer_t final_part_sb;
    char            dir_part[PATH_MAX * 4 + 100];
    explain_string_buffer_t dir_part_sb;

    explain_string_buffer_init
    (
        &final_part_sb,
        final_part,
        sizeof(final_part)
    );
    explain_buffer_caption_name_type_st(&final_part_sb, 0, comp, comp_st);
    explain_string_buffer_init(&dir_part_sb, dir_part, sizeof(dir_part));
    explain_buffer_caption_name_type_st(&dir_part_sb, caption, dir, dir_st);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a process does not have
         * execute permission to something it attempts to execute; for
         * example, one of the execve calls, or similar.
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
        i18n("the process does not have execute permission to the %s in "
            "the %s"),
        final_part,
        dir_part
    );
    explain_explain_execute_permission(sb, comp_st, hip);
}


static void
does_not_have_read_permission(explain_string_buffer_t *sb, const char *comp,
    const struct stat *comp_st, const char *caption, const char *dir,
    const struct stat *dir_st, const explain_have_identity_t *hip)
{
    char            final_part[NAME_MAX * 4 + 100];
    explain_string_buffer_t final_part_sb;
    char            dir_part[PATH_MAX * 4 + 100];
    explain_string_buffer_t dir_part_sb;

    explain_string_buffer_init
    (
        &final_part_sb,
        final_part,
        sizeof(final_part)
    );
    explain_buffer_caption_name_type_st(&final_part_sb, 0, comp, comp_st);
    explain_string_buffer_init(&dir_part_sb, dir_part, sizeof(dir_part));
    explain_buffer_caption_name_type_st(&dir_part_sb, caption, dir, dir_st);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a process does not have
         * read permission to something it attempts to
         * open for reading; for example, open() or fopen().
         * Different language grammars may need to rearrange the parts.
         *
         * %1$s => the name of the final component of the path, the
         *         regular file in question (will never have slashes).
         *         It will include the name of the file, and the file
         *         type "regular file".
         * %2$s => the name of the directory that contains the regular
         *         file to be executed; it may have zero, one or more
         *         slashes in it.  Will include the name of the function
         *         call argument, the name of the directory, and the
         *         file type "directory".
         */
        i18n("the process does not have read permission to the %s in the %s"),
        final_part,
        dir_part
    );
    explain_explain_read_permission(sb, comp_st, hip);
}


static void
does_not_have_write_permission(explain_string_buffer_t *sb, const char *comp,
    const struct stat *comp_st, const char *caption, const char *dir,
    const struct stat *dir_st, const explain_have_identity_t *hip)
{
    char            final_part[NAME_MAX * 4 + 100];
    explain_string_buffer_t final_part_sb;
    char            dir_part[PATH_MAX * 4 + 100];
    explain_string_buffer_t dir_part_sb;

    explain_string_buffer_init
    (
        &final_part_sb,
        final_part,
        sizeof(final_part)
    );
    explain_buffer_caption_name_type_st(&final_part_sb, 0, comp, comp_st);
    explain_string_buffer_init(&dir_part_sb, dir_part, sizeof(dir_part));
    explain_buffer_caption_name_type_st(&dir_part_sb, caption, dir, dir_st);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a process does not have
         * write permission to something it attempts to
         * open for writing; for example, open() or fopen().
         * Different language grammars may need to rearrange the parts.
         *
         * %1$s => the name of the final component of the path, the
         *         regular file in question (will never have slashes).
         *         It will include the name of the file, and the file
         *         type "regular file".
         * %2$s => the name of the directory that contains the regular
         *         file to be executed; it may have zero, one or more
         *         slashes in it.  Will include the name of the function
         *         call argument, the name of the directory, and the
         *         file type "directory".
         */
        i18n("the process does not have write permission to the %s in the %s"),
        final_part,
        dir_part
    );
    explain_explain_write_permission(sb, comp_st, hip);
}


static void
does_not_have_new_directory_entry_permission(explain_string_buffer_t *sb,
    const char *caption, const char *dir, const struct stat *dir_st,
    const char *comp, int comp_st_mode, const explain_have_identity_t *hip)
{
    char            dir_part[PATH_MAX * 4 + 100];
    explain_string_buffer_t dir_part_sb;
    char            final_part[NAME_MAX * 4 + 100];
    explain_string_buffer_t final_part_sb;

    explain_string_buffer_init(&dir_part_sb, dir_part, sizeof(dir_part));
    explain_buffer_caption_name_type_st(&dir_part_sb, caption, dir, dir_st);
    explain_string_buffer_init
    (
        &final_part_sb,
        final_part,
        sizeof(final_part)
    );
    explain_buffer_caption_name_type(&final_part_sb, 0, comp, comp_st_mode);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a process does not have write
         * permission to a directoryin order to create a new directory entry;
         * for example creat(), mkdir(), symlink(), etc.
         * Different language grammars may need to rearrange the parts.
         *
         * %1$s => The name of the directory that is to receive the new
         *         directory entry; it may have zero, one or more slashes in it.
         *         Will include the name of the function call argument, the name
         *         of the directory, and the file type "directory".
         * %2$s => The name of the final component of the path, the
         *         new directory entry in question (will never have slashes).
         *         It will include the name of the new file, and the file type.
         */
        i18n("the process does not have write permission to the %s, this is "
            "needed to create the directory entry for the %s"),
        dir_part,
        final_part
    );
    explain_explain_write_permission(sb, dir_st, hip);
}


static void
dangling_symbolic_link(explain_string_buffer_t *sb, const char *comp,
    int comp_st_mode, const char *caption, const char *dir, int dir_st_mode,
    const char *garbage)
{
    char            final_part[NAME_MAX * 2 + 100];
    explain_string_buffer_t final_part_sb;
    char            dir_part[PATH_MAX * 2 + 100];
    explain_string_buffer_t dir_part_sb;
    char            dangling_part[PATH_MAX * 2 + 100];
    explain_string_buffer_t dangling_part_sb;

    explain_string_buffer_init
    (
        &final_part_sb,
        final_part,
        sizeof(final_part)
    );
    explain_buffer_caption_name_type(&final_part_sb, 0, comp, comp_st_mode);
    explain_string_buffer_init(&dir_part_sb, dir_part, sizeof(dir_part));
    explain_buffer_caption_name_type
    (
        &dir_part_sb,
        caption,
        dir,
        dir_st_mode
    );
    explain_string_buffer_init
    (
        &dangling_part_sb,
        dangling_part,
        sizeof(dangling_part)
    );
    explain_string_buffer_puts_quoted(&dangling_part_sb, garbage);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when there is a dangling
         * symbolic link.
         *
         * Different language grammars may need to rearrange the parts.
         *
         * %1$s => The name of the final component of the path, the name of
         *         symbolic link in question, will include the file type
         *         "symbolic link", but will never have slashes.
         * %2$s => The name of the directory that contains the symbolic link;
         *         it may have zero, one or more slashes in it.  Will include
         *         the name of the function call argument, the name of the
         *         directory, and the file type "directory".
         * %3$s => the non-existent thing the symbolic link point to
         */
        i18n("the %s in the %s refers to %s that does not exist"),
        final_part,
        dir_part,
        dangling_part
    );
}


static void
name_too_long(explain_string_buffer_t *sb, const char *caption,
    const char *component, long comp_length, long name_max)
{
    char            part1[PATH_MAX * 2];
    explain_string_buffer_t part1_sb;

    explain_string_buffer_init(&part1_sb, part1, sizeof(part1));
    explain_string_buffer_puts(&part1_sb, caption);
    explain_string_buffer_putc(&part1_sb, ' ');
    explain_string_buffer_puts_quoted(&part1_sb, component);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a path name component is
         * longer than the system limit (NAME_MAX, not PATH_MAX).
         *
         * %1$s => the name of the function call argument and the quoted
         *         text of the offending path component.
         */
        i18n("%s component is longer than the system limit"),
        part1
    );
    explain_string_buffer_printf(sb, " (%ld > %ld)", comp_length, name_max);
}


static void
not_a_subdirectory(explain_string_buffer_t *sb, const char *comp,
    int comp_st_mode, const char *caption, const char *dir, int dir_st_mode)
{
    char part1[NAME_MAX * 4 + 100];
    explain_string_buffer_t part1_sb;
    char part2[PATH_MAX * 4 + 100];
    explain_string_buffer_t part2_sb;

    explain_string_buffer_init(&part1_sb, part1, sizeof(part1));
    explain_buffer_caption_name_type(&part1_sb, 0, comp, comp_st_mode);
    explain_string_buffer_init(&part2_sb, part2, sizeof(part2));
    explain_buffer_caption_name_type(&part2_sb, caption, dir, dir_st_mode);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when directory has a directory
         * entry for the named component, but a directory was expected
         * and something else was there instead.
         *
         * Different language grammars may need to rearrange the parts.
         *
         * %1$s => The name of the offending path component (will never have
         *         slashes).  It is already quoted.
         * %2$s => The name of the directory that contains the problematic
         *         component; it may have zero, one or more slashes in it.  Will
         *         include the name of the function call argument, the name of
         *         the directory, and the file type "directory".
         */
        i18n("the %s in the %s is being used as a directory when it is not"),
        part1,
        part2
    );
}


static void
wrong_file_type(explain_string_buffer_t *sb, const char *caption,
    const char *dir, int dir_st_mode, const char *comp, int comp_st_mode,
    int wanted_st_mode)
{
    char part1[PATH_MAX * 4 + 100];
    explain_string_buffer_t part1_sb;
    char part2[NAME_MAX * 4 + 100];
    explain_string_buffer_t part2_sb;
    char part3[100];
    explain_string_buffer_t part3_sb;

    explain_string_buffer_init(&part2_sb, part2, sizeof(part2));
    explain_buffer_caption_name_type(&part2_sb, 0, comp, comp_st_mode);
    explain_string_buffer_init(&part1_sb, part1, sizeof(part1));
    explain_buffer_caption_name_type(&part1_sb, caption, dir, dir_st_mode);
    explain_string_buffer_init(&part3_sb, part3, sizeof(part3));
    explain_buffer_file_type(&part3_sb, wanted_st_mode);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when directory has a directory
         * entry for the named component, but a directory was expected
         * and something else was there instead.
         *
         * Different language grammars may need to rearrange the parts.
         *
         * %1$s => The name of the directory that contains the problematic
         *         component; it may have zero, one or more slashes in
         *         it.  It will include the name of the function call
         *         argument, the name of the directory, and the file
         *         type "directory".
         * %2$s => The name of the offending path component and file
         *         type (will never have slashes).  It will be quoted.
         * %3$s => the desired file type
         */
        i18n("in the %s there is a %s, but it should be a %s"),
        part1,
        part2,
        part3
    );
}


static void
need_dir_write_for_remove_dir_entry(explain_string_buffer_t *sb,
    const char *dir_caption, const char *dir_name, int dir_type,
    const char *comp_name, int comp_type)
{
    explain_string_buffer_t dir_part_sb;
    explain_string_buffer_t comp_part_sb;
    char dir_part[PATH_MAX * 4 + 100];
    char comp_part[PATH_MAX * 4 + 100];

    explain_string_buffer_init(&dir_part_sb, dir_part, sizeof(dir_part));
    explain_buffer_caption_name_type
    (
        &dir_part_sb,
        dir_caption,
        dir_name,
        dir_type
    );
    explain_string_buffer_init(&comp_part_sb, comp_part, sizeof(comp_part));
    explain_buffer_caption_name_type(&comp_part_sb, 0, comp_name, comp_type);

    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when the process has
         * insufficient permissions to a directory to remove a directory
         * entry from it.
         *
         * %1$s => The name of the offending system call argument, the
         *         quoted name of the corresponding directory, and its
         *         file type already translated.
         * %2$s => The quoted name of the directory component, and its
         *         file type already translated.
         */
        i18n("the process does not have write permission to the %s, this "
             "is needed to remove the directory entry for the %s"),
        dir_part,
        comp_part
    );
}


static void
explain_sticky_bit_vs_unlink(explain_string_buffer_t *sb,
    const explain_have_identity_t *hip, const struct stat *dir_st,
    const struct stat *file_st)
{
    explain_string_buffer_t proc_part_sb;
    explain_string_buffer_t dir_part_sb;
    explain_string_buffer_t file_part_sb;
    explain_string_buffer_t ftype_sb;
    char            proc_part[100];
    char            dir_part[100];
    char            file_part[100];
    char            ftype[100];

    explain_string_buffer_init(&proc_part_sb, proc_part, sizeof(proc_part));
    explain_string_buffer_init(&dir_part_sb, dir_part, sizeof(dir_part));
    explain_string_buffer_init(&file_part_sb, file_part, sizeof(file_part));
    explain_string_buffer_init(&ftype_sb, ftype, sizeof(ftype));

    explain_buffer_uid(&proc_part_sb, hip->uid);
    explain_buffer_uid(&dir_part_sb, dir_st->st_uid);
    explain_buffer_uid(&file_part_sb, file_st->st_uid);
    explain_buffer_file_type_st(&ftype_sb, file_st);

    explain_string_buffer_puts(sb, ", ");
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * %1$s => the kind of UID, either "effective UID" or "real
         *         UID", already translated
         * %2$s => the process's UID and the corresponding login name,
         *         already quoted
         * %3$s => the file's UID and the corresponding login name,
         *         already quoted
         * %4$s => the type of file to be removed (e.g. "regular file"),
         *         already translated
         * %5$s => the directory's UID and the corresponding login name,
         *         already quoted
         */
        i18n("the directory has the sticky bit (S_ISVTX) set and the "
            "process's %s %s is neither the owner UID %s of the %s "
            "to be removed, nor the owner UID %s of the directory "
            "containing it"),
        explain_have_identity_kind_of_uid(hip),
        proc_part,
        file_part,
        ftype,
        dir_part
    );
    explain_buffer_dac_fowner(sb);
}


static int
current_directory_confusing(void)
{
    char parent_dir[PATH_MAX + 1];
    char child_dir[PATH_MAX + 1];

    /*
     * If this process' current directory differs from that of the
     * parent process' current directory, it means that the user may
     * well have a different idea of "the current directory" than this
     * process does.
     */
    if (!explain_getppcwd(parent_dir, sizeof(parent_dir)))
        return 1;
    if (!getcwd(child_dir, sizeof(child_dir)))
        return 1;
    return (0 != strcmp(parent_dir, child_dir));
}


static int
is_ok_pathname_caption(const char *caption)
{
    /* most of them */
    if (0 == strcmp(caption, "pathname"))
        return 1;
    /* rename */
    if  (0 == strcmp(caption, "oldpath"))
        return 1;
    /* rename */
    if (0 == strcmp(caption, "newpath"))
        return 1;

    /* mount */
    if (0 == strcmp(caption, "source"))
        return 1;
    /* mount */
    if (0 == strcmp(caption, "target"))
        return 1;

    /* heuristic */
    return !strchr(caption, '/');
}


int
explain_buffer_errno_path_resolution(explain_string_buffer_t *sb,
    int expected_errno, const char *initial_pathname, const char *caption,
    const explain_final_t *final_component)
{
    char            pathname[PATH_MAX * 2 + 1];
    explain_string_buffer_t pathname_buf;
    int             number_of_symlinks_followed;
    const char      *pp;
    char            lookup_directory[PATH_MAX + 1];
    explain_string_buffer_t lookup_directory_buf;
    char            **symlinks_so_far;
    int             symloop_max;
    long            path_max;

    /*
     * It's easy to pass pathname twice, rather than pathname and
     * "pathname", so we do a little checking.  Add to this list if
     * something else turns up, but I don't expect any more.
     *
     * RESIST the temptation to add more, because it actually makes the
     * libexplain API inconsistent (at the expense of avoiding lame
     * POSIX argument name inconsistencies).
     */
    assert(is_ok_pathname_caption(caption));

    if (expected_errno == EMLINK)
        expected_errno = ELOOP;
    symlinks_so_far = 0;
    number_of_symlinks_followed = 0;

    /*
     * Empty pathname:
     * In the original Unix, the empty pathname referred to the current
     * directory.  Nowadays POSIX decrees that an empty pathname must
     * not be resolved successfully.  Linux returns ENOENT in this case.
     *
     * FIXME: is there a way pathconf can tell us which semanrics apply?
     *        or maybe sysconf?
     */
    if (!initial_pathname || !*initial_pathname)
    {
#ifdef __sun__
        initial_pathname = ".";
#else
        explain_string_buffer_printf
        (
            sb,
            /*
             * xgettext: this explanation is given for paths that are
             * the empty string.
             *
             * %1$s => the name of the relevant system call argument.
             */
            i18n("POSIX decrees that an empty %s must not be resolved "
                "successfully"),
            caption
        );
        return 0;
#endif
    }

    /*
     * Deal with some boundary conditions.
     * What we want is to always have at least one component.
     */
    if (!initial_pathname || !*initial_pathname)
        initial_pathname = ".";
    else if (all_slash(initial_pathname))
        initial_pathname = "/.";

    path_max = final_component->path_max;
    if (path_max < 0)
        path_max = pathconf(initial_pathname, _PC_PATH_MAX);
    if (path_max <= 0)
        path_max = PATH_MAX;

    if (expected_errno == ENAMETOOLONG)
    {
        size_t          len;

        len = strlen(initial_pathname);
        if (len > (size_t)path_max)
        {
            explain_string_buffer_printf_gettext
            (
                sb,
                /*
                 * xgettext: This message is used when a pathname
                 * exceeds the maximum (system specific) path name
                 * length (in bytes, not characters).
                 *
                 * %1$s => the name of the offending system call argument
                 */
                i18n("%s exceeds the system maximum path length"),
                caption
            );
            explain_string_buffer_printf
            (
                sb,
                " (%ld > %ld)",
                (long)len,
                path_max
            );
            return 0;
        }
    }

    explain_string_buffer_init(&pathname_buf, pathname, sizeof(pathname));
    if (initial_pathname[0] != '/' && current_directory_confusing())
    {
        if (getcwd(pathname, sizeof(pathname) - 1))
        {
            pathname_buf.position = strlen(pathname);
            explain_string_buffer_putc(&pathname_buf, '/');
        }
    }
    explain_string_buffer_puts(&pathname_buf, initial_pathname);

    /*
     * Trailing slashes
     * If a pathname ends in a '/', that forces resolution of the
     * preceding component as in Step 2: it has to exist and resolve
     * to a directory.  Otherwise a trailing '/' is ignored.  (Or,
     * equivalently, a pathname with a trailing '/' is equivalent to the
     * pathname obtained by appending '.' to it.)
     */
    if (pathname[pathname_buf.position - 1] == '/')
        explain_string_buffer_putc(&pathname_buf, '.');

    /*
     * Try to get the system's idea of the loop maximum.
     */
    symloop_max = explain_symloopmax();

    symlinks_so_far = malloc(2 * symloop_max * sizeof(char *));
    if
    (
        !symlinks_so_far
    &&
        (expected_errno == ELOOP || expected_errno == EMLINK)
    )
        return -1;

    pp = pathname;

    /*
     * Set the current lookup directory to the starting lookup directory.
     */
    explain_string_buffer_init
    (
        &lookup_directory_buf,
        lookup_directory,
        sizeof(lookup_directory)
    );
    explain_string_buffer_puts
    (
        &lookup_directory_buf,
        (*pp == '/' ? "/" : ".")
    );
    while (*pp == '/')
        ++pp;

    /*
     * Walk along the path
     */
    for (;;)
    {
        char            component[PATH_MAX + 1]; /* deliberately not NAME_MAX */
        explain_string_buffer_t component_buf;
        char            intermediate_path[PATH_MAX + 1];
        explain_string_buffer_t intermediate_path_buf;
        int             final;
        int             lookup_directory_writable;
        struct stat     lookup_directory_st;
        struct stat     intermediate_path_st;

        /*
         * Check that the lookup directory will play ball.
         */
        if (lstat(lookup_directory, &lookup_directory_st) != 0)
        {
            int lookup_directory_st_errnum = errno;
            if (lookup_directory_st_errnum != ENOENT)
                goto return_minus_1;
            directory_does_not_exist(sb, caption, lookup_directory);
            return_0:
            if (symlinks_so_far)
            {
                int             j;

                for (j = 0; j < number_of_symlinks_followed; ++j)
                    free(symlinks_so_far[j]);
                free(symlinks_so_far);
            }
            return 0;
        }

        /*
         * The lookup directory is never a symbolic link.
         * We take care to ensure it, see below.
         */
        assert(!S_ISLNK(lookup_directory_st.st_mode));

        if (!S_ISDIR(lookup_directory_st.st_mode))
        {
            /*
             * If the lookup_directory is found, but is not a directory,
             * an ENOTDIR error is returned ("Not a directory").
             */
            not_a_directory
            (
                sb,
                caption,
                lookup_directory,
                &lookup_directory_st
            );
            goto return_0;
        }

        /*
         * If the process does not have search permission on the current
         * lookup directory, an EACCES error is returned ("Permission
         * denied").
         */
        if
        (
            !explain_have_search_permission
            (
                &lookup_directory_st,
                &final_component->id
            )
        )
        {
            does_not_have_search_permission1
            (
                sb,
                caption,
                lookup_directory,
                &lookup_directory_st,
                &final_component->id
            );
            goto return_0;
        }

        lookup_directory_writable =
            explain_have_write_permission
            (
                &lookup_directory_st,
                &final_component->id
            );

        /*
         * Extract the next path component.
         * A path component is a substring delimited by '/' characters.
         */
        explain_string_buffer_init
        (
            &component_buf,
            component,
            sizeof(component)
        );
        for (;;)
        {
            unsigned char c = *pp++;
            switch (c)
            {
            default:
                explain_string_buffer_putc(&component_buf, c);
                continue;

            case '\0':
                --pp;
                break;

            case '/':
                break;
            }
            break;
        }
        while (*pp == '/')
            ++pp;
        final = !*pp;

        /*
         * Check the length of the component
         */
        if (expected_errno == ENAMETOOLONG)
        {
            long            no_trunc;
            int             silent_truncate;
            long            name_max;

            /*
             * Components can only be too long if the file system
             * they are on does not silently truncate over-long path
             * components.
             */
            no_trunc = pathconf(lookup_directory, _PC_NO_TRUNC);
            silent_truncate = (no_trunc == 0);

            name_max = pathconf(lookup_directory, _PC_NAME_MAX);
            if (name_max <= 0)
                name_max = NAME_MAX;
            if (!silent_truncate && component_buf.position > (size_t)name_max)
            {
                name_too_long
                (
                    sb,
                    caption,
                    component,
                    (long)component_buf.position,
                    name_max
                );
                goto return_0;
            }
            if (component_buf.position > (size_t)name_max)
            {
                explain_string_buffer_truncate(&component_buf, name_max);
            }
        }
        else
        {
            /*
             * Silently truncate over-long path components.
             */
            long name_max = pathconf(lookup_directory, _PC_NAME_MAX);
            if (name_max <= 0)
                name_max = NAME_MAX;
            if (component_buf.position > (size_t)name_max)
            {
                explain_string_buffer_truncate(&component_buf, name_max);
            }
        }

        /*
         * Build the intermediate path by joining the lookup_directory
         * and the component.
         */
        explain_string_buffer_init
        (
            &intermediate_path_buf,
            intermediate_path,
            sizeof(intermediate_path)
        );
        explain_string_buffer_puts(&intermediate_path_buf, lookup_directory);
        explain_string_buffer_path_join(&intermediate_path_buf, component);

        if (lstat(intermediate_path, &intermediate_path_st) < 0)
        {
            int             intermediate_path_st_errnum;

            intermediate_path_st_errnum = errno;
            if (!final)
            {
                if (intermediate_path_st_errnum == ENOENT)
                {
                    no_such_directory_entry
                    (
                        sb,
                        component,
                        S_IFDIR,
                        caption,
                        lookup_directory,
                        lookup_directory_st.st_mode
                    );

                    /*
                     * If it was a typo, see if we can find something similar.
                     */
                    look_for_similar(sb, lookup_directory, component);
                    goto return_0;
                }

                /*
                 * intermediate path of non-final component returned
                 * unexpected error to lstat (i.e. not ENOENT), bail out
                 */
                goto return_minus_1;
            }

            /*
             * At this point, we know it is a final component.
             */
            if (final_component->must_not_exist)
            {
                if (intermediate_path_st_errnum == ENOENT)
                {
                    if (!lookup_directory_writable)
                    {
                        /* EACCES */
                        does_not_have_new_directory_entry_permission
                        (
                            sb,
                            caption,
                            lookup_directory,
                            &lookup_directory_st,
                            component,
                            final_component->st_mode,
                            &final_component->id
                        );
                        goto return_0;
                    }

                    /*
                     * yay,
                     * but we were looking for an error
                     * and didn't find it
                     */
                    goto return_minus_1;
                }

                /*
                 * The final component is not meant to exist, but the
                 * final component gave an unexpected error to lstat
                 * (i.e. not ENOENT), bail out
                 */
                goto return_minus_1;
            }
            if (final_component->must_exist)
            {
                if (intermediate_path_st_errnum != ENOENT)
                {
                    /*
                     * The final component is meant to exist, but the
                     * final component gave an unexpected error to
                     * lstat (i.e. not success and not ENOENT), bail out
                     */
                    goto return_minus_1;
                }

                no_such_directory_entry
                (
                    sb,
                    component,
                    final_component->st_mode,
                    caption,
                    lookup_directory,
                    lookup_directory_st.st_mode
                );

                /*
                 * If it was a typo, see if we can find something similar.
                 */
                look_for_similar(sb, lookup_directory, component);

                goto return_0;
            }

            /*
             * Creating a new file requires write permission in the
             * containing directory.
             */
            if
            (
                (expected_errno == EACCES || expected_errno == EPERM)
            &&
                intermediate_path_st_errnum == ENOENT
            &&
                final_component->want_to_create
            &&
                !lookup_directory_writable
            )
            {
                does_not_have_new_directory_entry_permission
                (
                    sb,
                    caption,
                    lookup_directory,
                    &lookup_directory_st,
                    component,
                    final_component->st_mode,
                    &final_component->id
                );

                /*
                 * What if they meant to overwrite an existing file?
                 * That would not have needed write permission on the
                 * containing directory.
                 */
                look_for_similar(sb, lookup_directory, component);
                goto return_0;
            }

            /*
             * It's OK if the final component path doesn't exist,
             * but we were looking for an error and didn't find one.
             */
            goto return_minus_1;
        }

        /*
         * At this point, we know that the intermediate path exists.
         */
        if
        (
            S_ISLNK(intermediate_path_st.st_mode)
        &&
            (!final || final_component->follow_symlink)
        )
        {
            int             n;
            char            rlb[PATH_MAX + 1];

            /*
             * Have we read this symlink before?
             */
            if (symlinks_so_far)
            {
                int             j;

                for (j = 0; j < number_of_symlinks_followed; ++j)
                {
                    if (0 == strcmp(symlinks_so_far[j], intermediate_path))
                    {
                        char            qs[PATH_MAX * 2 + 1];
                        explain_string_buffer_t qs_sb;

                        explain_string_buffer_init(&qs_sb, qs, sizeof(qs));
                        explain_string_buffer_puts_quoted
                        (
                            &qs_sb,
                            intermediate_path
                        );
                        explain_string_buffer_printf_gettext
                        (
                            sb,
                            /*
                             * xgettext: This message is used when a
                             * symbolic link loop has been detected,
                             * usually as a result of an ELOOP error.
                             *
                             * %1$s => The name of the offending system
                             *         call argument
                             * %2$s => The path of the first symlink in
                             *         the loop, already quoted.
                             */
                            i18n("a symbolic link loop was encountered in %s, "
                                "starting at %s"),
                            caption,
                            qs
                        );
                        goto return_0;
                    }
                }
            }

            /*
             * Follow the symbolic link, that way we can give the actual
             * path in our error messages.
             */
            n = readlink(intermediate_path, rlb, sizeof(rlb) - 1);
            if (n >= 0)
            {
                char            new_pathname[PATH_MAX + 1];
                explain_string_buffer_t new_pathname_buf;

                if (symlinks_so_far)
                {
                    size_t          len;
                    char            *p;

                    len = strlen(intermediate_path) + 1;
                    p = malloc(len);
                    if (!p)
                    {
                        int             j;

                        for (j = 0; j < number_of_symlinks_followed; ++j)
                            free(symlinks_so_far[j]);
                        free(symlinks_so_far);
                        symlinks_so_far = 0;
                        if (expected_errno == ELOOP || expected_errno == EMLINK)
                            goto return_minus_1;
                    }
                    else
                    {
                        memcpy(p, intermediate_path, len);
                        symlinks_so_far[number_of_symlinks_followed] = p;
                    }
                }

                if (n == 0)
                {
                    rlb[0] = '.';
                    n = 1;
                }
                rlb[n] = '\0';

                explain_string_buffer_init
                (
                    &new_pathname_buf,
                    new_pathname,
                    sizeof(new_pathname)
                );
                explain_string_buffer_puts(&new_pathname_buf, rlb);
                explain_string_buffer_path_join(&new_pathname_buf, pp);

                explain_string_buffer_copy(&pathname_buf, &new_pathname_buf);

                /*
                 * Check for dangling symbolic links
                 */
                explain_string_buffer_init
                (
                    &intermediate_path_buf,
                    intermediate_path,
                    sizeof(intermediate_path)
                );
                if (rlb[0] == '/')
                {
                    explain_string_buffer_puts(&intermediate_path_buf, rlb);
                }
                else
                {
                    explain_string_buffer_puts
                    (
                        &intermediate_path_buf,
                        lookup_directory
                    );
                    explain_string_buffer_path_join
                    (
                        &intermediate_path_buf,
                        rlb
                    );
                }
                if
                (
                    lstat(intermediate_path, &intermediate_path_st) < 0
                &&
                    errno == ENOENT
                )
                {
                    dangling_symbolic_link
                    (
                        sb,
                        component,
                        S_IFLNK,
                        caption,
                        lookup_directory,
                        lookup_directory_st.st_mode,
                        rlb
                    );
                    goto return_0;
                }

                if (rlb[0] == '/')
                {
                    explain_string_buffer_rewind(&lookup_directory_buf);
                    explain_string_buffer_putc(&lookup_directory_buf, '/');
                }
                ++number_of_symlinks_followed;
                if (number_of_symlinks_followed >= symloop_max)
                {
                    explain_string_buffer_printf_gettext
                    (
                        sb,
                        /*
                         * xgettext: This message is used when too
                         * may links (ELOOP or EMLINK) are seen when
                         * resolving a path.
                         *
                         * It may ioptionally be followed by the limit,
                         * in parentheses, so sentence structure that
                         * works that way would be a plus.
                         *
                         * %1$s => The name of the offending system call
                         *         argument.
                         */
                        i18n("too many symbolic links were encountered in %s"),
                        caption
                    );
                    if (explain_option_dialect_specific())
                    {
                        explain_string_buffer_printf
                        (
                            sb,
                            " (%d)",
                            number_of_symlinks_followed
                        );
                    }
                    goto return_0;
                }
                pp = pathname;
                while (*pp == '/')
                    ++pp;
                continue;
            }
        }

        if (!final)
        {
            /*
             * check that intermediate_path is, in fact, a directory
             */
            if (!S_ISDIR(intermediate_path_st.st_mode))
            {
                /* ENOTDIR */
                not_a_subdirectory
                (
                    sb,
                    component,
                    intermediate_path_st.st_mode,
                    caption,
                    lookup_directory,
                    lookup_directory_st.st_mode
                );
                return 0;
            }

            explain_string_buffer_copy
            (
                &lookup_directory_buf,
                &intermediate_path_buf
            );
            continue;
        }

        /*
         * At this point, we know that intermediate_path is the final
         * path, and we know it *does* exist.
         */
        if (final_component->must_not_exist)
        {
            explain_buffer_eexist5
            (
                sb,
                component,
                intermediate_path_st.st_mode,
                lookup_directory,
                lookup_directory_st.st_mode
            );
            goto return_0;
        }

        if
        (
            final_component->must_be_a_st_mode
        &&
            (intermediate_path_st.st_mode & S_IFMT) != final_component->st_mode
        )
        {
            wrong_file_type
            (
                sb,
                caption,
                lookup_directory,
                lookup_directory_st.st_mode,
                component,
                intermediate_path_st.st_mode,
                final_component->st_mode
            );
            goto return_0;
        }

        if
        (
            (expected_errno == EACCES || expected_errno == EPERM)
        &&
            final_component->want_to_modify_inode
        &&
            !explain_have_inode_permission
            (
                &intermediate_path_st,
                &final_component->id
            )
        )
        {
            explain_buffer_does_not_have_inode_modify_permission
            (
                sb,
                component,
                &intermediate_path_st,
                caption,
                lookup_directory,
                &lookup_directory_st,
                &final_component->id
            );
            goto return_0;
        }

        if
        (
            (expected_errno == EACCES || expected_errno == EPERM)
        &&
            final_component->want_to_unlink
        &&
            !lookup_directory_writable
        )
        {
            if ((lookup_directory_st.st_mode & S_ISVTX) == 0)
            {
                /*
                 * No sticky bit set, therefore only need write permissions on
                 * the lookup directory.
                 */
                need_dir_write_for_remove_dir_entry
                (
                    sb,
                    caption,
                    lookup_directory,
                    lookup_directory_st.st_mode,
                    component,
                    intermediate_path_st.st_mode
                );
                goto return_0;
            }
            else
            {
                int             uid;

                uid = final_component->id.uid;
                if
                (
                    uid != (int)intermediate_path_st.st_uid
                &&
                    uid != (int)lookup_directory_st.st_uid
                &&
                    !explain_capability_fowner()
                )
                {
                    need_dir_write_for_remove_dir_entry
                    (
                        sb,
                        caption,
                        lookup_directory,
                        lookup_directory_st.st_mode,
                        component,
                        intermediate_path_st.st_mode
                    );
                    explain_sticky_bit_vs_unlink
                    (
                        sb,
                        &final_component->id,
                        &lookup_directory_st,
                        &intermediate_path_st
                    );
                    goto return_0;
                }
            }
        }

        if (expected_errno == EACCES)
        {
            if
            (
                final_component->want_to_read
            &&
                !explain_have_read_permission
                (
                    &intermediate_path_st,
                    &final_component->id
                )
            )
            {
                does_not_have_read_permission
                (
                    sb,
                    component,
                    &intermediate_path_st,
                    caption,
                    lookup_directory,
                    &lookup_directory_st,
                    &final_component->id
                );
                goto return_0;
            }

            if
            (
                final_component->want_to_write
            &&
                !explain_have_write_permission
                (
                    &intermediate_path_st,
                    &final_component->id
                )
            )
            {
                does_not_have_write_permission
                (
                    sb,
                    component,
                    &intermediate_path_st,
                    caption,
                    lookup_directory,
                    &lookup_directory_st,
                    &final_component->id
                );
                goto return_0;
            }

            if (final_component->want_to_execute)
            {
                if
                (
                    explain_have_execute_permission
                    (
                        &intermediate_path_st,
                        &final_component->id
                    )
                )
                {
                    if
                    (
                        explain_mount_point_noexec(intermediate_path)
                    &&
                        !explain_capability_dac_override()
                    )
                    {
                        explain_buffer_gettext
                        (
                            sb,
                            /*
                             * xgettext: This message is used when the process
                             * attempts to execute a regular file which would
                             * otherwise be executable, except that it resides
                             * on a file system that is mounted with the
                             * "noexec" option.
                             */
                            i18n("the executable is on a file system that is "
                            "mounted with the \"noexec\" option")
                        );
                        explain_buffer_mount_point(sb, intermediate_path);
                        goto return_0;
                    }

                    if
                    (
                        (intermediate_path_st.st_mode & (S_ISUID | S_ISGID))
                    &&
                        explain_mount_point_nosuid(intermediate_path)
                    &&
                        !explain_capability_dac_override()
                    )
                    {
                        explain_buffer_gettext
                        (
                            sb,
                            /*
                             * xgettext: This message is used when the process
                             * attempts to execute a regular file which would
                             * otherwise be executable, except that it has the
                             * set-UID (S_ISUID) or set-GID (S_ISGID) bit set,
                             * and it resides on a file system that is mounted
                             * with the "nosuid" option.
                             */
                            i18n("the executable is on a file system that is "
                            "mounted with the \"nosuid\" option")
                        );
                        explain_buffer_mount_point(sb, intermediate_path);
                        goto return_0;
                    }

                    /*
                     * If it is a #! script, check the command interpreter.
                     * (But avoid command interpreter infinite loops.)
                     */
                    if (final_component->follow_interpreter)
                    {
                        if (!command_interpreter_broken(sb, intermediate_path))
                            goto return_0;
                    }
                    else
                    {
                        if (!hash_bang(sb, intermediate_path))
                            goto return_0;
                    }
                }
                else
                {
                    if (!S_ISREG(intermediate_path_st.st_mode))
                    {
                        not_possible_to_execute
                        (
                            sb,
                            caption,
                            component,
                            intermediate_path_st.st_mode
                        );
                        goto return_0;
                    }

                    does_not_have_execute_permission
                    (
                        sb,
                        component,
                        &intermediate_path_st,
                        caption,
                        lookup_directory,
                        &lookup_directory_st,
                        &final_component->id
                    );
                    goto return_0;
                }
            }

            if
            (
                final_component->want_to_search
            &&
                !explain_have_search_permission
                (
                    &intermediate_path_st,
                    &final_component->id
                )
            )
            {
                does_not_have_search_permission
                (
                    sb,
                    component,
                    &intermediate_path_st,
                    caption,
                    lookup_directory,
                    &lookup_directory_st,
                    &final_component->id
                );
                goto return_0;
            }
        }

        /*
         * No error, yay!  Except that we were looking for an error, and
         * did not find one.
         */
        return_minus_1:
        if (symlinks_so_far)
        {
            int             k;

            for (k = 0; k < number_of_symlinks_followed; ++k)
                free(symlinks_so_far[k]);
            free(symlinks_so_far);
        }
        return -1;
    }
}


void
explain_final_init(explain_final_t *p)
{
    p->want_to_read = 0;
    p->want_to_write = 0;
    p->want_to_search = 0;
    p->want_to_execute = 0;
    p->want_to_create = 0;
    p->want_to_modify_inode = 0;
    p->want_to_unlink = 0;
    p->must_exist = 1;
    p->must_not_exist = 0;
    p->must_be_a_st_mode = 0;
    p->follow_symlink = 1;
    p->follow_interpreter = 1;
    p->st_mode = S_IFREG;
    explain_have_identity_init(&p->id);
    p->path_max = -1;
}


/* vim: set ts=8 sw=4 et : */
