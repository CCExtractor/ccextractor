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

#include <libexplain/ac/linux/if_ppp.h>

#include <libexplain/buffer/if_ppp_state.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_IF_PPP_H

static explain_parse_bits_table_t table[] =
{
    { "SC_COMP_PROT", SC_COMP_PROT },
    { "SC_COMP_AC", SC_COMP_AC },
    { "SC_COMP_TCP", SC_COMP_TCP },
    { "SC_NO_TCP_CCID", SC_NO_TCP_CCID },
    { "SC_REJ_COMP_AC", SC_REJ_COMP_AC },
    { "SC_REJ_COMP_TCP", SC_REJ_COMP_TCP },
    { "SC_CCP_OPEN", SC_CCP_OPEN },
    { "SC_CCP_UP", SC_CCP_UP },
    { "SC_ENABLE_IP", SC_ENABLE_IP },
    { "SC_LOOP_TRAFFIC", SC_LOOP_TRAFFIC },
    { "SC_MULTILINK", SC_MULTILINK },
    { "SC_MP_SHORTSEQ", SC_MP_SHORTSEQ },
    { "SC_COMP_RUN", SC_COMP_RUN },
    { "SC_DECOMP_RUN", SC_DECOMP_RUN },
    { "SC_MP_XSHORTSEQ", SC_MP_XSHORTSEQ },
    { "SC_DEBUG", SC_DEBUG },
    { "SC_LOG_INPKT", SC_LOG_INPKT },
    { "SC_LOG_OUTPKT", SC_LOG_OUTPKT },
    { "SC_LOG_RAWIN", SC_LOG_RAWIN },
    { "SC_LOG_FLUSH", SC_LOG_FLUSH },
    { "SC_SYNC", SC_SYNC },
    { "SC_MUST_COMP", SC_MUST_COMP },
    { "SC_XMIT_BUSY", SC_XMIT_BUSY },
    { "SC_RCV_ODDP", SC_RCV_ODDP },
    { "SC_RCV_EVNP", SC_RCV_EVNP },
    { "SC_RCV_B7_1", SC_RCV_B7_1 },
    { "SC_RCV_B7_0", SC_RCV_B7_0 },
    { "SC_DC_FERROR", SC_DC_FERROR },
    { "SC_DC_ERROR", SC_DC_ERROR },
};


void
explain_buffer_if_ppp_state(explain_string_buffer_t *sb, const int *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
    }
    else
    {
        explain_string_buffer_puts(sb, "{ ");
        explain_parse_bits_print(sb, *data, table, SIZEOF(table));
        explain_string_buffer_puts(sb, " }");
    }
}

#else

void
explain_buffer_if_ppp_state(explain_string_buffer_t *sb, const int *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
