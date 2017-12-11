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

#include <libexplain/ac/sys/mtio.h>

#include <libexplain/buffer/mtop.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


static void
explain_buffer_mtop_op(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef MTBSF
        { "MTBSF", MTBSF },
#endif
#ifdef MTBSFM
        { "MTBSFM", MTBSFM },
#endif
#ifdef MTBSR
        { "MTBSR", MTBSR },
#endif
#ifdef MTBSS
        { "MTBSS", MTBSS },
#endif
#ifdef MTCOMPRESSION
        { "MTCOMPRESSION", MTCOMPRESSION },
#endif
#ifdef MTEOM
        { "MTEOM", MTEOM },
#endif
#ifdef MTERASE
        { "MTERASE", MTERASE },
#endif
#ifdef MTFSF
        { "MTFSF", MTFSF },
#endif
#ifdef MTFSFM
        { "MTFSFM", MTFSFM },
#endif
#ifdef MTFSR
        { "MTFSR", MTFSR },
#endif
#ifdef MTFSS
        { "MTFSS", MTFSS },
#endif
#ifdef MTGRSZ
        /* not linux: get record size */
        { "MTGRSZ", MTGRSZ },
#endif
#ifdef MTLOAD
        { "MTLOAD", MTLOAD },
#endif
#ifdef MTLOCK
        { "MTLOCK", MTLOCK },
#endif
#ifdef MTMKPART
        { "MTMKPART", MTMKPART },
#endif
#ifdef MTNBSF
        { "MTNBSF", MTNBSF },
#endif
#ifdef MTNFSF
        { "MTNFSF", MTNFSF },
#endif
#ifdef MTNOP
        { "MTNOP", MTNOP },
#endif
#ifdef MTOFFL
        { "MTOFFL", MTOFFL },
#endif
#ifdef MTRAS1
        { "MTRAS1", MTRAS1 },
#endif
#ifdef MTRAS2
        { "MTRAS2", MTRAS2 },
#endif
#ifdef MTRAS3
        { "MTRAS3", MTRAS3 },
#endif
#ifdef MTRESET
        { "MTRESET", MTRESET },
#endif
#ifdef MTRETEN
        { "MTRETEN", MTRETEN },
#endif
#ifdef MTREW
        { "MTREW", MTREW },
#endif
#ifdef MTSEEK
        { "MTSEEK", MTSEEK },
#endif
#ifdef MTSETBLK
        { "MTSETBLK", MTSETBLK },
#endif
#ifdef MTSETDENSITY
        { "MTSETDENSITY", MTSETDENSITY },
#endif
#ifdef MTSETDRVBUFFER
        { "MTSETDRVBUFFER", MTSETDRVBUFFER },
#endif
#ifdef MTSETPART
        { "MTSETPART", MTSETPART },
#endif
#ifdef MTSRSZ
        /* Solaris version of MTSETBLK */
        { "MTSRSZ", MTSRSZ },
#endif
#ifdef MTTELL
        { "MTTELL", MTTELL },
#endif
#ifdef MTUNLOAD
        { "MTUNLOAD", MTUNLOAD },
#endif
#ifdef MTUNLOCK
        { "MTUNLOCK", MTUNLOCK },
#endif
#ifdef MTWEOF
        { "MTWEOF", MTWEOF },
#endif
#ifdef MTWSM
        { "MTWSM", MTWSM },
#endif
    };

    explain_parse_bits_print_single(sb, data, table, SIZEOF(table));
}


void
explain_buffer_mtop(explain_string_buffer_t *sb,
    const struct mtop *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        explain_string_buffer_puts(sb, "{ mt_top = ");
        explain_buffer_mtop_op(sb, data->mt_op);
        explain_string_buffer_puts(sb, ", mt_count = ");
        switch (data->mt_op)
        {
#ifdef MTLOAD
#ifdef MT_ST_HPLOADER_OFFSET
        case MTLOAD:
            if (data->mt_count & MT_ST_HPLOADER_OFFSET)
                explain_string_buffer_puts(sb, "MT_ST_HPLOADER_OFFSET | ");
            explain_string_buffer_printf
            (
                sb,
                "%ld",
                (long)(data->mt_count & ~MT_ST_HPLOADER_OFFSET)
            );
            break;
#endif
#endif

#ifdef MTSETDENSITY
        case MTSETDENSITY:
#ifdef MT_ST_WRITE_THRESHOLD
            if (data->mt_count & MT_ST_WRITE_THRESHOLD)
            {
                explain_string_buffer_puts(sb, "MT_ST_WRITE_THRESHOLD | ");
                explain_string_buffer_printf
                (
                    sb,
                    "%ld",
                    (long)(data->mt_count & 0x0FFFFFFF)
                );
                break;
            }
#endif
            explain_string_buffer_printf(sb, "%ld", (long)data->mt_count);
            break;
#endif

        default:
            explain_string_buffer_printf(sb, "%ld", (long)data->mt_count);
            break;
        }
        explain_string_buffer_puts(sb, " }");
    }
}


/* vim: set ts=8 sw=4 et : */
