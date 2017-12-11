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

#include <libexplain/ac/linux/cdrom.h>

#include <libexplain/buffer/cdrom_generic_command.h>
#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef CDROM_SEND_PACKET

static void
explain_buffer_data_direction(explain_string_buffer_t *sb, int dir)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "CGC_DATA_UNKNOWN", CGC_DATA_UNKNOWN },
        { "CGC_DATA_WRITE", CGC_DATA_WRITE },
        { "CGC_DATA_READ", CGC_DATA_READ },
        { "CGC_DATA_NONE", CGC_DATA_NONE },
    };

    explain_parse_bits_print_single(sb, dir, table, SIZEOF(table));
}


static void
explain_buffer_gpcmd(explain_string_buffer_t *sb, int cmd)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "GPCMD_BLANK", GPCMD_BLANK },
        { "GPCMD_CLOSE_TRACK", GPCMD_CLOSE_TRACK },
        { "GPCMD_FLUSH_CACHE", GPCMD_FLUSH_CACHE },
        { "GPCMD_FORMAT_UNIT", GPCMD_FORMAT_UNIT },
        { "GPCMD_GET_CONFIGURATION", GPCMD_GET_CONFIGURATION },
        { "GPCMD_GET_EVENT_STATUS_NOTIFICATION",
            GPCMD_GET_EVENT_STATUS_NOTIFICATION },
        { "GPCMD_GET_PERFORMANCE", GPCMD_GET_PERFORMANCE },
        { "GPCMD_INQUIRY", GPCMD_INQUIRY },
        { "GPCMD_LOAD_UNLOAD", GPCMD_LOAD_UNLOAD },
        { "GPCMD_MECHANISM_STATUS", GPCMD_MECHANISM_STATUS },
        { "GPCMD_MODE_SELECT_10", GPCMD_MODE_SELECT_10 },
        { "GPCMD_MODE_SENSE_10", GPCMD_MODE_SENSE_10 },
        { "GPCMD_PAUSE_RESUME", GPCMD_PAUSE_RESUME },
        { "GPCMD_PLAY_AUDIO_10", GPCMD_PLAY_AUDIO_10 },
        { "GPCMD_PLAY_AUDIO_MSF", GPCMD_PLAY_AUDIO_MSF },
        { "GPCMD_PLAY_AUDIO_TI", GPCMD_PLAY_AUDIO_TI },
        { "GPCMD_PLAY_CD", GPCMD_PLAY_CD },
        { "GPCMD_PREVENT_ALLOW_MEDIUM_REMOVAL",
            GPCMD_PREVENT_ALLOW_MEDIUM_REMOVAL },
        { "GPCMD_READ_10", GPCMD_READ_10 },
        { "GPCMD_READ_12", GPCMD_READ_12 },
#ifdef GPCMD_READ_BUFFER
        { "GPCMD_READ_BUFFER", GPCMD_READ_BUFFER },
