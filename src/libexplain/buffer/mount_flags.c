/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013, 2014 Peter Miller
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

#include <libexplain/ac/sys/mount.h>

#include <libexplain/buffer/mount_flags.h>
#include <libexplain/parse_bits.h>


static const explain_parse_bits_table_t table[] =
{
#ifdef MS_RDONLY
    { "MS_RDONLY", MS_RDONLY },
#endif
#ifdef ST_RDONLY
    { "ST_RDONLY", MT_RDONLY }, /* POSIX */
#endif
#ifdef MS_NOSUID
    { "MS_NOSUID", MS_NOSUID },
#endif
#ifdef ST_NOSUID
    { "ST_NOSUID", ST_NOSUID } /* POSIX */
#endif
#ifdef MS_NODEV
    { "MS_NODEV", MS_NODEV },
#endif
#ifdef MS_NOEXEC
    { "MS_NOEXEC", MS_NOEXEC },
#endif
#ifdef MS_SYNCHRONOUS
    { "MS_SYNCHRONOUS", MS_SYNCHRONOUS },
#endif
#ifdef MS_SYNC
    { "MS_SYNC", MS_SYNCHRONOUS }, /* histortical, ambiguous */
#endif
#ifdef MS_REMOUNT
    { "MS_REMOUNT", MS_REMOUNT },
#endif
#ifdef MS_MANDLOCK
    { "MS_MANDLOCK", MS_MANDLOCK },
#endif
#ifdef MS_DIRSYNC
    { "MS_DIRSYNC", MS_DIRSYNC },
#endif
#ifdef MS_NOATIME
    { "MS_NOATIME", MS_NOATIME },
#endif
#ifdef MS_NODIRATIME
    { "MS_NODIRATIME", MS_NODIRATIME },
#endif
#ifdef MS_BIND
    { "MS_BIND", MS_BIND },
#endif
#ifdef HAVE_SYS_MOUNT_MS_MOVE
    { "MS_MOVE", MS_MOVE },
#endif
#ifdef MS_REC
    { "MS_REC", MS_REC },
#endif
#ifdef MS_SILENT
    { "MS_SILENT", MS_SILENT },
#endif
#ifdef MS_POSIXACL
    { "MS_POSIXACL", MS_POSIXACL },
#endif
#ifdef MS_UNBINDABLE
    { "MS_UNBINDABLE", MS_UNBINDABLE },
#endif
#ifdef MS_PRIVATE
    { "MS_PRIVATE", MS_PRIVATE },
#endif
#ifdef MS_SLAVE
    { "MS_SLAVE", MS_SLAVE },
#endif
#ifdef MS_SHARED
    { "MS_SHARED", MS_SHARED },
#endif
#ifdef MS_RELATIME
    { "MS_RELATIME", MS_RELATIME },
#endif
#ifdef MS_KERNMOUNT
    { "MS_KERNMOUNT", MS_KERNMOUNT },
#endif
#ifdef MS_I_VERSION
    { "MS_I_VERSION", MS_I_VERSION },
#endif
#ifdef MS_STRICTATIME
    { "MS_STRICTATIME", MS_STRICTATIME },
#endif
#ifdef MS_NOSEC
    { "MS_NOSEC", MS_NOSEC },
#endif
#ifdef MS_BORN
    { "MS_BORN", MS_BORN },
#endif
#ifdef MS_ACTIVE
    { "MS_ACTIVE", MS_ACTIVE },
#endif
#ifdef MS_NOUSER
    { "MS_NOUSER", MS_NOUSER },
#endif
#ifdef MS_VERBOSE
    { "MS_VERBOSE", MS_VERBOSE },
#endif

#ifdef MS_MGC_VAL
    { "MS_MGC_VAL", MS_MGC_VAL },
#endif
#ifdef MS_MGC_MSK
    { "MS_MGC_MSK", MS_MGC_MSK },
#endif
#ifdef MS_RMT_MASK
    { "MS_RMT_MASK", MS_RMT_MASK },
#endif
};


void
explain_buffer_mount_flags(explain_string_buffer_t *sb, unsigned long flags)
{
    unsigned long   other;
    int             first;

    /*
     * Discard magic:
     *
     * Linux kernel 3.2 :: fs/namespace.c circa line2316
     *
     * Pre-0.97 versions of mount() didn't have a flags word.  When
     * the flags word was introduced its top half was required to have
     * the magic value 0xC0ED, and this remained so until 2.4.0-test9.
     * Therefore, if this magic number is present, it carries no
     * information and must be discarded.
     */
    first = 1;
#ifdef MS_MGC_MSK
    if ((flags & MS_MGC_MSK) == MS_MGC_VAL)
    {
        flags &= ~MS_MGC_MSK;
        explain_string_buffer_puts(sb, "MS_MGC_VAL");
        if (!flags)
            return;
        first = 0;
    }
    else
#endif
    if (flags == 0)
    {
        explain_string_buffer_putc(sb, '0');
        return;
    }

    other = 0;
    for (;;)
    {
        int             bit;
        const explain_parse_bits_table_t *tp;

        bit = (flags & -flags);
        flags &= ~bit;
        tp = explain_parse_bits_find_by_value(bit, table, SIZEOF(table));
        if (tp)
        {
            if (!first)
                explain_string_buffer_puts(sb, " | ");
            explain_string_buffer_puts(sb, tp->name);
            first = 0;
        }
        else
            other |= bit;
        if (!flags)
            break;
    }
    if (other)
    {
        /* Linux kernel sources use hex, so we will */
        if (!first)
            explain_string_buffer_puts(sb, " | ");
        explain_string_buffer_printf(sb, "0x%lX", other);
        first = 0;
    }
}


unsigned long
explain_parse_mount_flags_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, table, SIZEOF(table), caption);
}


/* vim: set ts=8 sw=4 et : */
