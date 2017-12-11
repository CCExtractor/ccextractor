/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#include <libexplain/ac/sys/shm.h>
#include <libexplain/ac/sys/stat.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/gid.h>
#include <libexplain/buffer/group_permission_ignored.h>
#include <libexplain/buffer/others_permission.h>
#include <libexplain/buffer/others_permission_ignored.h>
#include <libexplain/buffer/rwx.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/capability.h>
#include <libexplain/gettext.h>
#include <libexplain/option.h>
#include <libexplain/translate.h>


static void
owner_permission_mode_used(explain_string_buffer_t *sb,
    const struct ipc_perm *perm, uid_t proc_uid, const char *proc_uid_caption,
    const char *whatsit, const char *ipc_uid_caption)
{
    char            part2[40];
    explain_string_buffer_t part2_sb;
    char            part4[8];
    explain_string_buffer_t part4_sb;

    explain_string_buffer_init(&part2_sb, part2, sizeof(part2));
    explain_buffer_uid(&part2_sb, proc_uid);
    explain_string_buffer_init(&part4_sb, part4, sizeof(part4));
    explain_buffer_rwx(&part4_sb, perm->mode & S_IRWXU);

    explain_string_buffer_puts(sb, ", ");
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining which permission mode
         * bits are used when determining IPC access permissions.
         *
         * %1$s => the kind of process UID, "real UID" or "effective UID",
         *         already translated.
         * %2$s => the UID of the process, number and name.
         * %3$s => the kind if IPC object, e.g. "shared memory segment",
         *         already translated
         * %4$s => the kind of IPC object UID, "owner UID" or
         *         "creator UID", already translated.
         * %5$s => The mode bits like "rwx", including the quotes, in
         *         a form resembling the ls -l representation of mode
         *         bits.
         */
        i18n("the process %s %s matches the %s %s and the "
            "owner permission mode is %s"),
        proc_uid_caption,
        part2,
        whatsit,
        ipc_uid_caption,
        part4
    );
}


static void
owner_permission_mode_mismatch(explain_string_buffer_t *sb,
    const struct ipc_perm *perm, int proc_uid, const char *proc_uid_caption,
    const char *whatsit, int shm_uid, const char *shm_uid_caption)
{
    char            part1[40];
    explain_string_buffer_t part1_sb;
    char            part2[40];
    explain_string_buffer_t part2_sb;
    char            part3[8];
    explain_string_buffer_t part3_sb;

    explain_string_buffer_init(&part1_sb, part1, sizeof(part1));
    explain_buffer_uid(&part1_sb, proc_uid);
    explain_string_buffer_init(&part2_sb, part2, sizeof(part2));
    explain_buffer_uid(&part2_sb, shm_uid);
    explain_string_buffer_init(&part3_sb, part3, sizeof(part3));
    explain_buffer_rwx(&part3_sb, perm->mode & S_IRWXU);

    explain_string_buffer_puts(sb, ", ");
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining which owner permission
         * mode bits are ignored when determining IPC access permissions.
         *
         * %1$s => the kind of process UID, "real UID" or "effective UID",
         *         already translated
         * %2$s => the UID of the process, number and name.
         * %3$s => the kind of IPC object, e.g. "shared memory segment",
         *         already tramslated
         * %4$s => the kind of shm UID, "owner UID" or "creator UID"
         * %5$s => the shm UID of the IPC object, number and name.
         * %6$s => The mode bits like "rwx", including the quotes, in
         *         a form resembling the ls -l representation of mode
         *         bits.
         */
        i18n("the process %s %s does not match the %s %s %s "
            "so the owner permission mode %s is ignored"),
        proc_uid_caption,
        part1,
        whatsit,
        shm_uid_caption,
        part2,
        part3
    );
}


static void
group_permission_mode_used(explain_string_buffer_t *sb,
    const struct ipc_perm *perm, int proc_gid, const char *proc_gid_caption,
    const char *whatsit, const char *ipc_gid_caption)
{
    char            part1[40];
    explain_string_buffer_t part1_sb;
    char            part3[8];
    explain_string_buffer_t part3_sb;

    explain_string_buffer_init(&part1_sb, part1, sizeof(part1));
    explain_buffer_gid(&part1_sb, proc_gid);
    explain_string_buffer_init(&part3_sb, part3, sizeof(part3));
    explain_buffer_rwx(&part3_sb, perm->mode & S_IRWXG);

    explain_string_buffer_puts(sb, ", ");
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining which permission
         * mode bits are used when determining IPC access permissions.
         *
         * This is explicitly nothing to do with files.
         *
         * %1$s => the process kind of GID, "real GID" or "effective GID",
         *         already translated
         * %2$s => the GID of the process, number and name.
         * %3$s => the kinf of IPC object, e.g. "shared memory segment",
         *         already translated
         * %4$s => the IPC object kind of GID, "owner GID" or "creator GID",
         *         already translated
         * %5$s => the GID of the IPC object, number and name.
         * %6$s => The mode bits like "rwx", including the quotes, in
         *         a form resembling the ls -l representation of mode
         *         bits.
         */
        i18n("the process %s %s matches the %s %s and "
            "the group permission mode is %s"),
        proc_gid_caption,
        part1,
        whatsit,
        ipc_gid_caption,
        part3
    );
}


