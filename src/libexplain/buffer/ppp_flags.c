/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2011, 2013 Peter Miller
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

#include <libexplain/ac/net/if_ppp.h>

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/ppp_flags.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>


void
explain_buffer_ppp_flags(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t table[] =
    {
#ifdef SC_COMP_PROT
        { "SC_COMP_PROT", SC_COMP_PROT },
#endif
#ifdef SC_COMP_AC
        { "SC_COMP_AC", SC_COMP_AC },
#endif
#ifdef SC_COMP_TCP
        { "SC_COMP_TCP", SC_COMP_TCP },
#endif
#ifdef SC_NO_TCP_CCID
        { "SC_NO_TCP_CCID", SC_NO_TCP_CCID },
#endif
#ifdef SC_REJ_COMP_AC
        { "SC_REJ_COMP_AC", SC_REJ_COMP_AC },
#endif
#ifdef SC_REJ_COMP_TCP
        { "SC_REJ_COMP_TCP", SC_REJ_COMP_TCP },
#endif
#ifdef SC_CCP_OPEN
        { "SC_CCP_OPEN", SC_CCP_OPEN },
#endif
#ifdef SC_CCP_UP
        { "SC_CCP_UP", SC_CCP_UP },
#endif
#ifdef SC_ENABLE_IP
        { "SC_ENABLE_IP", SC_ENABLE_IP },
#endif
#ifdef SC_LOOP_TRAFFIC
        { "SC_LOOP_TRAFFIC", SC_LOOP_TRAFFIC },
#endif
#ifdef SC_MULTILINK
        { "SC_MULTILINK", SC_MULTILINK },
#endif
#ifdef SC_MP_SHORTSEQ
        { "SC_MP_SHORTSEQ", SC_MP_SHORTSEQ },
#endif
#ifdef SC_COMP_RUN
        { "SC_COMP_RUN", SC_COMP_RUN },
#endif
#ifdef SC_DECOMP_RUN
        { "SC_DECOMP_RUN", SC_DECOMP_RUN },
#endif
#ifdef SC_MP_XSHORTSEQ
        { "SC_MP_XSHORTSEQ", SC_MP_XSHORTSEQ },
#endif
#ifdef SC_DEBUG
        { "SC_DEBUG", SC_DEBUG },
#endif
#ifdef SC_LOG_INPKT
        { "SC_LOG_INPKT", SC_LOG_INPKT },
#endif
#ifdef SC_LOG_OUTPKT
        { "SC_LOG_OUTPKT", SC_LOG_OUTPKT },
#endif
#ifdef SC_LOG_RAWIN
        { "SC_LOG_RAWIN", SC_LOG_RAWIN },
#endif
#ifdef SC_LOG_FLUSH
        { "SC_LOG_FLUSH", SC_LOG_FLUSH },
#endif
#ifdef SC_SYNC
        { "SC_SYNC", SC_SYNC },
#endif
#ifdef SC_MUST_COMP
        { "SC_MUST_COMP", SC_MUST_COMP },
#endif
    };

    explain_parse_bits_print(sb, value, table, SIZEOF(table));
}


void
explain_buffer_ppp_flags_star(explain_string_buffer_t *sb, const int *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
    }
    else
    {
        explain_string_buffer_puts(sb, "{ ");
        explain_buffer_ppp_flags(sb, *data);
        explain_string_buffer_puts(sb, " }");
    }
}


/* vim: set ts=8 sw=4 et : */
