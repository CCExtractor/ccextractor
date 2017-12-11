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

#include <libexplain/ac/linux/mii.h>

#include <libexplain/buffer/mii_ioctl_data.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_MII_H

static void
explain_buffer_mii_reg(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "MII_BMCR", MII_BMCR },
        { "MII_BMSR", MII_BMSR },
        { "MII_PHYSID1", MII_PHYSID1 },
        { "MII_PHYSID2", MII_PHYSID2 },
        { "MII_ADVERTISE", MII_ADVERTISE },
        { "MII_LPA", MII_LPA },
        { "MII_EXPANSION", MII_EXPANSION },
#ifdef MII_CTRL1000
        { "MII_CTRL1000", MII_CTRL1000 },
#endif
#ifdef MII_STAT1000
        { "MII_STAT1000", MII_STAT1000 },
#endif
#ifdef MII_ESTATUS
        { "MII_ESTATUS", MII_ESTATUS },
#endif
        { "MII_DCOUNTER", MII_DCOUNTER },
        { "MII_FCSCOUNTER", MII_FCSCOUNTER },
        { "MII_NWAYTEST", MII_NWAYTEST },
        { "MII_RERRCOUNTER", MII_RERRCOUNTER },
        { "MII_SREVISION", MII_SREVISION },
        { "MII_LBRERROR", MII_LBRERROR },
        { "MII_PHYADDR", MII_PHYADDR },
        { "MII_TPISTATUS", MII_TPISTATUS },
        { "MII_NCONFIG", MII_NCONFIG },
    };

    explain_parse_bits_print_single(sb, data, table, SIZEOF(table));
}


static void
explain_buffer_mii_bmcr(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "BMCR_RESV", BMCR_RESV },
        { "BMCR_SPEED1000", BMCR_SPEED1000 },
        { "BMCR_CTST", BMCR_CTST },
        { "BMCR_FULLDPLX", BMCR_FULLDPLX },
        { "BMCR_ANRESTART", BMCR_ANRESTART },
        { "BMCR_ISOLATE", BMCR_ISOLATE },
        { "BMCR_PDOWN", BMCR_PDOWN },
        { "BMCR_ANENABLE", BMCR_ANENABLE },
        { "BMCR_SPEED100", BMCR_SPEED100 },
        { "BMCR_LOOPBACK", BMCR_LOOPBACK },
        { "BMCR_RESET", BMCR_RESET },
    };

    explain_parse_bits_print(sb, data, table, SIZEOF(table));
}


static void
explain_buffer_mii_bmsr(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "BMSR_ERCAP", BMSR_ERCAP },
        { "BMSR_JCD", BMSR_JCD },
        { "BMSR_LSTATUS", BMSR_LSTATUS },
        { "BMSR_ANEGCAPABLE", BMSR_ANEGCAPABLE },
        { "BMSR_RFAULT", BMSR_RFAULT },
        { "BMSR_ANEGCOMPLETE", BMSR_ANEGCOMPLETE },
        { "BMSR_RESV", BMSR_RESV },
#ifdef BMSR_ESTATEN
        { "BMSR_ESTATEN", BMSR_ESTATEN },
#endif
#ifdef BMSR_100HALF2
        { "BMSR_100HALF2", BMSR_100HALF2 },
#endif
#ifdef BMSR_100FULL2
        { "BMSR_100FULL2", BMSR_100FULL2 },
#endif
        { "BMSR_10HALF", BMSR_10HALF },
        { "BMSR_10FULL", BMSR_10FULL },
        { "BMSR_100HALF", BMSR_100HALF },
        { "BMSR_100FULL", BMSR_100FULL },
        { "BMSR_100BASE4", BMSR_100BASE4 },
    };

    explain_parse_bits_print(sb, data, table, SIZEOF(table));
}


static void
explain_buffer_mii_ad(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "ADVERTISE_SLCT", ADVERTISE_SLCT },
        { "ADVERTISE_CSMA", ADVERTISE_CSMA },
        { "ADVERTISE_10HALF", ADVERTISE_10HALF },
#ifdef ADVERTISE_1000XFULL
        { "ADVERTISE_1000XFULL", ADVERTISE_1000XFULL },
#endif
        { "ADVERTISE_10FULL", ADVERTISE_10FULL },
#ifdef ADVERTISE_1000XHALF
        { "ADVERTISE_1000XHALF", ADVERTISE_1000XHALF },
#endif
        { "ADVERTISE_100HALF", ADVERTISE_100HALF },
#ifdef ADVERTISE_1000XPAUSE
        { "ADVERTISE_1000XPAUSE", ADVERTISE_1000XPAUSE },
#endif
        { "ADVERTISE_100FULL", ADVERTISE_100FULL },
#ifdef ADVERTISE_1000XPSE_ASYM
        { "ADVERTISE_1000XPSE_ASYM", ADVERTISE_1000XPSE_ASYM },
#endif
        { "ADVERTISE_100BASE4", ADVERTISE_100BASE4 },
