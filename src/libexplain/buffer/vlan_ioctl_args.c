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

#include <libexplain/ac/linux/if_vlan.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/short.h>
#include <libexplain/buffer/vlan_ioctl_args.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_IF_VLAN_H

static void
explain_buffer_vlan_ioctl_cmd(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "ADD_VLAN_CMD", ADD_VLAN_CMD },
        { "DEL_VLAN_CMD", DEL_VLAN_CMD },
        { "GET_VLAN_EGRESS_PRIORITY_CMD", GET_VLAN_EGRESS_PRIORITY_CMD },
        { "GET_VLAN_INGRESS_PRIORITY_CMD", GET_VLAN_INGRESS_PRIORITY_CMD },
        { "GET_VLAN_REALDEV_NAME_CMD", GET_VLAN_REALDEV_NAME_CMD },
        { "GET_VLAN_VID_CMD", GET_VLAN_VID_CMD },
        { "SET_VLAN_EGRESS_PRIORITY_CMD", SET_VLAN_EGRESS_PRIORITY_CMD },
        { "SET_VLAN_FLAG_CMD", SET_VLAN_FLAG_CMD },
        { "SET_VLAN_INGRESS_PRIORITY_CMD", SET_VLAN_INGRESS_PRIORITY_CMD },
        { "SET_VLAN_NAME_TYPE_CMD", SET_VLAN_NAME_TYPE_CMD },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


static void
explain_buffer_vlan_ioctl_flag(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "VLAN_FLAG_REORDER_HDR", VLAN_FLAG_REORDER_HDR },
#ifdef VLAN_FLAG_GVRP
        { "VLAN_FLAG_GVRP", VLAN_FLAG_GVRP },
#endif
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


static void
explain_buffer_vlan_ioctl_name_type(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "VLAN_NAME_TYPE_PLUS_VID", VLAN_NAME_TYPE_PLUS_VID },
        { "VLAN_NAME_TYPE_RAW_PLUS_VID", VLAN_NAME_TYPE_RAW_PLUS_VID },
        { "VLAN_NAME_TYPE_PLUS_VID_NO_PAD", VLAN_NAME_TYPE_PLUS_VID_NO_PAD },
        { "VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD",
            VLAN_NAME_TYPE_RAW_PLUS_VID_NO_PAD },
        { "VLAN_NAME_TYPE_HIGHEST", VLAN_NAME_TYPE_HIGHEST },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_vlan_ioctl_args(explain_string_buffer_t *sb,
    const struct vlan_ioctl_args *data, int all)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ cmd = ");
    explain_buffer_vlan_ioctl_cmd(sb, data->cmd);
    explain_string_buffer_puts(sb, ", device1 = ");
    explain_string_buffer_puts_quoted_n(sb, data->device1,
        sizeof(data->device1));
    switch (data->cmd)
    {
    case ADD_VLAN_CMD:
    case DEL_VLAN_CMD:
    case SET_VLAN_EGRESS_PRIORITY_CMD:
    case SET_VLAN_FLAG_CMD:
    case SET_VLAN_INGRESS_PRIORITY_CMD:
    case SET_VLAN_NAME_TYPE_CMD:
        all = 1;
        break;

    default:
        break;
    }
    if (all)
    {
        switch (data->cmd)
        {
        case SET_VLAN_INGRESS_PRIORITY_CMD:
        case GET_VLAN_INGRESS_PRIORITY_CMD:
        case SET_VLAN_EGRESS_PRIORITY_CMD:
        case GET_VLAN_EGRESS_PRIORITY_CMD:
            explain_string_buffer_puts(sb, ", skb_priority = ");
            explain_buffer_uint(sb, data->u.skb_priority);
            break;

        case SET_VLAN_NAME_TYPE_CMD:
            explain_string_buffer_puts(sb, ", name_type = ");
            explain_buffer_vlan_ioctl_name_type(sb, data->u.name_type);
            break;

        case SET_VLAN_FLAG_CMD:
            explain_string_buffer_puts(sb, ", flag = ");
            explain_buffer_vlan_ioctl_flag(sb, data->u.flag);
            break;

        case GET_VLAN_REALDEV_NAME_CMD:
            explain_string_buffer_puts(sb, ", device2 = ");
            explain_string_buffer_puts_quoted_n(sb, data->u.device2,
                sizeof(data->u.device2));
            break;

        case GET_VLAN_VID_CMD:
            explain_string_buffer_puts(sb, ", vid = ");
            explain_buffer_int(sb, data->u.VID);
            break;

        case ADD_VLAN_CMD:
        case DEL_VLAN_CMD:
        default:
            /* do nothing */
            break;
        }
    }
    if (data->vlan_qos)
    {
        explain_string_buffer_puts(sb, ", vlan_qos = ");
        explain_buffer_short(sb, data->vlan_qos);
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_vlan_ioctl_args(explain_string_buffer_t *sb,
    const struct vlan_ioctl_args *data, int all)
{
    (void)all;
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
