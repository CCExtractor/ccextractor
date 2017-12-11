/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/ioctl.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/linux/ata.h>
#include <libexplain/ac/linux/hdreg.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/short.h>
#include <libexplain/iocontrol/generic.h>
#include <libexplain/iocontrol/hdio_get_identity.h>

#ifdef HDIO_GET_IDENTITY


static void
print_data(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_pointer(sb, data);
}


static void
print_explanation(const explain_iocontrol_t *p, explain_string_buffer_t *sb, int
    errnum, int fildes, int request, const void *data)
{
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    default:
        explain_iocontrol_generic_print_explanation
        (
            p,
            sb,
            errnum,
            fildes,
            request,
            data
        );
        break;
    }
}


#ifdef HAVE_LINUX_ATA_H

static void
explain_buffer_ata_identity(explain_string_buffer_t *sb, int data)
{
    static const explain_parse_bits_table_t table[] =
    {
        { "ATA_ID_CONFIG", ATA_ID_CONFIG },
        { "ATA_ID_CYLS", ATA_ID_CYLS },
        { "ATA_ID_HEADS", ATA_ID_HEADS },
        { "ATA_ID_SECTORS", ATA_ID_SECTORS },
        { "ATA_ID_SERNO", ATA_ID_SERNO },
        { "ATA_ID_BUF_SIZE", ATA_ID_BUF_SIZE },
        { "ATA_ID_FW_REV", ATA_ID_FW_REV },
        { "ATA_ID_PROD", ATA_ID_PROD },
        { "ATA_ID_MAX_MULTSECT", ATA_ID_MAX_MULTSECT },
        { "ATA_ID_DWORD_IO", ATA_ID_DWORD_IO },
        { "ATA_ID_CAPABILITY", ATA_ID_CAPABILITY },
        { "ATA_ID_OLD_PIO_MODES", ATA_ID_OLD_PIO_MODES },
        { "ATA_ID_OLD_DMA_MODES", ATA_ID_OLD_DMA_MODES },
        { "ATA_ID_FIELD_VALID", ATA_ID_FIELD_VALID },
        { "ATA_ID_CUR_CYLS", ATA_ID_CUR_CYLS },
        { "ATA_ID_CUR_HEADS", ATA_ID_CUR_HEADS },
        { "ATA_ID_CUR_SECTORS", ATA_ID_CUR_SECTORS },
        { "ATA_ID_MULTSECT", ATA_ID_MULTSECT },
        { "ATA_ID_LBA_CAPACITY", ATA_ID_LBA_CAPACITY },
        { "ATA_ID_SWDMA_MODES", ATA_ID_SWDMA_MODES },
        { "ATA_ID_MWDMA_MODES", ATA_ID_MWDMA_MODES },
        { "ATA_ID_PIO_MODES", ATA_ID_PIO_MODES },
        { "ATA_ID_EIDE_DMA_MIN", ATA_ID_EIDE_DMA_MIN },
        { "ATA_ID_EIDE_DMA_TIME", ATA_ID_EIDE_DMA_TIME },
        { "ATA_ID_EIDE_PIO", ATA_ID_EIDE_PIO },
        { "ATA_ID_EIDE_PIO_IORDY", ATA_ID_EIDE_PIO_IORDY },
        { "ATA_ID_QUEUE_DEPTH", ATA_ID_QUEUE_DEPTH },
        { "ATA_ID_MAJOR_VER", ATA_ID_MAJOR_VER },
        { "ATA_ID_COMMAND_SET_1", ATA_ID_COMMAND_SET_1 },
        { "ATA_ID_COMMAND_SET_2", ATA_ID_COMMAND_SET_2 },
        { "ATA_ID_CFSSE", ATA_ID_CFSSE },
        { "ATA_ID_CFS_ENABLE_1", ATA_ID_CFS_ENABLE_1 },
        { "ATA_ID_CFS_ENABLE_2", ATA_ID_CFS_ENABLE_2 },
        { "ATA_ID_CSF_DEFAULT", ATA_ID_CSF_DEFAULT },
        { "ATA_ID_UDMA_MODES", ATA_ID_UDMA_MODES },
        { "ATA_ID_HW_CONFIG", ATA_ID_HW_CONFIG },
        { "ATA_ID_SPG", ATA_ID_SPG },
        { "ATA_ID_LBA_CAPACITY_2", ATA_ID_LBA_CAPACITY_2 },
        { "ATA_ID_LAST_LUN", ATA_ID_LAST_LUN },
        { "ATA_ID_DLF", ATA_ID_DLF },
        { "ATA_ID_CSFO", ATA_ID_CSFO },
        { "ATA_ID_CFA_POWER", ATA_ID_CFA_POWER },
        { "ATA_ID_CFA_KEY_MGMT", ATA_ID_CFA_KEY_MGMT },
        { "ATA_ID_CFA_MODES", ATA_ID_CFA_MODES },
        { "ATA_ID_DATA_SET_MGMT", ATA_ID_DATA_SET_MGMT },
        { "ATA_ID_ROT_SPEED", ATA_ID_ROT_SPEED },
        { "track_bytes", 4 },
        { "sector_bytes", 5 },
        { "buf_type", 20 },
        { "ecc_bytes", 22 },
        { "cur_capacity", 57 },
    };

    explain_parse_bits_print_single(sb, data, table, SIZEOF(table));
}


