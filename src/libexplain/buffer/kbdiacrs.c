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

#include <libexplain/buffer/kbdiacrs.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_KD_H

void
explain_buffer_kbdiacrs(explain_string_buffer_t *sb,
    const struct kbdiacrs *value, int extra)
{
    if (explain_is_efault_pointer(value, sizeof(*value)))
        explain_buffer_pointer(sb, value);
    else
    {
        explain_string_buffer_printf(sb, "{ kb_cnt = %u", value->kb_cnt);
        if (extra && explain_option_debug())
        {
            size_t          max;
            const struct kbdiacr *ep;
            const struct kbdiacr *end;

            max = value->kb_cnt;
            if (max > SIZEOF(value->kbdiacr))
                max = SIZEOF(value->kbdiacr);
            ep = value->kbdiacr;
            end = ep + max;
            explain_string_buffer_puts(sb, ", kbdiacr = {");
            for (; ep < end; ++ep)
            {
                explain_string_buffer_printf
                (
                    sb,
                    " { diacr = %u, base = %u, result = %u },",
                    ep->diacr,
                    ep->base,
                    ep->result
                );
            }
            explain_string_buffer_puts(sb, " }");
        }
        explain_string_buffer_puts(sb, " }");
    }
}

#endif /* HAVE_LINUX_KD_H */


/* vim: set ts=8 sw=4 et : */