static void
group_permission_mode_mismatch(explain_string_buffer_t *sb,
    const struct ipc_perm *perm, int proc_gid, const char *proc_gid_caption,
    const char *whatsit, int ipc_gid, const char *ipc_gid_caption)
{
    char            part1[40];
    explain_string_buffer_t part1_sb;
    char            part2[40];
    explain_string_buffer_t part2_sb;
    char            part3[8];
    explain_string_buffer_t part3_sb;

    explain_string_buffer_init(&part1_sb, part1, sizeof(part1));
    explain_buffer_gid(&part1_sb, proc_gid);
    explain_string_buffer_init(&part2_sb, part2, sizeof(part2));
    explain_buffer_gid(&part2_sb, ipc_gid);
    explain_string_buffer_init(&part3_sb, part3, sizeof(part3));
    explain_buffer_rwx(&part3_sb, perm->mode & S_IRWXG);

    explain_string_buffer_puts(sb, ", ");
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when explaining which permission
         * mode bits are used when determining IPC access permissions.
         *
         * This is explicitly nothing to do with files.
         *
         * %1$s => the process kind of GID, "real GID" or "effective GID",
         *         already translated
         * %2$s => the GID of the process, number and name.
         * %3$s => the IPOC onject kind, e.g. "shared memory segment",
         *         already trsmalated
         * %4$s => the IPC object kind of GID, "owner GID" or "creator GID",
         *         already translated
         * %5$s => the GID of the IPC object, number and name.
         * %6$s => The mode bits like "rwx", including the quotes, in
         *         a form resembling the ls -l representation of mode
         *         bits.
         */
        i18n("the process %s %s does not match the %s %s %s "
            "and so the group permission mode %s is ignored"),
        proc_gid_caption,
        part1,
        whatsit,
        ipc_gid_caption,
        part2,
        part3
    );
}


static const char *
translate_owner_uid(void)
{
    return
        explain_gettext
        (
            /*
             * xgettext: this phrase it used to describe an aspect of IPC
             * object, the owner user id (see struct ipc_perm member uid, or
             * shmat(2) for more information).
             *
             * This is explicitly nothing to do with files.
             */
            i18n("owner UID")
        );
}


static const char *
translate_creator_uid(void)
{
    return
        explain_gettext
        (
            /*
             * xgettext: this phrase it used to describe an aspect of IPC
             * object, the creator user id (see struct ipc_perm member cuid, or
             * shmat(2) for more information).
             *
             * This is explicitly nothing to do with files.
             */
            i18n("creator UID")
        );
}


static const char *
translate_owner_gid(void)
{
    return
        explain_gettext
        (
            /*
             * xgettext: this phrase it used to describe an aspect of
             * IPC object, their owner group id (see ipc_perm::gid, or
             * shmat(2) for more information).
             *
             * This is explicitly nothing to do with files.
             */
            i18n("owner GID")
        );
}


static const char *
translate_creator_gid(void)
{
    return
        explain_gettext
        (
            /*
             * xgettext: this phrase it used to describe an aspect of
             * IPC object, their creator group id (see ipc_perm::cgid, or
             * shmat(2) for more information).
             *
             * This is explicitly nothing to do with files.
             */
            i18n("creator GID")
        );
}


static void
no_ipc_permission(explain_string_buffer_t *sb, const char *whatsit)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This error message is used to explain that a
         * process does not have the necessary IPC permissions to access
         * the resource it requested.
         *
         * %1$s => The kind of resource, e.g. "shared memory segment" or
         *         "semaphore", etc.  Already translated.
         *
         */
        i18n("the process does not have the necessary %s access permissions"),
        whatsit
    );
}


