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

#include <libexplain/ac/linux/tiocl.h>
#include <libexplain/ac/stdint.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/int32_t.h>
#include <libexplain/buffer/int8.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/tioclinux.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_TIOCL_H

static void
explain_buffer_tiocl(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "TIOCL_SETSEL", TIOCL_SETSEL },
        { "TIOCL_PASTESEL", TIOCL_PASTESEL },
        { "TIOCL_UNBLANKSCREEN", TIOCL_UNBLANKSCREEN },
        { "TIOCL_SELLOADLUT", TIOCL_SELLOADLUT },
        { "TIOCL_GETSHIFTSTATE", TIOCL_GETSHIFTSTATE },
        { "TIOCL_GETMOUSEREPORTING", TIOCL_GETMOUSEREPORTING },
        { "TIOCL_SETVESABLANK", TIOCL_SETVESABLANK },
        { "TIOCL_SETKMSGREDIRECT", TIOCL_SETKMSGREDIRECT },
        { "TIOCL_GETFGCONSOLE", TIOCL_GETFGCONSOLE },
        { "TIOCL_SCROLLCONSOLE", TIOCL_SCROLLCONSOLE },
        { "TIOCL_BLANKSCREEN", TIOCL_BLANKSCREEN },
        { "TIOCL_BLANKEDSCREEN", TIOCL_BLANKEDSCREEN },
#ifdef TIOCL_GETKMSGREDIRECT
        { "TIOCL_GETKMSGREDIRECT", TIOCL_GETKMSGREDIRECT },
#endif
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


static void
explain_buffer_tiocl_setsel(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "TIOCL_SELCHAR", TIOCL_SELCHAR },
        { "TIOCL_SELWORD", TIOCL_SELWORD },
        { "TIOCL_SELLINE", TIOCL_SELLINE },
        { "TIOCL_SELPOINTER", TIOCL_SELPOINTER },
        { "TIOCL_SELCLEAR", TIOCL_SELCLEAR },
        { "TIOCL_SELMOUSEREPORT", TIOCL_SELMOUSEREPORT },
        { "TIOCL_SELBUTTONMASK", TIOCL_SELBUTTONMASK },
    };

    explain_parse_bits_print_single(sb, value, table, SIZEOF(table));
}


void
explain_buffer_tioclinux(explain_string_buffer_t *sb, const void *data)
{
    /* See console_ioctl(4) for more information. */
    if (!data)
    {
        print_pointer:
        explain_buffer_pointer(sb, data);
    }
    else
    {
        const unsigned char *cp;

        cp = data;
        switch (*cp)
        {
        case TIOCL_SETSEL:
            {
                struct tiocl_selection sel;

                if (explain_is_efault_pointer(cp, 1 + sizeof(sel)))
                    goto print_pointer;
                memcpy(&sel, cp + 1, sizeof(sel));
                explain_buffer_tiocl(sb, cp[0]);
                explain_string_buffer_puts(sb, "{ case = ");
                explain_buffer_tiocl(sb, cp[0]);
                explain_string_buffer_puts(sb, ", xs = ");
                explain_buffer_uint(sb, sel.xs);
                explain_string_buffer_puts(sb, ", ys = ");
                explain_buffer_uint(sb, sel.ys);
                explain_string_buffer_puts(sb, ", xe = ");
                explain_buffer_uint(sb, sel.xe);
                explain_string_buffer_puts(sb, ", ye = ");
                explain_buffer_uint(sb, sel.ye);
                explain_string_buffer_puts(sb, ", sel_mode = ");
                explain_buffer_tiocl_setsel(sb, sel.sel_mode);
                explain_string_buffer_puts(sb, " }");
            }
            break;

        case TIOCL_SETVESABLANK:
            if (explain_is_efault_pointer(data, 2))
                goto print_pointer;
            explain_string_buffer_puts(sb, "{ case = ");
            explain_buffer_tiocl(sb, cp[0]);
            explain_string_buffer_puts(sb, ", value = ");
            explain_buffer_uint8(sb, cp[1]);
            explain_string_buffer_puts(sb, " }");
            break;

        case TIOCL_SELLOADLUT:
            {
                uint32_t        lut[8];

                if (explain_is_efault_pointer(data, 4 + sizeof(lut)))
                    goto print_pointer;
                memcpy(lut, cp + 4, sizeof(lut));
                explain_string_buffer_puts(sb, "{ case = ");
                explain_buffer_tiocl(sb, cp[0]);
                explain_string_buffer_puts(sb, ", value = ");
                explain_buffer_uint32_array(sb, lut, SIZEOF(lut));
                explain_string_buffer_puts(sb, " }");
            }
            break;

        case TIOCL_SCROLLCONSOLE:
            {
                int32_t         value;

                if (explain_is_efault_pointer(data, 4 + sizeof(value)))
                    goto print_pointer;
                memcpy(&value, cp + 4, sizeof(value));
                explain_string_buffer_puts(sb, "{ case = ");
                explain_buffer_tiocl(sb, cp[0]);
                explain_string_buffer_puts(sb, ", value = ");
                explain_buffer_int32_t(sb, value);
                explain_string_buffer_puts(sb, " }");
            }
            break;

        case TIOCL_GETSHIFTSTATE:
        case TIOCL_GETMOUSEREPORTING:
        case TIOCL_SETKMSGREDIRECT:
        case TIOCL_GETFGCONSOLE:
            /*
             * The above 4 cases return data BUT they overwrite the
             * control bytes, so we don't know at print_data_returned
             * time which code it was, and thus can't print a
             * representation.  Sigh.
             *
             * Fall through...
             */

        case TIOCL_BLANKEDSCREEN:
        case TIOCL_BLANKSCREEN:
        case TIOCL_PASTESEL:
        case TIOCL_UNBLANKSCREEN:
#ifdef TIOCL_GETKMSGREDIRECT
        case TIOCL_GETKMSGREDIRECT:
#endif
        default:
            if (explain_is_efault_pointer(data, 1))
                goto print_pointer;
            explain_string_buffer_puts(sb, "{ case = ");
            explain_buffer_tiocl(sb, cp[0]);
            explain_string_buffer_puts(sb, " }");
            break;
        }
    }
}

#else

void
explain_buffer_tioclinux(explain_string_buffer_t *sb, const void *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
