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

#include <libexplain/ac/sys/timex.h>

#include <libexplain/buffer/timex.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/is_efault.h>
#include <libexplain/parse_bits.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_SYS_TIMEX_H

static void
explain_buffer_timex_mode(explain_string_buffer_t *sb, int mode)
{
    static const struct explain_parse_bits_table_t table[] =
    {
#ifdef ADJ_OFFSET
        { "ADJ_OFFSET", ADJ_OFFSET },
#endif
#ifdef ADJ_FREQUENCY
        { "ADJ_FREQUENCY", ADJ_FREQUENCY },
#endif
#ifdef ADJ_MAXERROR
        { "ADJ_MAXERROR", ADJ_MAXERROR },
#endif
#ifdef ADJ_ESTERROR
        { "ADJ_ESTERROR", ADJ_ESTERROR },
#endif
#ifdef ADJ_STATUS
        { "ADJ_STATUS", ADJ_STATUS },
#endif
#ifdef ADJ_TIMECONST
        { "ADJ_TIMECONST", ADJ_TIMECONST },
#endif
#ifdef ADJ_TICK
        { "ADJ_TICK", ADJ_TICK },
#endif
#ifdef MOD_OFFSET
        { "MOD_OFFSET", MOD_OFFSET },
#endif
#ifdef MOD_FREQUENCY
        { "MOD_FREQUENCY", MOD_FREQUENCY },
#endif
#ifdef MOD_MAXERROR
        { "MOD_MAXERROR", MOD_MAXERROR },
#endif
#ifdef MOD_ESTERROR
        { "MOD_ESTERROR", MOD_ESTERROR },
#endif
#ifdef MOD_STATUS
        { "MOD_STATUS", MOD_STATUS },
#endif
#ifdef MOD_TIMECONST
        { "MOD_TIMECONST", MOD_TIMECONST },
#endif
#ifdef MOD_PSSMAX
        { "MOD_PSSMAX", MOD_PSSMAX },
#endif
#ifdef MOD_TAI
        { "MOD_TAI", MOD_TAI },
#endif
#ifdef MOD_MICRO
        { "MOD_MICRO", MOD_MICRO },
#endif
#ifdef MOD_NANO
        { "MOD_NANO", MOD_NANO },
#endif
#ifdef MOD_CLKA
        { "MOD_CLKA", MOD_CLKA },
#endif
#ifdef MOD_CLKB
        { "MOD_CLKB", MOD_CLKB },
#endif
    };

#ifdef ADJ_OFFSET_SINGLESHOT
    if (mode == ADJ_OFFSET_SINGLESHOT)
        explain_string_buffer_puts(sb, "ADJ_OFFSET_SINGLESHOT");
    else
#endif
#ifdef ADJ_OFFSET_SS_READ
    if (mode == ADJ_OFFSET_SS_READ)
        explain_string_buffer_puts(sb, "ADJ_OFFSET_SS_READ");
    else
#endif
        explain_parse_bits_print(sb, mode, table, SIZEOF(table));
}


static void
explain_buffer_timex_status(explain_string_buffer_t *sb, int status)
{
    static const struct explain_parse_bits_table_t table[] =
    {
        { "STA_PLL", STA_PLL },
        { "STA_PPSFREQ", STA_PPSFREQ },
        { "STA_PPSTIME", STA_PPSTIME },
        { "STA_FLL", STA_FLL },
        { "STA_INS", STA_INS },
        { "STA_DEL", STA_DEL },
        { "STA_UNSYNC", STA_UNSYNC },
        { "STA_FREQHOLD", STA_FREQHOLD },
        { "STA_PPSSIGNAL", STA_PPSSIGNAL },
        { "STA_PPSJITTER", STA_PPSJITTER },
        { "STA_PPSWANDER", STA_PPSWANDER },
        { "STA_PPSERROR", STA_PPSERROR },
        { "STA_CLOCKERR", STA_CLOCKERR },
#ifdef STA_NANO
        { "STA_NANO", STA_NANO },
#endif
#ifdef STA_MODE
        { "STA_MODE", STA_MODE },
#endif
#ifdef STA_CLK
        { "STA_CLK", STA_CLK },
#endif
    };

    explain_parse_bits_print(sb, status, table, SIZEOF(table));
}

#endif


void
explain_buffer_timex(explain_string_buffer_t *sb,
    const struct timex *data)
{
#ifdef HAVE_SYS_TIMEX_H
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ modes = ");
    explain_buffer_timex_mode(sb, data->modes);
#ifdef ADJ_OFFSET_SINGLESHOT
    if (data->modes == ADJ_OFFSET_SINGLESHOT)
    {
        explain_string_buffer_printf(sb, ", offset = %ld", (long)data->offset);
    }
    else
#endif
#ifdef ADJ_OFFSET_SS_READ
    if (data->modes == ADJ_OFFSET_SS_READ)
    {
        /* do nothing more */
    }
    else
#endif
    {
#if !defined(ADJ_OFFSET) && defined(MOD_OFFSET)
#define ADJ_OFFSET MOD_OFFSET
#endif
#ifdef ADJ_OFFSET
        if (data->modes & ADJ_OFFSET)
        {
            explain_string_buffer_printf
            (
                sb,
                ", offset = %ld",
                (long)data->offset
            );
        }
#endif
#if !defined(ADJ_FREQUENCY) && defined(MOD_FREQUENCY)
#define ADJ_FREQUENCY MOD_FREQUENCY
#endif
#ifdef ADJ_FREQUENCY
        if (data->modes & ADJ_FREQUENCY)
        {
            explain_string_buffer_printf
            (
                sb,
                ", freq = %ld",
                (long)data->freq
            );
        }
#endif
#if !defined(ADJ_MAXERROR) && defined(MOD_MAXERROR)
#define ADJ_MAXERROR MOD_MAXERROR
#endif
#ifdef ADJ_MAXERROR
        if (data->modes & ADJ_MAXERROR)
        {
            explain_string_buffer_printf
            (
                sb,
                ", maxerror = %ld",
                (long)data->maxerror
            );
        }
#endif
#if !defined(ADJ_ESTERROR) && defined(MOD_ESTERROR)
#define ADJ_ESTERROR MOD_ESTERROR
#endif
#ifdef ADJ_ESTERROR
        if (data->modes & ADJ_ESTERROR)
        {
            explain_string_buffer_printf
            (
                sb,
                ", esterror = %ld",
                (long)data->esterror
            );
        }
#endif
#if !defined(ADJ_STATUS) && defined(MOD_STATUS)
#define ADJ_STATUS MOD_STATUS
#endif
#ifdef ADJ_STATUS
        if (data->modes & ADJ_STATUS)
        {
            explain_string_buffer_puts(sb, ", status = ");
            explain_buffer_timex_status(sb, data->status);
        }
#endif
#if !defined(ADJ_TIMECONST) && defined(MOD_TIMECONST)
#define ADJ_TIMECONST MOD_TIMECONST
#endif
#ifdef ADJ_TIMECONST
        if (data->modes & ADJ_TIMECONST)
        {
            explain_string_buffer_printf
            (
                sb,
                ", constant = %ld",
                (long)data->constant
            );
        }
#endif
#ifdef ADJ_TICK
        if (data->modes & ADJ_TICK)
        {
            explain_string_buffer_printf
            (
                sb,
                ", tick = %ld",
                (long)data->tick
            );
        }
#endif
    }
    explain_string_buffer_puts(sb, " }");
#else
    explain_buffer_pointer(sb, data);
#endif
}


/* vim: set ts=8 sw=4 et : */
