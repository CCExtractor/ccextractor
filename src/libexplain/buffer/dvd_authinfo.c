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

#include <libexplain/ac/linux/cdrom.h>

#include <libexplain/buffer/dvd_authinfo.h>
#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_CDROM_H

static void
explain_buffer_dvd_authinfo_type(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "DVD_LU_SEND_AGID", DVD_LU_SEND_AGID },
        { "DVD_HOST_SEND_CHALLENGE", DVD_HOST_SEND_CHALLENGE },
        { "DVD_LU_SEND_KEY1", DVD_LU_SEND_KEY1 },
        { "DVD_LU_SEND_CHALLENGE", DVD_LU_SEND_CHALLENGE },
        { "DVD_HOST_SEND_KEY2", DVD_HOST_SEND_KEY2 },
        { "DVD_AUTH_ESTABLISHED", DVD_AUTH_ESTABLISHED },
        { "DVD_AUTH_FAILURE", DVD_AUTH_FAILURE },
        { "DVD_LU_SEND_TITLE_KEY", DVD_LU_SEND_TITLE_KEY },
        { "DVD_LU_SEND_ASF", DVD_LU_SEND_ASF },
        { "DVD_INVALIDATE_AGID", DVD_INVALIDATE_AGID },
        { "DVD_LU_SEND_RPC_STATE", DVD_LU_SEND_RPC_STATE },
        { "DVD_HOST_SEND_RPC_STATE", DVD_HOST_SEND_RPC_STATE },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_dvd_authinfo(explain_string_buffer_t *sb,
    const dvd_authinfo *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_dvd_authinfo_type(sb, data->type);
    switch (data->type)
    {
    case DVD_LU_SEND_AGID:
        explain_string_buffer_puts(sb, ", agid = ");
        explain_buffer_int(sb, data->lsa.agid);
        break;

    case DVD_HOST_SEND_CHALLENGE:
        explain_string_buffer_puts(sb, ", agid = ");
        explain_buffer_int(sb, data->hsc.agid);
        explain_string_buffer_puts(sb, ", chal = ");
        explain_buffer_hexdump(sb, data->hsc.chal, sizeof(data->hsc.chal));
        break;

    case DVD_LU_SEND_KEY1:
        explain_string_buffer_puts(sb, ", agid = ");
        explain_buffer_int(sb, data->lsk.agid);
        explain_string_buffer_puts(sb, ", chal = ");
        explain_buffer_hexdump(sb, data->lsk.key, sizeof(data->lsk.key));
        break;

    case DVD_LU_SEND_CHALLENGE:
        explain_string_buffer_puts(sb, ", agid = ");
        explain_buffer_int(sb, data->lsc.agid);
        explain_string_buffer_puts(sb, ", chal = ");
        explain_buffer_hexdump(sb, data->lsc.chal, sizeof(data->lsc.chal));
        break;

    case DVD_HOST_SEND_KEY2:
        explain_string_buffer_puts(sb, ", agid = ");
        explain_buffer_int(sb, data->hsk.agid);
        explain_string_buffer_puts(sb, ", chal = ");
        explain_buffer_hexdump(sb, data->hsk.key, sizeof(data->hsk.key));
        break;

    case DVD_AUTH_ESTABLISHED:
        break;

    case DVD_AUTH_FAILURE:
        break;

    case DVD_LU_SEND_TITLE_KEY:
        explain_string_buffer_puts(sb, ", agid = ");
        explain_buffer_int(sb, data->lstk.agid);
        explain_string_buffer_puts(sb, ", title_key = ");
        explain_buffer_hexdump
        (
            sb,
            data->lstk.title_key,
            sizeof(data->lstk.title_key)
        );
        explain_string_buffer_puts(sb, ", lba = ");
        explain_buffer_int(sb, data->lstk.lba);
        explain_string_buffer_puts(sb, ", cpm = ");
        explain_buffer_int(sb, data->lstk.cpm);
        explain_string_buffer_puts(sb, ", cp_sec = ");
        explain_buffer_int(sb, data->lstk.cp_sec);
        explain_string_buffer_puts(sb, ", cgms = ");
        explain_buffer_int(sb, data->lstk.cgms);
        break;

    case DVD_LU_SEND_ASF:
        explain_string_buffer_puts(sb, ", agid = ");
        explain_buffer_int(sb, data->lsasf.agid);
        explain_string_buffer_puts(sb, ", asf = ");
        explain_buffer_int(sb, data->lsasf.asf);
        break;

    case DVD_INVALIDATE_AGID:
        break;

    case DVD_LU_SEND_RPC_STATE:
        explain_string_buffer_puts(sb, ", type = ");
        explain_buffer_int(sb, data->lrpcs.type);
        explain_string_buffer_puts(sb, ", vra = ");
        explain_buffer_int(sb, data->lrpcs.vra);
        explain_string_buffer_puts(sb, ", ucca = ");
        explain_buffer_int(sb, data->lrpcs.ucca);
        explain_string_buffer_puts(sb, ", region_mask = ");
        explain_buffer_int(sb, data->lrpcs.region_mask);
        explain_string_buffer_puts(sb, ", rpc_scheme = ");
        explain_buffer_int(sb, data->lrpcs.rpc_scheme);
        break;

    case DVD_HOST_SEND_RPC_STATE:
        explain_string_buffer_puts(sb, ", type = ");
        explain_buffer_int(sb, data->hrpcs.type);
        explain_string_buffer_puts(sb, ", pdrc = ");
        explain_buffer_int(sb, data->hrpcs.pdrc);
        break;

    default:
        break;
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_dvd_authinfo(explain_string_buffer_t *sb, const void *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