#endif
        { "GPCMD_READ_BUFFER_CAPACITY", GPCMD_READ_BUFFER_CAPACITY },
        { "GPCMD_READ_CDVD_CAPACITY", GPCMD_READ_CDVD_CAPACITY },
        { "GPCMD_READ_CD", GPCMD_READ_CD },
        { "GPCMD_READ_CD_MSF", GPCMD_READ_CD_MSF },
        { "GPCMD_READ_DISC_INFO", GPCMD_READ_DISC_INFO },
        { "GPCMD_READ_DVD_STRUCTURE", GPCMD_READ_DVD_STRUCTURE },
        { "GPCMD_READ_FORMAT_CAPACITIES", GPCMD_READ_FORMAT_CAPACITIES },
        { "GPCMD_READ_HEADER", GPCMD_READ_HEADER },
        { "GPCMD_READ_TRACK_RZONE_INFO", GPCMD_READ_TRACK_RZONE_INFO },
        { "GPCMD_READ_SUBCHANNEL", GPCMD_READ_SUBCHANNEL },
        { "GPCMD_READ_TOC_PMA_ATIP", GPCMD_READ_TOC_PMA_ATIP },
        { "GPCMD_REPAIR_RZONE_TRACK", GPCMD_REPAIR_RZONE_TRACK },
        { "GPCMD_REPORT_KEY", GPCMD_REPORT_KEY },
        { "GPCMD_REQUEST_SENSE", GPCMD_REQUEST_SENSE },
        { "GPCMD_RESERVE_RZONE_TRACK", GPCMD_RESERVE_RZONE_TRACK },
        { "GPCMD_SEND_CUE_SHEET", GPCMD_SEND_CUE_SHEET },
        { "GPCMD_SCAN", GPCMD_SCAN },
        { "GPCMD_SEEK", GPCMD_SEEK },
        { "GPCMD_SEND_DVD_STRUCTURE", GPCMD_SEND_DVD_STRUCTURE },
        { "GPCMD_SEND_EVENT", GPCMD_SEND_EVENT },
        { "GPCMD_SEND_KEY", GPCMD_SEND_KEY },
        { "GPCMD_SEND_OPC", GPCMD_SEND_OPC },
        { "GPCMD_SET_READ_AHEAD", GPCMD_SET_READ_AHEAD },
        { "GPCMD_SET_STREAMING", GPCMD_SET_STREAMING },
        { "GPCMD_START_STOP_UNIT", GPCMD_START_STOP_UNIT },
        { "GPCMD_STOP_PLAY_SCAN", GPCMD_STOP_PLAY_SCAN },
        { "GPCMD_TEST_UNIT_READY", GPCMD_TEST_UNIT_READY },
        { "GPCMD_VERIFY_10", GPCMD_VERIFY_10 },
        { "GPCMD_WRITE_10", GPCMD_WRITE_10 },
#ifdef GPCMD_WRITE_12
        { "GPCMD_WRITE_12", GPCMD_WRITE_12 },
#endif
        { "GPCMD_WRITE_AND_VERIFY_10", GPCMD_WRITE_AND_VERIFY_10 },
#ifdef GPCMD_WRITE_BUFFER
        { "GPCMD_WRITE_BUFFER", GPCMD_WRITE_BUFFER },
#endif
        { "GPCMD_SET_SPEED", GPCMD_SET_SPEED },
        { "GPCMD_PLAYAUDIO_TI", GPCMD_PLAYAUDIO_TI },
        { "GPCMD_GET_MEDIA_STATUS", GPCMD_GET_MEDIA_STATUS },
    };

    explain_parse_bits_print_single(sb, cmd, table, SIZEOF(table));
}


void
explain_buffer_cdrom_generic_command(explain_string_buffer_t *sb,
    const struct cdrom_generic_command *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ cmd = { ");
    explain_buffer_gpcmd(sb, data->cmd[0]);
    explain_string_buffer_putc(sb, ',');
    explain_buffer_hexdump(sb, data->cmd + 1, sizeof(data->cmd) - 1);
    explain_string_buffer_puts(sb, "}, buffer = ");
    explain_buffer_pointer(sb, data->buffer);
    explain_string_buffer_puts(sb, ", buflen = ");
    explain_buffer_long(sb, data->buflen);
    explain_string_buffer_puts(sb, ", stat = ");
    explain_buffer_long(sb, data->stat);
    explain_string_buffer_puts(sb, ", sense = ");
    explain_buffer_pointer(sb, data->sense);
    explain_string_buffer_puts(sb, ", data_direction = ");
    explain_buffer_data_direction(sb, data->data_direction);
    explain_string_buffer_puts(sb, ", quiet = ");
    explain_buffer_long(sb, data->quiet);
    explain_string_buffer_puts(sb, ", timeout = ");
    explain_buffer_long(sb, data->timeout);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_cdrom_generic_command(explain_string_buffer_t *sb,
    const struct cdrom_generic_command *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
