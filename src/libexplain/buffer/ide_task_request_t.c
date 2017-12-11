/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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

#include <libexplain/buffer/char_data.h>
#include <libexplain/buffer/ide_task_request_t.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_HDREG_H

static void
explain_buffer_ide_reg_valid_t(explain_string_buffer_t *sb,
    ide_reg_valid_t data)
{
    int comma = 0;
    if (data.b.data)
    {
        explain_string_buffer_puts(sb, "data");
        comma = 1;
    }
    if (data.b.error_feature)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "error_feature");
        comma = 1;
    }
    if (data.b.sector)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "sector");
        comma = 1;
    }
    if (data.b.nsector)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "nsector");
        comma = 1;
    }
    if (data.b.lcyl)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "lcyl");
        comma = 1;
    }
    if (data.b.hcyl)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "hcyl");
        comma = 1;
    }
    if (data.b.select)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "select");
        comma = 1;
    }
    if (data.b.status_command)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "status_command");
        comma = 1;
    }
    if (data.b.data_hob)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "data_hob");
        comma = 1;
    }
    if (data.b.error_feature_hob)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "error_feature_hob");
        comma = 1;
    }
    if (data.b.sector_hob)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "sector_hob");
        comma = 1;
    }
    if (data.b.nsector_hob)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "nsector_hob");
        comma = 1;
    }
    if (data.b.lcyl_hob)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "lcyl_hob");
        comma = 1;
    }
    if (data.b.hcyl_hob)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "hcyl_hob");
        comma = 1;
    }
    if (data.b.select_hob)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "select_hob");
        comma = 1;
    }
    if (data.b.control_hob)
    {
        if (comma)
            explain_string_buffer_puts(sb, ", ");
        explain_string_buffer_puts(sb, "control_hob");
        comma = 1;
    }
    if (!comma)
        explain_string_buffer_puts(sb, "0");
}


