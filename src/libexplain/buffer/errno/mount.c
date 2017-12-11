/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013, 2014 Peter Miller
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

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/fcntl.h>
#include <libexplain/ac/mntent.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/sys/mount.h>
#include <libexplain/ac/sys/stat.h> /* for major()/minor() except Solaris */
#include <libexplain/ac/sys/statvfs.h>
#include <libexplain/ac/sys/sysmacros.h> /* for major()/minor() on Solaris */
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/ebusy.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/emfile.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enodev.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/enotblk.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/enxio.h>
#include <libexplain/buffer/eperm.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/mount.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/mount_flags.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>
#include <libexplain/fileinfo.h>
#include <libexplain/fstrcmp.h>
#include <libexplain/is_efault.h>
#include <libexplain/option.h>


static void
explain_buffer_errno_mount_system_call(explain_string_buffer_t *sb, int errnum,
    const char *source, const char *target, const char *file_systems_type,
    unsigned long flags, const void *data)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "mount(source = ");
    explain_buffer_pathname(sb, source);
    explain_string_buffer_puts(sb, ", target = ");
    explain_buffer_pathname(sb, target);
    explain_string_buffer_puts(sb, ", file_system_type = ");
    explain_buffer_pathname(sb, file_systems_type);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_mount_flags(sb, flags);

    /*
     * From mount(2) man page:
     * "The data argument is interpreted differently by the different
     * file systems.  Typically it is a string of comma-separated
     * options understood by this file system.  See mount(8) for details
     * of the options available for each filesystem type."
     */
    explain_string_buffer_puts(sb, ", data = ");
    explain_buffer_pathname(sb, data);
    explain_string_buffer_putc(sb, ')');
}


static int
is_a_known_file_system_type(const char *fs_type, char **fuzzy)
{
    FILE *fp;

    if (!fs_type)
        return 0;
    if (explain_is_efault_path(fs_type))
        return 0;
    if (!*fs_type)
        return 0;

    fp = fopen("/proc/filesystems", "r");
    if (fp)
    {
        double best_w = 0.6;
        char *best_name = 0;
        for(;;)
        {
            char line[80];
            char *col2;
            char *col2_end;

            if (!fgets(line, sizeof(line), fp))
                break;
            col2 = strchr(line, '\t');
            if (!col2)
                continue;
            ++col2;

            col2_end = strchr(col2, '\n');
            if (col2_end)
                *col2_end = '\0';

            if (0 == strcmp(fs_type, col2))
                return 1;

            /*
             * do a fuzzy match to improve error messages for command-line users
             */
            {
                double w = explain_fstrcmp(fs_type, col2);
                if (w > best_w)
                {
                    best_w = w;
                    if (best_name)
                        free(best_name);
                    best_name = strdup(col2);
                }
            }
        }
        fclose(fp);

        if (best_name && fuzzy)
        {
            *fuzzy = best_name;
        }
    }

    /*
     * Linux is civilised, I have no ide how to get at this data on any
     * other Posix o/s.
     */
    return -1;
}


static int
target_is_already_mounted(const char *target)
{
    FILE            *fp;

    fp = setmntent(MOUNTED, "r");
    if (!fp)
        return 0;
    for (;;)
    {
        const char      *dir;
#if GETMNTENT_NARGS == 2
        struct mnttab   mdata;

        if (getmntent(fp, &mdata) != 0)
            break;
        dir = mdata.mnt_mountp;
#else
        struct mntent   *mnt;

        /* FIXME: use getmntent_r if available */
        mnt = getmntent(fp);
        if (!mnt)
            break;
        dir = mnt->mnt_dir;
#endif
        /* FIXME: using the device info from stat(2) would be more accurate */
        if (0 == strcmp(target, dir))
        {
            endmntent(fp);
            return 1;
        }
    }
    endmntent(fp);
    return 0;
}