static void
explain_buffer_ata_identity(explain_string_buffer_t *sb, const short *data)
{
    if (explain_is_efault_pointer(data, ATA_ID_WORDS * sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_putc(sb, '{');
    comma = 0;
    for (j = 0; j < ATA_ID_WORDS; ++j)
    {
        if (data[j])
        {
            if (comma)
                explain_string_buffer_puts(sb, ", ");
            explain_buffer_ata_id(sb, j);
            explain_string_buffer_puts(sb, " = ");
            switch (j)
            {
            case ATA_ID_SERNO:
                explain_string_buffer_putsu_quoted_n
                (
                    sb,
                    &data[j],
                    ATA_ID_SERNO_LEN
                );
                break;

            case ATA_ID_FW_REV:
                explain_string_buffer_putsu_quoted_n
                (
                    sb,
                    &data[j],
                    ATA_FW_REV_LEN
                );
                break;

            case ATA_ID_PROD:
                explain_string_buffer_putsu_quoted_n
                (
                    sb,
                    &data[j],
                    ATA_ID_PROD_LEN
                );
                break;

            case 57:
            case ATA_ID_LBA_CAPACITY:
            case ATA_ID_SPG:
                explain_string_buffer_printf
                (
                    sb,
                    "0x%lX",
                    ata_id_u32(data, j)
                );
                break;

            case ATA_ID_LBA_CAPACITY_2:
                explain_string_buffer_printf
                (
                    sb,
                    "0x%llX",
                    ata_id_u64(data, j)
                );
                break;

            default:
                explain_string_buffer_printf(sb, "0x%hX", data[j]);
                break;
            }
            comma = 1;
        }
    }
    explain_string_buffer_putc(sb, " }");
}

#else

static void
explain_buffer_ata_identity(explain_string_buffer_t *sb, const short *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


static void
print_data_returned(const explain_iocontrol_t *p, explain_string_buffer_t *sb,
    int errnum, int fildes, int request, const void *data)
{
    (void)p;
    (void)errnum;
    (void)fildes;
    (void)request;
    explain_buffer_ata_identity(sb, data);
}


const explain_iocontrol_t explain_iocontrol_hdio_get_identity =
{
    "HDIO_GET_IDENTITY", /* name */
    HDIO_GET_IDENTITY, /* value */
    0, /* disambiguate */
    0, /* print_name */
    print_data,
    print_explanation,
    print_data_returned,
#ifdef HAVE_LINUX_ATA_H
    sizeof(short[ATA_ID_WORDS]), /* data_size */
    "short[ATA_ID_WORDS]", /* data_type */
#else
    sizeof(short[256]), /* data_size */
    "short[256]", /* data_type */
#endif
    IOCONTROL_FLAG_NON_META, /* flags */
    __FILE__,
    __LINE__,
};

#else /* ndef HDIO_GET_IDENTITY */

const explain_iocontrol_t explain_iocontrol_hdio_get_identity =
{
    0, /* name */
    0, /* value */
    0, /* disambiguate */
    0, /* print_name */
    0, /* print_data */
    0, /* print_explanation */
    0, /* print_data_returned */
    0, /* data_size */
    0, /* data_type */
    0, /* flags */
    __FILE__,
    __LINE__,
};

#endif /* HDIO_GET_IDENTITY */


/* vim: set ts=8 sw=4 et : */
