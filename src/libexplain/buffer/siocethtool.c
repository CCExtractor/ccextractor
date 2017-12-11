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

#include <libexplain/ac/net/if.h>
#include <libexplain/ac/linux/ethtool.h>

#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/siocethtool.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_ETHTOOL_H

void
explain_buffer_siocethtool(explain_string_buffer_t *sb, const struct ifreq *ifr)
{
    if (explain_is_efault_pointer(ifr, sizeof(*ifr)))
        explain_buffer_pointer(sb, ifr);
    else
    {
        explain_string_buffer_puts(sb, "{ ifr_name = ");
        explain_string_buffer_puts_quoted_n
        (
            sb,
            ifr->ifr_name,
            sizeof(ifr->ifr_name)
        );
        explain_string_buffer_puts(sb, ", ifr_data = ");
        if
        (
            explain_is_efault_pointer
            (
                ifr->ifr_data,
                sizeof(struct ethtool_cmd)
            )
        )
        {
            explain_buffer_pointer(sb, ifr->ifr_data);
        }
        else
        {
            static const explain_parse_bits_table_t table[] =
            {
                { "ETHTOOL_GDRVINFO", ETHTOOL_GDRVINFO },
                { "ETHTOOL_GMSGLVL", ETHTOOL_GMSGLVL },
                { "ETHTOOL_GCOALESCE", ETHTOOL_GCOALESCE },
                { "ETHTOOL_GRINGPARAM", ETHTOOL_GRINGPARAM },
                { "ETHTOOL_GPAUSEPARAM", ETHTOOL_GPAUSEPARAM },
                { "ETHTOOL_GRXCSUM", ETHTOOL_GRXCSUM },
                { "ETHTOOL_GTXCSUM", ETHTOOL_GTXCSUM },
                { "ETHTOOL_GSG", ETHTOOL_GSG },
                { "ETHTOOL_GSTRINGS", ETHTOOL_GSTRINGS },
                { "ETHTOOL_GTSO", ETHTOOL_GTSO },
#ifdef ETHTOOL_GPERMADDR
                { "ETHTOOL_GPERMADDR", ETHTOOL_GPERMADDR },
#endif
#ifdef ETHTOOL_GUFO
                { "ETHTOOL_GUFO", ETHTOOL_GUFO },
#endif
#ifdef ETHTOOL_GGSO
                { "ETHTOOL_GGSO", ETHTOOL_GGSO },
#endif
#ifdef ETHTOOL_GFLAGS
                { "ETHTOOL_GFLAGS", ETHTOOL_GFLAGS },
#endif
#ifdef ETHTOOL_GPFLAGS
                { "ETHTOOL_GPFLAGS", ETHTOOL_GPFLAGS },
#endif
#ifdef ETHTOOL_GRXFH
                { "ETHTOOL_GRXFH", ETHTOOL_GRXFH },
#endif
            };
            const struct ethtool_cmd *etp;

            etp = (const struct ethtool_cmd *)ifr->ifr_data;
            explain_string_buffer_puts(sb, "{ cmd = ");
            explain_parse_bits_print_single
            (
                sb,
                etp->cmd,
                table,
                SIZEOF(table)
            );
            explain_string_buffer_puts(sb, " }");
        }
        explain_string_buffer_puts(sb, " }");
    }
}

#else

void
explain_buffer_siocethtool(explain_string_buffer_t *sb, const struct ifreq *ifr)
{
    explain_buffer_pointer(sb, ifr);
}

#endif


/* vim: set ts=8 sw=4 et : */