static int
source_is_already_mounted(const char *source)
{
    FILE            *fp;

    fp = setmntent(MOUNTED, "r");
    if (!fp)
        return -1;
    for (;;)
    {
        const char      *dev;
#if GETMNTENT_NARGS == 2
        struct mnttab   mdata;

        if (getmntent(fp, &mdata) != 0)
            break;
        dev = mdata.mnt_device;
#else
        struct mntent   *mnt;

        /* FIXME: use getmntent_r if available */
        mnt = getmntent(fp);
        if (!mnt)
            break;
        dev = mnt->mnt_fsname;
#endif
        /* FIXME: using the device info from stat(2) would be more accurate */
        if (0 == strcmp(source, dev))
        {
            endmntent(fp);
            return 1;
        }
    }
    endmntent(fp);
    return 0;
}


static int
source_mounted_on_target(const char *source, const char *target)
{
    FILE            *fp;

    fp = setmntent(MOUNTED, "r");
    if (!fp)
        return -1;
    for (;;)
    {
        const char      *dev;
        const char      *dir;
#if GETMNTENT_NARGS == 2
        struct mnttab   mdata;

        if (getmntent(fp, &mdata) != 0)
            break;
        dev = mdata.mnt_device;
        dir = mdata.mnt_dir
#else
        struct mntent   *mnt;

        /* FIXME: use getmntent_r if available */
        mnt = getmntent(fp);
        if (!mnt)
            break;
        dev = mnt->mnt_fsname;
        dir = mnt->mnt_dir;
#endif
        /* FIXME: using the device info from stat(2) would be more accurate */
        if (0 == strcmp(source, dev) && 0 == strcmp(target, dir))
        {
            endmntent(fp);
            return 1;
        }
    }
    endmntent(fp);
    return 0;
}


static int
source_is_in_partition_table(const char *source)
{
    int found;
    struct stat st;
    FILE *fp;

    if (stat(source, &st) < 0)
        return -1;
    fp = fopen("/proc/partitions", "r");
    if (!fp)
        return -1;
    found = 0;
    for (;;)
    {
        char line[100];
        char *lp;
        char *ep;
        unsigned maj;
        unsigned min;
        unsigned dev;
        unsigned long blocks;

        if (!fgets(line, sizeof(line), fp))
            break;
        if (memcmp(line, "major", 5))
            continue;
        if (line[0] == '\n')
            continue;

         lp = line;
         ep = 0;
         maj = strtoul(lp, &ep, 0);
         lp = ep;
         min = strtoul(lp, &ep, 0);
         lp = ep;
         blocks = strtoul(lp, &ep, 0);
         lp = ep;

         while (isspace((unsigned char)*lp))
             ++lp;
         dev = makedev(maj, min);
         if (dev == st.st_rdev)
             ++found;
        (void)blocks;
    }
    return found;
}


static int
file_system_type_needs_block_special_device( const char *file_system_type)
{
    FILE *fp = fopen("/proc/filesystems", "r");
    if (!fp)
        return -1;
    for (;;)
    {
        char line[100];
        char *end;
        char *column1;
        char *column2;
        char *tab;

        if (!fgets(line, sizeof(line), fp))
            break;
        end = line + strlen(line);
        while (end > line && isspace((unsigned char)(end[-1])))
            *--end = '\0';
        column1 = line;
        tab = strchr(line, '\t');
        if (tab)
        {
            *tab++ = 0;
        }
        column2 = tab;
        if (0 == strcmp(column2, file_system_type))
        {
            fclose(fp);
            return (0 != strcmp(column1, "nodev"));
        }
    }
    fclose(fp);
    return 0;
}


static int
is_a_block_special_device(const char *source)
{
    struct stat st;
    if (stat(source, &st) < 0)
        return -1;
    return !!S_ISBLK(st.st_mode);
}


