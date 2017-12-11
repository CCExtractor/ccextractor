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

#include <libexplain/buffer/dvd_struct.h>
#include <libexplain/buffer/hexdump.h>
#include <libexplain/buffer/int.h>
#include <libexplain/buffer/long.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_CDROM_H

static void
explain_buffer_dvd_struct_type(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "DVD_STRUCT_PHYSICAL", DVD_STRUCT_PHYSICAL },
        { "DVD_STRUCT_COPYRIGHT", DVD_STRUCT_COPYRIGHT },
        { "DVD_STRUCT_DISCKEY", DVD_STRUCT_DISCKEY },
        { "DVD_STRUCT_BCA", DVD_STRUCT_BCA },
        { "DVD_STRUCT_MANUFACT", DVD_STRUCT_MANUFACT },
    };

    return explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


static void
explain_buffer_dvd_layer(explain_string_buffer_t *sb,
    const struct dvd_layer *data)
{
    explain_string_buffer_puts(sb, "{ book_version = ");
    explain_buffer_int(sb, data->book_version);
    explain_string_buffer_puts(sb, ", book_type = ");
    explain_buffer_int(sb, data->book_type);
    explain_string_buffer_puts(sb, ", min_rate = ");
    explain_buffer_int(sb, data->min_rate);
    explain_string_buffer_puts(sb, ", disc_size = ");
    explain_buffer_int(sb, data->disc_size);
    explain_string_buffer_puts(sb, ", layer_type = ");
    explain_buffer_int(sb, data->layer_type);
    explain_string_buffer_puts(sb, ", track_path = ");
    explain_buffer_int(sb, data->track_path);
    explain_string_buffer_puts(sb, ", nlayers = ");
    explain_buffer_int(sb, data->nlayers);
    explain_string_buffer_puts(sb, ", track_density = ");
    explain_buffer_int(sb, data->track_density);
    explain_string_buffer_puts(sb, ", linear_density = ");
    explain_buffer_int(sb, data->linear_density);
    explain_string_buffer_puts(sb, ", bca = ");
    explain_buffer_int(sb, data->bca);
    explain_string_buffer_puts(sb, ", start_sector = ");
    explain_buffer_ulong(sb, data->start_sector);
    explain_string_buffer_puts(sb, ", end_sector = ");
    explain_buffer_ulong(sb, data->end_sector);
    explain_string_buffer_puts(sb, ", end_sector_l0 = ");
    explain_buffer_ulong(sb, data->end_sector_l0);
    explain_string_buffer_puts(sb, " }");
}


static void
explain_buffer_dvd_physical(explain_string_buffer_t *sb,
    const struct dvd_physical *data)
{
    size_t          j;

    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_dvd_struct_type(sb, data->type);
    explain_string_buffer_puts(sb, ", layer_num = ");
    explain_buffer_int(sb, data->layer_num);
    explain_string_buffer_puts(sb, ", layer = {");
    for (j = 0; j < SIZEOF(data->layer); ++j)
        explain_buffer_dvd_layer(sb, &data->layer[j]);
    explain_string_buffer_puts(sb, "}}");
}


static void
explain_buffer_dvd_copyright(explain_string_buffer_t *sb,
    const struct dvd_copyright *data)
{
    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_dvd_struct_type(sb, data->type);
    explain_string_buffer_puts(sb, ", layer_num = ");
    explain_buffer_int(sb, data->layer_num);
    explain_string_buffer_puts(sb, ", cpst = {");
    explain_buffer_int(sb, data->cpst);
    explain_string_buffer_puts(sb, ", rmi = {");
    explain_buffer_int(sb, data->rmi);
    explain_string_buffer_puts(sb, " }");
}


static void
explain_buffer_dvd_disckey(explain_string_buffer_t *sb,
    const struct dvd_disckey *data)
{
    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_dvd_struct_type(sb, data->type);
    explain_string_buffer_puts(sb, ", agid = ");
    explain_buffer_int(sb, data->agid);
    explain_string_buffer_puts(sb, ", value = {");
    explain_buffer_hexdump(sb, data->value, 8);
    explain_string_buffer_puts(sb, " ... }}");
}


static void
explain_buffer_dvd_bca(explain_string_buffer_t *sb,
    const struct dvd_bca *data)
{
    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_dvd_struct_type(sb, data->type);
    explain_string_buffer_puts(sb, ", len = ");
    explain_buffer_int(sb, data->len);
    explain_string_buffer_puts(sb, ", value = {");
    if (data->len > 0)
    {
        size_t          len;

        len = data->len;
        if (len > sizeof(data->value))
            len = sizeof(data->value);
        if (len > 8)
        {
            explain_buffer_hexdump(sb, data->value, 8);
            explain_string_buffer_puts(sb, " ...");
        }
        else
            explain_buffer_hexdump(sb, data->value, len);
        explain_string_buffer_puts(sb, "}");
    }
    explain_string_buffer_puts(sb, " }");
}


static void
explain_buffer_dvd_manufact(explain_string_buffer_t *sb,
    const struct dvd_manufact *data)
{
    explain_string_buffer_puts(sb, "{ type = ");
    explain_buffer_dvd_struct_type(sb, data->type);
    explain_string_buffer_puts(sb, ", layer_num = ");
    explain_buffer_int(sb, data->layer_num);
    explain_string_buffer_puts(sb, ", len = ");
    explain_buffer_int(sb, data->len);
    explain_string_buffer_puts(sb, ", value = {");
    if (data->len > 0)
    {
        size_t          len;

        len = data->len;
        if (len > sizeof(data->value))
            len = sizeof(data->value);
        if (len > 8)
        {
            explain_buffer_hexdump(sb, data->value, 8);
            explain_string_buffer_puts(sb, " ...");
        }
        else
            explain_buffer_hexdump(sb, data->value, len);
        explain_string_buffer_puts(sb, "}");
    }
    explain_string_buffer_puts(sb, " }");
}


void
explain_buffer_dvd_struct(explain_string_buffer_t *sb,
    const dvd_struct *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    switch (data->type)
    {
    case DVD_STRUCT_PHYSICAL:
        explain_buffer_dvd_physical(sb, &data->physical);
        break;

    case DVD_STRUCT_COPYRIGHT:
        explain_buffer_dvd_copyright(sb, &data->copyright);
        break;

    case DVD_STRUCT_DISCKEY:
        explain_buffer_dvd_disckey(sb, &data->disckey);
        break;

    case DVD_STRUCT_BCA:
        explain_buffer_dvd_bca(sb, &data->bca);
        break;

    case DVD_STRUCT_MANUFACT:
        explain_buffer_dvd_manufact(sb, &data->manufact);
        break;

    default:
        explain_string_buffer_puts(sb, "{ type = ");
        explain_buffer_dvd_struct_type(sb, data->type);
        explain_string_buffer_puts(sb, " }");
        break;
    }
}

#else

void
explain_buffer_dvd_struct(explain_string_buffer_t *sb, const void *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
