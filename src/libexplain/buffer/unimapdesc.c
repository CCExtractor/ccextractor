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

#include <libexplain/ac/linux/kd.h>

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/unimapdesc.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>


#ifdef HAVE_LINUX_KD_H

void
explain_buffer_unimapdesc(explain_string_buffer_t *sb,
    const struct unimapdesc *value, int extra)
{
    if (explain_is_efault_pointer(value, sizeof(*value)))
        explain_buffer_pointer(sb, value);
    else
    {
        explain_string_buffer_printf
        (
            sb,
            "{ entry_ct = %d, entries = ",
            value->entry_ct
        );
        if (extra && explain_option_debug())
        {
            const struct unipair *ep;
            const struct unipair *end;

            ep = value->entries;
            end = ep + value->entry_ct;
            if (!explain_is_efault_pointer(ep, (end - ep) * sizeof(*ep)))
            {
                for (; ep < end; ++ep)
                {
                    explain_string_buffer_printf
                    (
                        sb,
                        " { unicode = 0x%04X, fontpos = %u },",
                        ep->unicode,
                        ep->fontpos
                    );
                }
                explain_string_buffer_puts(sb, " }");
                return;
            }
        }
        explain_buffer_pointer(sb, value->entries);
    }
}

#endif /* HAVE_LINUX_KD_H */


/* vim: set ts=8 sw=4 et : */