#ifdef ADVERTISE_PAUSE_CAP
        { "ADVERTISE_PAUSE_CAP", ADVERTISE_PAUSE_CAP },
#endif
#ifdef ADVERTISE_PAUSE_ASYM
        { "ADVERTISE_PAUSE_ASYM", ADVERTISE_PAUSE_ASYM },
#endif
        { "ADVERTISE_RESV", ADVERTISE_RESV },
        { "ADVERTISE_RFAULT", ADVERTISE_RFAULT },
        { "ADVERTISE_LPACK", ADVERTISE_LPACK },
        { "ADVERTISE_NPAGE", ADVERTISE_NPAGE },
    };

    explain_parse_bits_print(sb, data, table, SIZEOF(table));
}


static void
explain_buffer_mii_lpa(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "LPA_SLCT", LPA_SLCT },
        { "LPA_10HALF", LPA_10HALF },
#ifdef LPA_1000XFULL
        { "LPA_1000XFULL", LPA_1000XFULL },
#endif
        { "LPA_10FULL", LPA_10FULL },
#ifdef LPA_1000XHALF
        { "LPA_1000XHALF", LPA_1000XHALF },
#endif
        { "LPA_100HALF", LPA_100HALF },
#ifdef LPA_1000XPAUSE
        { "LPA_1000XPAUSE", LPA_1000XPAUSE },
#endif
        { "LPA_100FULL", LPA_100FULL },
#ifdef LPA_1000XPAUSE_ASYM
        { "LPA_1000XPAUSE_ASYM", LPA_1000XPAUSE_ASYM },
#endif
        { "LPA_100BASE4", LPA_100BASE4 },
#ifdef LPA_PAUSE_CAP
        { "LPA_PAUSE_CAP", LPA_PAUSE_CAP },
#endif
#ifdef LPA_PAUSE_ASYM
        { "LPA_PAUSE_ASYM", LPA_PAUSE_ASYM },
#endif
        { "LPA_RESV", LPA_RESV },
        { "LPA_RFAULT", LPA_RFAULT },
        { "LPA_LPACK", LPA_LPACK },
        { "LPA_NPAGE", LPA_NPAGE },
    };

    explain_parse_bits_print(sb, data, table, SIZEOF(table));
}


static void
explain_buffer_mii_expansion(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "EXPANSION_NWAY", EXPANSION_NWAY },
        { "EXPANSION_LCWP", EXPANSION_LCWP },
        { "EXPANSION_ENABLENPAGE", EXPANSION_ENABLENPAGE },
        { "EXPANSION_NPCAPABLE", EXPANSION_NPCAPABLE },
        { "EXPANSION_MFAULTS", EXPANSION_MFAULTS },
    };

    explain_parse_bits_print(sb, data, table, SIZEOF(table));
}


void
explain_buffer_mii_ioctl_data(explain_string_buffer_t *sb,
    const struct mii_ioctl_data *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
        explain_buffer_pointer(sb, data);
    else
    {
        explain_string_buffer_printf(sb, "{ phy_id = %d, ", data->phy_id);
        explain_string_buffer_puts(sb, "reg_num = %d, ");
        explain_buffer_mii_reg(sb, data->reg_num);
        switch (data->reg_num)
        {
        case MII_BMCR:
            explain_string_buffer_puts(sb, ", val_in = ");
            explain_buffer_mii_bmcr(sb, data->val_in);
            explain_string_buffer_puts(sb, ", val_out = ");
            explain_buffer_mii_bmcr(sb, data->val_out);
            break;

        case MII_BMSR:
            explain_string_buffer_puts(sb, ", val_in = ");
            explain_buffer_mii_bmsr(sb, data->val_in);
            explain_string_buffer_puts(sb, ", val_out = ");
            explain_buffer_mii_bmsr(sb, data->val_out);
            break;

        case MII_ADVERTISE:
            explain_string_buffer_puts(sb, ", val_in = ");
            explain_buffer_mii_ad(sb, data->val_in);
            explain_string_buffer_puts(sb, ", val_out = ");
            explain_buffer_mii_ad(sb, data->val_out);
            break;

        case MII_LPA:
            explain_string_buffer_puts(sb, ", val_in = ");
            explain_buffer_mii_lpa(sb, data->val_in);
            explain_string_buffer_puts(sb, ", val_out = ");
            explain_buffer_mii_lpa(sb, data->val_out);
            break;

        case MII_EXPANSION:
            explain_string_buffer_puts(sb, ", val_in = ");
            explain_buffer_mii_expansion(sb, data->val_in);
            explain_string_buffer_puts(sb, ", val_out = ");
            explain_buffer_mii_expansion(sb, data->val_out);
            break;

        default:
            explain_string_buffer_printf
            (
                sb,
                ", val_in = %d, val_out = %d",
                data->val_in,
                data->val_out
            );
            break;
        }
        explain_string_buffer_puts(sb, " }");
    }
}

#else

void
explain_buffer_mii_ioctl_data(explain_string_buffer_t *sb,
    const struct mii_ioctl_data *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