void
explain_buffer_errno_mount_explanation(explain_string_buffer_t *sb, int errnum,
    const char *syscall_name, const char *source, const char *target,
    const char *file_system_type, unsigned long flags, const void *data)
{
   explain_final_t source_fc;
   explain_final_t target_fc;

   explain_final_init(&source_fc);
   source_fc.must_be_a_st_mode = 1;
   source_fc.st_mode = S_IFBLK;
   source_fc.must_exist = 1;

   explain_final_init(&target_fc);
   target_fc.must_be_a_st_mode = 1;
   target_fc.st_mode = S_IFDIR;
   target_fc.must_exist = 1;

#ifdef MS_MGC_MSK
   if ((flags & MS_MGC_MSK) & MS_MGC_VAL)
       flags &= ~MS_MGC_MSK;
#endif

    switch (errnum)
    {
    case EACCES:
        /*
         * A component of a path was not searchable.
         * See also path_resolution(7).
         */
        if
        (
            0 ==
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                source,
                "source",
                &source_fc
            )
        )
            return;
        if
        (
            0 ==
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                target,
                "target",
                &target_fc
            )
        )
            return;

        /*
         * Or, mounting a read-only file system was attempted without
         * giving the MS_RDONLY flag.
         */
#ifdef MS_RDONLY
        /* FIXME: this is Linux specific, add more systems? */
        if (!(flags & MS_RDONLY))
        {
#ifdef BLKROGET
            int fd = open(source, O_RDONLY, 0);
            if (fd >= 0)
            {
                if (ioctl(fd, BLKROGET, 0) > 0)
                {
                    explain_buffer_gettext
                    (
                        sb,
                        /* FIXME: i18n */
                        "mounting a read-only source device was attempted "
                        "without giving the MS_RDONLY flag "
                    );
                    return;
                }
            }
            close(fd);
#endif
        }
#endif

        /*
         * Or, the block device source is located on a file system
         * mounted with the MS_NODEV option.
         */
#ifdef MS_NODEV
        /* FIXME: this is Linux specific, add more systems? */
        {
            struct statvfs info;
            if (statvfs(source, &info) >  0)
            {
                if (info.f_flag & MS_NODEV)
                {
                     explain_buffer_gettext
                     (
                        sb,
                        "the source block device is located on a file "
                        "system mounted with the MS_NODEV option"
                     );
                     if (explain_option_dialect_specific())
                     {
                        explain_buffer_mount_point(sb, source);
                     }
                     return;
                }
            }
        }
#endif

        /* no idea */
        explain_buffer_eacces(sb, target, "target", &target_fc);
        return;

    case EBUSY:
        /*
         * source is already mounted
         */
        if (source_is_already_mounted(source))
        {
            explain_buffer_ebusy_path(sb, source, "source", syscall_name);
            return;
        }

        /*
         * Or, it cannot be remounted read-only, because it still holds
         * files open for writing.
         */
#ifdef MS_REMOUNT
        {
            unsigned long flags2 = MS_RDONLY | MS_REMOUNT;
            if ((flags & flags2) == flags2)
            {
                explain_buffer_gettext
                (
                    sb,
                    /* FIXME: i18n */
                    "source cannot be remounted read-only, because it "
                    "still holds files open for writing"
                );
                return;
            }
        }