static int
buffer_eacces_shm(explain_string_buffer_t *sb, const struct ipc_perm *perm,
    int read_only, const char *whatsit)
{
    uid_t           proc_uid;
    const char      *proc_uid_caption;
    gid_t           proc_gid;
    const char      *proc_gid_caption;
    uid_t           ipc_uid;
    const char      *ipc_uid_caption;
    gid_t           ipc_gid;
    const char      *ipc_gid_caption;
    unsigned        mode;

    mode = read_only ? 0444 : 0666;

    if (getuid() == perm->uid)
    {
        proc_uid = getuid();
        proc_uid_caption = explain_translate_real_uid();
        ipc_uid = perm->uid;
        ipc_uid_caption = translate_owner_uid();
    }
    else if (getuid() == perm->cuid)
    {
        proc_uid = getuid();
        proc_uid_caption = explain_translate_real_uid();
        ipc_uid = perm->cuid;
        ipc_uid_caption = translate_creator_uid();
    }
    else if (geteuid() == perm->uid)
    {
        proc_uid = geteuid();
        proc_uid_caption = explain_translate_effective_uid();
        ipc_uid = perm->uid;
        ipc_uid_caption = translate_owner_uid();
    }
    else if (geteuid() == perm->cuid)
    {
        proc_uid = geteuid();
        proc_uid_caption = explain_translate_effective_uid();
        ipc_uid = perm->cuid;
        ipc_uid_caption = translate_creator_uid();
    }
    else
    {
        proc_uid = geteuid();
        proc_uid_caption = explain_translate_effective_uid();
        ipc_uid = perm->uid;
        ipc_uid_caption = translate_owner_uid();
    }

    if (getgid() == perm->gid)
    {
        proc_gid = getgid();
        proc_gid_caption = explain_translate_real_gid();
        ipc_gid = perm->gid;
        ipc_gid_caption = translate_owner_gid();
    }
    else if (getgid() == perm->cgid)
    {
        proc_gid = getgid();
        proc_gid_caption = explain_translate_real_gid();
        ipc_gid = perm->cgid;
        ipc_gid_caption = translate_creator_gid();
    }
    else if (getegid() == perm->gid)
    {
        proc_gid = getegid();
        proc_gid_caption = explain_translate_effective_gid();
        ipc_gid = perm->gid;
        ipc_gid_caption = translate_owner_gid();
    }
    else if (getegid() == perm->cgid)
    {
        proc_gid = getegid();
        proc_gid_caption = explain_translate_effective_gid();
        ipc_gid = perm->cgid;
        ipc_gid_caption = translate_creator_gid();
    }
    else
    {
        proc_gid = getegid();
        proc_gid_caption = explain_translate_effective_gid();
        ipc_gid = perm->gid;
        ipc_gid_caption = translate_owner_gid();
    }

    /*
     * First, see if there is a permissions problem at all.
     */
    if (explain_capability_ipc_owner())
        return 0;
    if (proc_uid == ipc_uid)
    {
        int mode2 = mode & S_IRWXU;
        if ((perm->mode & mode2) == mode2)
            return 0;
    }
    else if (proc_gid == ipc_gid)
    {
        int mode2 = mode & S_IRWXG;
        if ((perm->mode & mode2) == mode2)
            return 0;
    }
    else
    {
        int mode2 = mode & S_IRWXO;
        if ((perm->mode & mode2) == mode2)
            return 0;
    }

    /*
     * At this point we know something broke.
     */
    no_ipc_permission(sb, whatsit);
    if (explain_option_dialect_specific())
    {
        if (proc_uid == ipc_uid)
        {
            owner_permission_mode_used(sb, perm, proc_uid, proc_uid_caption,
                whatsit, ipc_uid_caption);
            explain_buffer_group_permission_ignored(sb, perm->mode);
            explain_buffer_others_permission_ignored(sb, perm->mode);
        }
        else
        {
            owner_permission_mode_mismatch
            (
                sb,
                perm,
                proc_uid,
                proc_uid_caption,
                whatsit,
                ipc_uid,
                ipc_uid_caption
            );
            if (proc_gid == ipc_gid)
            {
                group_permission_mode_used
                (
                    sb,
                    perm,
                    proc_gid,
                    proc_gid_caption,
                    whatsit,
                    ipc_gid_caption
                );
                explain_buffer_others_permission_ignored(sb, perm->mode);
            }
            else
            {
                group_permission_mode_mismatch
                (
                    sb,
                    perm,
                    proc_gid,
                    proc_gid_caption,
                    whatsit,
                    ipc_gid,
                    ipc_gid_caption
                );
                explain_buffer_others_permission(sb, perm->mode);
            }
        }
    }
    explain_buffer_dac_ipc_owner(sb);

    /*
     * The return value says we printed an explanation.
     */
    return 1;
}


int
explain_buffer_eacces_shmat(explain_string_buffer_t *sb,
    const struct ipc_perm *perm, int read_only)
{
    return
        buffer_eacces_shm
        (
            sb,
            perm,
            read_only,
            explain_translate_shared_memory_segment()
        );
}


void
explain_buffer_eacces_shmat_vague(explain_string_buffer_t *sb)
{
    no_ipc_permission(sb, explain_translate_shared_memory_segment());
    explain_buffer_dac_ipc_owner(sb);
}


/* vim: set ts=8 sw=4 et : */