static void
explain_buffer_req_cmd(explain_string_buffer_t *sb, int value)
{
    static explain_parse_bits_table_t table[] =
    {
#ifdef WIN_SRST
        { "WIN_SRST", WIN_SRST },
#endif
#ifdef WIN_DEVICE_RESET
        { "WIN_DEVICE_RESET", WIN_DEVICE_RESET },
#endif
#ifdef WIN_RECAL
        { "WIN_RECAL", WIN_RECAL },
#endif
#ifdef WIN_RESTORE
        { "WIN_RESTORE", WIN_RESTORE },
#endif
#ifdef WIN_READ
        { "WIN_READ", WIN_READ },
#endif
#ifdef WIN_READ_ONCE
        { "WIN_READ_ONCE", WIN_READ_ONCE },
#endif
#ifdef WIN_READ_LONG
        { "WIN_READ_LONG", WIN_READ_LONG },
#endif
#ifdef WIN_READ_LONG_ONCE
        { "WIN_READ_LONG_ONCE", WIN_READ_LONG_ONCE },
#endif
#ifdef WIN_READ_EXT
        { "WIN_READ_EXT", WIN_READ_EXT },
#endif
#ifdef WIN_READDMA_EXT
        { "WIN_READDMA_EXT", WIN_READDMA_EXT },
#endif
#ifdef WIN_READDMA_QUEUED_EXT
        { "WIN_READDMA_QUEUED_EXT", WIN_READDMA_QUEUED_EXT },
#endif
#ifdef WIN_READ_NATIVE_MAX_EXT
        { "WIN_READ_NATIVE_MAX_EXT", WIN_READ_NATIVE_MAX_EXT },
#endif
#ifdef WIN_MULTREAD_EXT
        { "WIN_MULTREAD_EXT", WIN_MULTREAD_EXT },
#endif
#ifdef WIN_WRITE
        { "WIN_WRITE", WIN_WRITE },
#endif
#ifdef WIN_WRITE_ONCE
        { "WIN_WRITE_ONCE", WIN_WRITE_ONCE },
#endif
#ifdef WIN_WRITE_LONG
        { "WIN_WRITE_LONG", WIN_WRITE_LONG },
#endif
#ifdef WIN_WRITE_LONG_ONCE
        { "WIN_WRITE_LONG_ONCE", WIN_WRITE_LONG_ONCE },
#endif
#ifdef WIN_WRITE_EXT
        { "WIN_WRITE_EXT", WIN_WRITE_EXT },
#endif
#ifdef WIN_WRITEDMA_EXT
        { "WIN_WRITEDMA_EXT", WIN_WRITEDMA_EXT },
#endif
#ifdef WIN_WRITEDMA_QUEUED_EXT
        { "WIN_WRITEDMA_QUEUED_EXT", WIN_WRITEDMA_QUEUED_EXT },
#endif
#ifdef WIN_SET_MAX_EXT
        { "WIN_SET_MAX_EXT", WIN_SET_MAX_EXT },
#endif
#ifdef CFA_WRITE_SECT_WO_ERASE
        { "CFA_WRITE_SECT_WO_ERASE", CFA_WRITE_SECT_WO_ERASE },
#endif
#ifdef WIN_MULTWRITE_EXT
        { "WIN_MULTWRITE_EXT", WIN_MULTWRITE_EXT },
#endif
#ifdef WIN_WRITE_VERIFY
        { "WIN_WRITE_VERIFY", WIN_WRITE_VERIFY },
#endif
#ifdef WIN_VERIFY
        { "WIN_VERIFY", WIN_VERIFY },
#endif
#ifdef WIN_VERIFY_ONCE
        { "WIN_VERIFY_ONCE", WIN_VERIFY_ONCE },
#endif
#ifdef WIN_VERIFY_EXT
        { "WIN_VERIFY_EXT", WIN_VERIFY_EXT },
#endif
#ifdef WIN_FORMAT
        { "WIN_FORMAT", WIN_FORMAT },
#endif
#ifdef WIN_INIT
        { "WIN_INIT", WIN_INIT },
#endif
#ifdef WIN_SEEK
        { "WIN_SEEK", WIN_SEEK },
#endif
#ifdef CFA_TRANSLATE_SECTOR
        { "CFA_TRANSLATE_SECTOR", CFA_TRANSLATE_SECTOR },
#endif
#ifdef WIN_DIAGNOSE
        { "WIN_DIAGNOSE", WIN_DIAGNOSE },
#endif
#ifdef WIN_SPECIFY
        { "WIN_SPECIFY", WIN_SPECIFY },
#endif
#ifdef WIN_DOWNLOAD_MICROCODE
        { "WIN_DOWNLOAD_MICROCODE", WIN_DOWNLOAD_MICROCODE },
#endif
#ifdef WIN_STANDBYNOW2
        { "WIN_STANDBYNOW2", WIN_STANDBYNOW2 },
#endif
#ifdef WIN_STANDBY2
        { "WIN_STANDBY2", WIN_STANDBY2 },
#endif
#ifdef WIN_SETIDLE2
        { "WIN_SETIDLE2", WIN_SETIDLE2 },
#endif
#ifdef WIN_CHECKPOWERMODE2
        { "WIN_CHECKPOWERMODE2", WIN_CHECKPOWERMODE2 },
#endif
#ifdef WIN_SLEEPNOW2
        { "WIN_SLEEPNOW2", WIN_SLEEPNOW2 },
#endif
#ifdef WIN_PACKETCMD
        { "WIN_PACKETCMD", WIN_PACKETCMD },
#endif
#ifdef WIN_PIDENTIFY
        { "WIN_PIDENTIFY", WIN_PIDENTIFY },
#endif
#ifdef WIN_QUEUED_SERVICE
        { "WIN_QUEUED_SERVICE", WIN_QUEUED_SERVICE },
#endif
#ifdef WIN_SMART
        { "WIN_SMART", WIN_SMART },
#endif
#ifdef CFA_ERASE_SECTORS
        { "CFA_ERASE_SECTORS", CFA_ERASE_SECTORS },
#endif
#ifdef WIN_MULTREAD
        { "WIN_MULTREAD", WIN_MULTREAD },
#endif
#ifdef WIN_MULTWRITE
        { "WIN_MULTWRITE", WIN_MULTWRITE },
#endif
#ifdef WIN_SETMULT
        { "WIN_SETMULT", WIN_SETMULT },
#endif
#ifdef WIN_READDMA_QUEUED
        { "WIN_READDMA_QUEUED", WIN_READDMA_QUEUED },
#endif
#ifdef WIN_READDMA
        { "WIN_READDMA", WIN_READDMA },
#endif
#ifdef WIN_READDMA_ONCE
        { "WIN_READDMA_ONCE", WIN_READDMA_ONCE },
#endif
#ifdef WIN_WRITEDMA
        { "WIN_WRITEDMA", WIN_WRITEDMA },
#endif
#ifdef WIN_WRITEDMA_ONCE
        { "WIN_WRITEDMA_ONCE", WIN_WRITEDMA_ONCE },
#endif
#ifdef WIN_WRITEDMA_QUEUED
        { "WIN_WRITEDMA_QUEUED", WIN_WRITEDMA_QUEUED },
#endif
#ifdef CFA_WRITE_MULTI_WO_ERASE
        { "CFA_WRITE_MULTI_WO_ERASE", CFA_WRITE_MULTI_WO_ERASE },
#endif
#ifdef WIN_GETMEDIASTATUS
        { "WIN_GETMEDIASTATUS", WIN_GETMEDIASTATUS },
#endif
#ifdef WIN_ACKMEDIACHANGE
        { "WIN_ACKMEDIACHANGE", WIN_ACKMEDIACHANGE },
#endif
#ifdef WIN_POSTBOOT
        { "WIN_POSTBOOT", WIN_POSTBOOT },
#endif
#ifdef WIN_PREBOOT
        { "WIN_PREBOOT", WIN_PREBOOT },
#endif
#ifdef WIN_DOORLOCK
        { "WIN_DOORLOCK", WIN_DOORLOCK },
#endif
#ifdef WIN_DOORUNLOCK
        { "WIN_DOORUNLOCK", WIN_DOORUNLOCK },
#endif
#ifdef WIN_STANDBYNOW1
        { "WIN_STANDBYNOW1", WIN_STANDBYNOW1 },
#endif
#ifdef WIN_IDLEIMMEDIATE
        { "WIN_IDLEIMMEDIATE", WIN_IDLEIMMEDIATE },
#endif
#ifdef WIN_STANDBY
        { "WIN_STANDBY", WIN_STANDBY },
#endif
#ifdef WIN_SETIDLE1
        { "WIN_SETIDLE1", WIN_SETIDLE1 },
#endif
#ifdef WIN_READ_BUFFER
        { "WIN_READ_BUFFER", WIN_READ_BUFFER },
#endif
#ifdef WIN_CHECKPOWERMODE1
        { "WIN_CHECKPOWERMODE1", WIN_CHECKPOWERMODE1 },
#endif
#ifdef WIN_SLEEPNOW1
        { "WIN_SLEEPNOW1", WIN_SLEEPNOW1 },
#endif
#ifdef WIN_FLUSH_CACHE
        { "WIN_FLUSH_CACHE", WIN_FLUSH_CACHE },
#endif
#ifdef WIN_WRITE_BUFFER
        { "WIN_WRITE_BUFFER", WIN_WRITE_BUFFER },
#endif
#ifdef WIN_WRITE_SAME
        { "WIN_WRITE_SAME", WIN_WRITE_SAME },
#endif
#ifdef WIN_FLUSH_CACHE_EXT
        { "WIN_FLUSH_CACHE_EXT", WIN_FLUSH_CACHE_EXT },
#endif
#ifdef WIN_IDENTIFY
        { "WIN_IDENTIFY", WIN_IDENTIFY },
#endif
#ifdef WIN_MEDIAEJECT
        { "WIN_MEDIAEJECT", WIN_MEDIAEJECT },
#endif
#ifdef WIN_IDENTIFY_DMA
        { "WIN_IDENTIFY_DMA", WIN_IDENTIFY_DMA },
#endif
#ifdef WIN_SETFEATURES
        { "WIN_SETFEATURES", WIN_SETFEATURES },
#endif
#ifdef EXABYTE_ENABLE_NEST
        { "EXABYTE_ENABLE_NEST", EXABYTE_ENABLE_NEST },
#endif
#ifdef WIN_SECURITY_SET_PASS
        { "WIN_SECURITY_SET_PASS", WIN_SECURITY_SET_PASS },
#endif
#ifdef WIN_SECURITY_UNLOCK
        { "WIN_SECURITY_UNLOCK", WIN_SECURITY_UNLOCK },
#endif
#ifdef WIN_SECURITY_ERASE_PREPARE
        { "WIN_SECURITY_ERASE_PREPARE", WIN_SECURITY_ERASE_PREPARE },
#endif
#ifdef WIN_SECURITY_ERASE_UNIT
        { "WIN_SECURITY_ERASE_UNIT", WIN_SECURITY_ERASE_UNIT },
#endif
#ifdef WIN_SECURITY_FREEZE_LOCK
        { "WIN_SECURITY_FREEZE_LOCK", WIN_SECURITY_FREEZE_LOCK },
#endif
#ifdef WIN_SECURITY_DISABLE
        { "WIN_SECURITY_DISABLE", WIN_SECURITY_DISABLE },
#endif
#ifdef WIN_READ_NATIVE_MAX
        { "WIN_READ_NATIVE_MAX", WIN_READ_NATIVE_MAX },
#endif
#ifdef WIN_SET_MAX
        { "WIN_SET_MAX", WIN_SET_MAX },
#endif
#ifdef DISABLE_SEAGATE
        { "DISABLE_SEAGATE", DISABLE_SEAGATE },
#endif
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_ide_task_request_t(explain_string_buffer_t *sb,
    const ide_task_request_t *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ io_ports = ");
    explain_buffer_char_data(sb, data->io_ports, sizeof(data->io_ports));
    explain_string_buffer_puts(sb, ", hob_ports = ");
    explain_buffer_char_data(sb, data->hob_ports, sizeof(data->hob_ports));
    explain_string_buffer_puts(sb, ", out_flags = ");
    explain_buffer_ide_reg_valid_t(sb, data->out_flags);
    explain_string_buffer_puts(sb, ", in_flags = ");
    explain_buffer_ide_reg_valid_t(sb, data->in_flags);
    explain_string_buffer_puts(sb, ", data_phase = ");
    explain_buffer_int(sb, data->data_phase);
    explain_string_buffer_puts(sb, ", req_cmd = ");
    explain_buffer_req_cmd(sb, data->req_cmd);
    explain_string_buffer_puts(sb, ", out_size = ");
    explain_buffer_ulong(sb, data->out_size);
    explain_string_buffer_puts(sb, ", in_size = ");
    explain_buffer_ulong(sb, data->in_size);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_ide_task_request_t(explain_string_buffer_t *sb, const void *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
