/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/sys/ioctl.h>

#include <libexplain/iocontrol/table.h>
#include <libexplain/parse_bits.h>


int
explain_parse_ioctl_request_or_die(const char *text)
{
    explain_parse_bits_table_t *tokens;
    size_t          size;
    int             result;

    /*
     * We must build a parse_bits table to pass to the parser.
     */
    tokens = malloc(sizeof(tokens[0]) * (explain_iocontrol_table_size + 2));
    size = 0;
    if (tokens)
    {
        explain_parse_bits_table_t *p;
        const explain_iocontrol_t *const *tpp;
        const explain_iocontrol_t *const *end;

        end = explain_iocontrol_table + explain_iocontrol_table_size;
        p = tokens;
        for (tpp = explain_iocontrol_table; tpp < end; ++tpp)
        {
            const explain_iocontrol_t *tp;

            tp = *tpp;
            if (tp->name && tp->number != -1)
            {
                p->name = tp->name;
                p->value = tp->number;
                ++p;
            }
        }

#ifdef SIOCDEVPRIVATE
        p->name = "SIOCDEVPRIVATE";
        p->value = SIOCDEVPRIVATE;
        ++p;
#endif

#ifdef SIOCPROTOPRIVATE
        p->name = "SIOCPROTOPRIVATE";
        p->value = SIOCPROTOPRIVATE;
        ++p;
#endif

        size = p - tokens;
    }
    result = explain_parse_bits_or_die(text, tokens, size, "ioctl request");
    if (tokens)
        free(tokens);
    return result;
}


/* vim: set ts=8 sw=4 et : */
