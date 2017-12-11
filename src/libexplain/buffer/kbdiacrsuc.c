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

#include <libexplain/buffer/kbdiacrsuc.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/option.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


void
explain_buffer_kbdiacrsuc(explain_string_buffer_t *sb,
    const struct kbdiacrsuc *value, int extra)
{
#if defined(HAVE_LINUX_KD_H) && defined(KBDIACRSUC)
    if (explain_is_efault_pointer(value, sizeof(*value)))
        explain_buffer_pointer(sb, value);
    else
    {
        explain_string_buffer_printf(sb, "{ kb_cnt = %u", value->kb_cnt);
        if (extra && explain_option_debug())
        {
            size_t          max;
            const struct kbdiacruc *ep;
            const struct kbdiacruc *end;

            max = value->kb_cnt;
            if (max > SIZEOF(value->kbdiacruc))
                max = SIZEOF(value->kbdiacruc);
            ep = value->kbdiacruc;
            end = ep + max;
            explain_string_buffer_puts(sb, ", kbdiacruc = {");
            for (; ep < end; ++ep)
            {
                explain_string_buffer_printf
                (
                    sb,
                    " { diacr = %#x, base = %#x, result = %#x },",
                    ep->diacr,
                    ep->base,
                    ep->result
                );
            }
            explain_string_buffer_puts(sb, " }");
        }
        explain_string_buffer_puts(sb, " }");
    }
#else
    (void)extra;
    explain_buffer_pointer(sb, value);
#endif
}


/* vim: set ts=8 sw=4 et : */
