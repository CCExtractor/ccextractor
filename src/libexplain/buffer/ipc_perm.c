/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011-2013 Peter Miller
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

#include <libexplain/buffer/gid.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/ipc_perm.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/short.h>
#include <libexplain/buffer/uid.h>
#include <libexplain/is_efault.h>
#include <libexplain/option.h>
#include <libexplain/parse_bits.h>


#ifdef HAVE_SYS_SHM_H

static void
explain_buffer_ipc_perm_mode(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "S_IRWXU", S_IRWXU },
        { "S_IRUSR", S_IRUSR },
        { "S_IWUSR", S_IWUSR },
        { "S_IXUSR", S_IXUSR },
        { "S_IRWXG", S_IRWXG },
        { "S_IRGRP", S_IRGRP },
        { "S_IWGRP", S_IWGRP },
        { "S_IXGRP", S_IXGRP },
        { "S_IRWXO", S_IRWXO },
        { "S_IROTH", S_IROTH },
        { "S_IWOTH", S_IWOTH },
        { "S_IXOTH", S_IXOTH },
#ifdef SHM_DEST
        { "SHM_DEST", SHM_DEST },
#endif
#ifdef SHM_LOCKED
        { "SHM_LOCKED", SHM_LOCKED },
#endif
    };

    if (explain_option_symbolic_mode_bits())
    {
        explain_parse_bits_print(sb, value, table, SIZEOF(table));
    }
    else
    {
        int             lo;

        lo = value & 0777;
        value &= ~0777;
        if (value)
        {
            explain_parse_bits_print(sb, value, table, SIZEOF(table));
            if (lo)
                explain_string_buffer_printf(sb, " | %#o", lo);
        }
        else
        {
            explain_string_buffer_printf(sb, "%#o", lo);
        }
    }
}

#endif

void
explain_buffer_ipc_perm(explain_string_buffer_t *sb,
    const struct ipc_perm *data, int extra)
{
#ifdef HAVE_SYS_SHM_H
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    if (extra)
    {
#ifdef SYS_SHM_H_struct_ipc_perm_underscore_key
        explain_string_buffer_puts(sb, "__key = ");
        explain_buffer_int(sb, data->__key);
#else
        explain_string_buffer_puts(sb, "key = ");
        explain_buffer_int(sb, data->key);
#endif
        explain_string_buffer_puts(sb, ", ");
    }
    explain_string_buffer_puts(sb, "uid = ");
    explain_buffer_uid(sb, data->uid);
    explain_string_buffer_puts(sb, ", gid = ");
    explain_buffer_gid(sb, data->gid);
    if (extra)
    {
        explain_string_buffer_puts(sb, ", cuid = ");
        explain_buffer_uid(sb, data->cuid);
        explain_string_buffer_puts(sb, ", cgid = ");
        explain_buffer_gid(sb, data->cgid);
    }
    explain_string_buffer_puts(sb, ", mode = ");
    explain_buffer_ipc_perm_mode(sb, data->mode);
    if (extra)
    {
#ifdef SYS_SHM_H_struct_ipc_perm_underscore_key
        explain_string_buffer_puts(sb, ", __seq = ");
        explain_buffer_ushort(sb, data->__seq);
#else
        explain_string_buffer_puts(sb, ", seq = ");
        explain_buffer_ushort(sb, data->seq);
#endif
    }
    explain_string_buffer_puts(sb, " }");
#else
    (void)extra;
    explain_buffer_pointer(sb, data);
#endif
}


/* vim: set ts=8 sw=4 et : */