#endif
        if (target_is_already_mounted(target))
        {
            explain_buffer_gettext
            (
                sb,
                /* FIXME: i18n */
                "target is already in use as a mount point"
            );
            /* FIXME: tell th user *which* block device */
            return;
        }

        /*
         * Or,
         * it cannot be mounted on target because target is still busy
         *    - (it is the working directory of some thread,
         *    - the mount point of another device,
         *    - has open files, etc.).
         */
        if (explain_fileinfo_dir_tree_in_use(target))
        {
            explain_buffer_gettext
            (
                sb,
                /* FIXME: i18n */
                "target is still busy with open files or working diretories "
            );
            return;
        }

        /* no idea */
        break;

    case EFAULT:
        /*
         * One of the pointer arguments points outside the user address
         * space.
         */
        if (!source)
        {
            explain_buffer_is_the_null_pointer(sb, "source");
            return;
        }
        if (explain_is_efault_path(source))
        {
            explain_buffer_efault(sb, "source");
            return;
        }

        if (!target)
        {
            explain_buffer_is_the_null_pointer(sb, "target");
            return;
        }
        if (explain_is_efault_path(target))
        {
            explain_buffer_efault(sb, "target");
            return;
        }

        if (!file_system_type)
        {
            explain_buffer_is_the_null_pointer(sb, "file_system_type");
            return;
        }
        if (explain_is_efault_path(file_system_type))
        {
            explain_buffer_efault(sb, "file_system_type");
            return;
        }

        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            return;
        }
        if (explain_is_efault_path(data))
        {
            explain_buffer_efault(sb, "data");
            return;
        }
        /* No idea... */
        break;

    case EINVAL:
        if (!target)
        {
            explain_buffer_is_the_null_pointer(sb, "target");
            return;
        }
        if (!*target)
        {
            explain_buffer_gettext
            (
                sb,
                "target is the empty string"
            );
            return;
        }

        /*
         * Or, a remount (MS_REMOUNT) was attempted, but source was not
         * already mounted on target.
         */
#ifdef MS_REMOUNT
        if ((flags & MS_REMOUNT) && !source_mounted_on_target(source, target))
        {
            explain_buffer_gettext
            (
                sb,
                /* FIXME: i18n */
                "a remount was attempted when source was not already mounted "
                "on target"
            );
            return;
        }
#endif

        /*
         * Or, a move (MS_MOVE) was attempted, but source was not a
         * mount point, or was '/'.
         */
#ifdef HAVE_SYS_MOUNT_MS_MOVE
        if (flags & MS_MOVE)
        {
             if (0 == strcmp(target, "/"))
             {
                explain_buffer_gettext
                (
                    sb,
                    /* FIXME: i18n */
                    "it is forbidden to move root"
                );
                return;
             }
             if (!target_is_already_mounted(target))
             {
                 explain_buffer_gettext
                 (
                    sb,
                    /* FIXME: i18n */
                    "a move was attempted when target was not a mount point"
                );
                return;
            }
        }
