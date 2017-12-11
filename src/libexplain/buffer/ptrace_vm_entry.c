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

#include <libexplain/ac/sys/ptrace.h>

#include <libexplain/buffer/char_data.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/ptrace_vm_entry.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/long.h>
#include <libexplain/is_efault.h>


void
explain_buffer_ptrace_vm_entry(explain_string_buffer_t *sb,
    const struct ptrace_vm_entry *data)
{
#ifdef PTRACE_VM_ENTRY
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ pve_entry = ");
    explain_buffer_int(sb, data->pve_entry);
    explain_string_buffer_puts(sb, ", pve_timestamp = ");
    explain_buffer_int(sb, data->pve_timestamp);
    explain_string_buffer_puts(sb, ", pve_start = ");
    explain_buffer_ulong(sb, data->pve_start);
    explain_string_buffer_puts(sb, ", pve_end = ");
    explain_buffer_ulong(sb, data->pve_end);
    explain_string_buffer_puts(sb, ", pve_offset = ");
    explain_buffer_ulong(sb, data->pve_offset);
    explain_string_buffer_puts(sb, ", pve_prot = ");
    explain_buffer_uint(sb, data->pve_prot);
    explain_string_buffer_puts(sb, ", pve_pathlen = ");
    explain_buffer_uint(sb, data->pve_pathlen);
    explain_string_buffer_puts(sb, ", pve_fileid = ");
    explain_buffer_long(sb, data->pve_fileid);
    explain_string_buffer_puts(sb, ", pve_fsid = ");
    explain_buffer_uint32_t(sb, data->pve_fsid);
    explain_string_buffer_puts(sb, ", pve_path = ");
    explain_buffer_char_data_quoted(sb, data->pve_path, data->pve_pathlen);
    explain_string_buffer_puts(sb, " }");
#else
    explain_buffer_pointer(sb, data);
#endif
}


/* vim: set ts=8 sw=4 et : */
