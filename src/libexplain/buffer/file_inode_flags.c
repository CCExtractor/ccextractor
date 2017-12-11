/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#include <libexplain/ac/linux/fs.h>

#include <libexplain/buffer/file_inode_flags.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#if defined(FS_IOC_GETFLAGS) || defined(FS_IOC32_GETFLAGS)

void
explain_buffer_file_inode_flags(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "FS_SECRM_FL", FS_SECRM_FL },
        { "FS_UNRM_FL", FS_UNRM_FL },
        { "FS_COMPR_FL", FS_COMPR_FL },
        { "FS_SYNC_FL", FS_SYNC_FL },
        { "FS_IMMUTABLE_FL", FS_IMMUTABLE_FL },
        { "FS_APPEND_FL", FS_APPEND_FL },
        { "FS_NODUMP_FL", FS_NODUMP_FL },
        { "FS_NOATIME_FL", FS_NOATIME_FL },
        { "FS_DIRTY_FL", FS_DIRTY_FL },
        { "FS_COMPRBLK_FL", FS_COMPRBLK_FL },
        { "FS_NOCOMP_FL", FS_NOCOMP_FL },
        { "FS_ECOMPR_FL", FS_ECOMPR_FL },
        { "FS_BTREE_FL", FS_BTREE_FL },
        { "FS_INDEX_FL", FS_INDEX_FL },
        { "FS_IMAGIC_FL", FS_IMAGIC_FL },
        { "FS_JOURNAL_DATA_FL", FS_JOURNAL_DATA_FL },
        { "FS_NOTAIL_FL", FS_NOTAIL_FL },
        { "FS_DIRSYNC_FL", FS_DIRSYNC_FL },
        { "FS_TOPDIR_FL", FS_TOPDIR_FL },
        { "FS_EXTENT_FL", FS_EXTENT_FL },
        { "FS_DIRECTIO_FL", FS_DIRECTIO_FL },
        { "FS_RESERVED_FL", FS_RESERVED_FL },
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}

#else

void
explain_buffer_file_inode_flags(explain_string_buffer_t *sb, int value)
{
    explain_buffer_int(sb, value);
}

#endif


void
explain_buffer_file_inode_flags_int_star(explain_string_buffer_t *sb,
    const int *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    explain_buffer_file_inode_flags(sb, *data);
    explain_string_buffer_puts(sb, " }");
}


void
explain_buffer_file_inode_flags_long_star(explain_string_buffer_t *sb,
    const long *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    explain_buffer_file_inode_flags(sb, *data);
    explain_string_buffer_puts(sb, " }");
}


/* vim: set ts=8 sw=4 et : */