#endif

        /*
         * (we have ruled out most everything else)
         * Pure guess:
         * source had an invalid superblock.
         */
        explain_buffer_gettext
        (
            sb,
            /* FIXME: i18n */
            "source had an invalid superblock"
        );
        return;

    case ELOOP:
        if
        (
            0 ==
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                source,
                "source",
                &source_fc
            )
        )
            return;
        if
        (
            0 ==
            explain_buffer_errno_path_resolution
            (
                sb,
                errnum,
                target,
                "target",
                &target_fc
            )
        )
            return;

        /* No idea... */
        break;


    case EMFILE:
        /*
         * (In case no block device is required:)
         * Table of dummy devices is full.
         */
        if (is_a_block_special_device(source) == 0)
        {
            explain_buffer_gettext
            (
                sb,
                /* FIXME: i18n */
                "kernel table of dummy devices is full"
            );
            return;
        }

        /* No idea... */
        break;

    case ENAMETOOLONG:
        /*
         * A pathname was longer than MAXPATHLEN.
         */
        if (0 == explain_buffer_errno_path_resolution(sb, errnum, source,
                    "source", &source_fc))
            return;
        if (0 == explain_buffer_errno_path_resolution(sb, errnum, target,
                    "target", &target_fc))
            return;
        /* No idea... */
        break;

    case ENODEV:
        explain_buffer_enodev(sb);
        break;

    case ENOENT:
        /*
         * file_system_type not configured in the kernel.
         */

        {
            char *fuzzy = 0;
            if (is_a_known_file_system_type(file_system_type, &fuzzy) == 0)
            {
                /* how about a fuzzy match, for access via the command line */
                explain_buffer_gettext
                (
                    sb,
                    /* FIXME: i18n */
                    "the file_system_type is not available in this kernel, "
                    "there may be no such file system, or there is, but it "
                    "was not compiled into this kernel"
                );
                if (fuzzy)
                {
                    char ftext[100];
                    explain_string_buffer_t ftext_sb;
                    explain_string_buffer_init(&ftext_sb, ftext, sizeof(ftext));
                    explain_string_buffer_puts_quoted(&ftext_sb, fuzzy);
                    explain_string_buffer_printf_gettext
                    (
                        sb->footnotes,
                        /* FIXME: i18n */
                        "did you mean the %s file system type instead?",
                        ftext
                    );
                    explain_string_buffer_puts(sb->footnotes, "; ");
                    explain_string_buffer_puts(sb->footnotes, ftext);
                }
                return;
            }
        }

        if (0 == explain_buffer_errno_path_resolution(sb, errnum, source,
                     "source", &source_fc))
            return;
        if (0 == explain_buffer_errno_path_resolution(sb, errnum, target,
                     "target", &target_fc))
            return;

        /* No idea... */
        break;

    case ENOMEM:
        /*
         * The kernel could not allocate a free page to copy filenames
         * or data into.
         */
        explain_buffer_enomem_kernel(sb);
        return;

    case ENOTBLK:
        /*
         * source is not a block device (and a device was required).
         */
        if
        (
            file_system_type_needs_block_special_device(file_system_type)
        &&
            is_a_block_special_device(source) == 0
        )
        {
            explain_buffer_gettext
            (
                sb,
                "the file_system_type requires that the source must be a "
                "block special device"
            );
            return;
        }

        if
        (
            file_system_type_needs_block_special_device(file_system_type)
        &&
            source_is_in_partition_table(source) == 0
        )
        {
            explain_buffer_gettext
            (
                sb,
                "the source block special device is not present "
                "in the system partition table "
            );
            return;
        }
        explain_buffer_enotblk(sb, source, "source");
        return;

    case ENOTDIR:
        /*
         * target, or a prefix of source, is not a directory.
         */
        if (explain_buffer_errno_path_resolution(sb, errnum, source,
                "source", &source_fc))
            return;
        if (explain_buffer_errno_path_resolution(sb, errnum, target,
                "target", &target_fc))
            return;

        /* No idea... */
        break;

    case ENXIO:
        /*
         * The minor device number may refer to a non-existent partition
         * (it is usually multiplexed with the disk number)
         */
        if (source_is_in_partition_table(source) == 0)
        {
            explain_buffer_gettext
            (
                sb,
                "the source block special device is not present "
                "in the system partition table "
            );
            return;
        }

        /*
         * The major number of the block device source is out of range.
         */
        explain_buffer_gettext
        (
            sb,
            "the major number of the source block device is out of range"
        );
        if (explain_option_dialect_specific())
        {
            /*
             * FIXME: print the range (n/n), but just how the heck do we
             * find the in-range limit?
             */
            struct stat st;
            if (stat(source, &st) >= 0)
            {
                explain_string_buffer_printf(sb, " (%d)", major(st.st_rdev));
            }
        }
        return;

    case EPERM:
        /*
         * The caller does not have the required privileges.
         */
        explain_buffer_eperm_cap_sys_admin(sb, syscall_name);
        return;

    default:
        /* No idea... */
        break;
    }
    explain_buffer_errno_generic(sb, errnum, syscall_name);
}


void
explain_buffer_errno_mount(explain_string_buffer_t *sb, int errnum, const char
    *source, const char *target, const char *file_systems_type, unsigned long
    flags, const void *data)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_mount_system_call(&exp.system_call_sb, errnum, source,
        target, file_systems_type, flags, data);
    explain_buffer_errno_mount_explanation(&exp.explanation_sb, errnum, "mount",
        source, target, file_systems_type, flags, data);
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
