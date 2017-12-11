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

#include <libexplain/ac/linux/blkpg.h>

#include <libexplain/buffer/blkpg_ioctl_arg.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long_long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_BLKPG_H

static void
explain_buffer_blkpg_op(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "BLKPG_ADD_PARTITION", BLKPG_ADD_PARTITION },
        { "BLKPG_DEL_PARTITION", BLKPG_DEL_PARTITION },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


static void
explain_buffer_blkpg_flags(explain_string_buffer_t *sb, int value)
{
    /* no flags bits defined, yet */
    explain_buffer_int(sb, value);
}


static void
explain_buffer_blkpg_partition(explain_string_buffer_t *sb,
    const struct blkpg_partition *p)
{
    if (explain_is_efault_pointer(p, sizeof(*p)))
    {
        explain_buffer_pointer(sb, p);
        return;
    }

    explain_string_buffer_puts(sb, "{ start = ");
    explain_buffer_long_long(sb, p->start);
    explain_string_buffer_puts(sb, ", length = ");
    explain_buffer_long_long(sb, p->length);
    explain_string_buffer_puts(sb, ", pno = ");
    explain_buffer_int(sb, p->pno);
    explain_string_buffer_puts(sb, ", devname = ");
    explain_string_buffer_puts_quoted_n(sb, p->devname, sizeof(p->devname));
    explain_string_buffer_puts(sb, ", volname = ");
    explain_string_buffer_puts_quoted_n(sb, p->volname, sizeof(p->volname));
    explain_string_buffer_puts(sb, " }");
}


void
explain_buffer_blkpg_ioctl_arg(explain_string_buffer_t *sb,
    const struct blkpg_ioctl_arg *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ op = ");
    explain_buffer_blkpg_op(sb, data->op);
    explain_string_buffer_puts(sb, ", flags = ");
    explain_buffer_blkpg_flags(sb, data->flags);
    switch (data->op)
    {
    case BLKPG_ADD_PARTITION:
    case BLKPG_DEL_PARTITION:
        explain_string_buffer_puts(sb, ", datalen = ");
        explain_buffer_int(sb, data->datalen);
        explain_string_buffer_puts(sb, ", data = ");
        explain_buffer_blkpg_partition(sb, data->data);
        break;

    default:
        explain_string_buffer_puts(sb, ", datalen = ");
        explain_buffer_int(sb, data->datalen);
        explain_string_buffer_puts(sb, ", data = ");
        explain_buffer_pointer(sb, data->data);
        break;
    }
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_blkpg_ioctl_arg(explain_string_buffer_t *sb,
    const struct blkpg_ioctl_arg *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
