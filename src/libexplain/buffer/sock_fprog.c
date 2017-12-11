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

#include <libexplain/ac/linux/filter.h>

#include <libexplain/buffer/int.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/sock_fprog.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>
#include <libexplain/sizeof.h>


#ifdef HAVE_LINUX_FILTER_H

static void
explain_buffer_filter_code(explain_string_buffer_t *sb, int value)
{
    static const explain_parse_bits_table_t class_table[] =
    {
        { "BPF_LD", BPF_LD },
        { "BPF_LDX", BPF_LDX },
        { "BPF_ST", BPF_ST },
        { "BPF_STX", BPF_STX },
        { "BPF_ALU", BPF_ALU },
        { "BPF_JMP", BPF_JMP },
        { "BPF_RET", BPF_RET },
        { "BPF_MISC", BPF_MISC },
    };

    static const explain_parse_bits_table_t size_table[] =
    {
        { "BPF_W", BPF_W },
        { "BPF_H", BPF_H },
        { "BPF_B", BPF_B },
    };

    static const explain_parse_bits_table_t mode_table[] =
    {
        { "BPF_IMM", BPF_IMM },
        { "BPF_ABS", BPF_ABS },
        { "BPF_IND", BPF_IND },
        { "BPF_MEM", BPF_MEM },
        { "BPF_LEN", BPF_LEN },
        { "BPF_MSH", BPF_MSH },
    };

    static const explain_parse_bits_table_t op_table[] =
    {
        { "BPF_ADD", BPF_ADD },
        { "BPF_SUB", BPF_SUB },
        { "BPF_MUL", BPF_MUL },
        { "BPF_DIV", BPF_DIV },
        { "BPF_OR", BPF_OR },
        { "BPF_AND", BPF_AND },
        { "BPF_LSH", BPF_LSH },
        { "BPF_RSH", BPF_RSH },
        { "BPF_NEG", BPF_NEG },
        { "BPF_JA", BPF_JA },
        { "BPF_JEQ", BPF_JEQ },
        { "BPF_JGT", BPF_JGT },
        { "BPF_JGE", BPF_JGE },
        { "BPF_JSET", BPF_JSET },
    };

    static const explain_parse_bits_table_t src_table[] =
    {
        { "BPF_K", BPF_K },
        { "BPF_X", BPF_X },
    };

    static const explain_parse_bits_table_t rval_table[] =
    {
        { "BPF_A", BPF_A },
    };

    static const explain_parse_bits_table_t miscop_table[] =
    {
        { "BPF_TAX", BPF_TAX },
        { "BPF_TXA", BPF_TXA },
    };

    explain_parse_bits_print_single
    (
        sb,
        BPF_CLASS(value),
        class_table,
        SIZEOF(class_table)
    );

    switch (BPF_CLASS(value))
    {
    case BPF_LD:
    case BPF_LDX:
    case BPF_ST:
    case BPF_STX:
        explain_string_buffer_puts(sb, " | ");
        explain_parse_bits_print_single
        (
            sb,
            BPF_SIZE(value),
            size_table,
            SIZEOF(size_table)
        );
        explain_string_buffer_puts(sb, " | ");
        explain_parse_bits_print_single
        (
            sb,
            BPF_MODE(value),
            mode_table,
            SIZEOF(mode_table)
        );
        break;

    case BPF_ALU:
    case BPF_JMP:
        explain_string_buffer_puts(sb, " | ");
        explain_parse_bits_print_single
        (
            sb,
            BPF_SIZE(value),
            op_table,
            SIZEOF(op_table)
        );
        explain_string_buffer_puts(sb, " | ");
        explain_parse_bits_print_single
        (
            sb,
            BPF_MODE(value),
            src_table,
            SIZEOF(src_table)
        );
        break;

    case BPF_RET:
        explain_string_buffer_puts(sb, " | ");
        explain_parse_bits_print_single
        (
            sb,
            BPF_SIZE(value),
            rval_table,
            SIZEOF(rval_table)
        );
        explain_string_buffer_puts(sb, " | ");
        explain_parse_bits_print_single
        (
            sb,
            BPF_MODE(value),
            src_table,
            SIZEOF(src_table)
        );
        break;

    case BPF_MISC:
        explain_string_buffer_puts(sb, " | ");
        explain_parse_bits_print_single
        (
            sb,
            BPF_MODE(value),
            miscop_table,
            SIZEOF(miscop_table)
        );
        break;

    default:
        break;
    }
}


static int
print_k(int code)
{
    switch (BPF_CLASS(code))
    {
    case BPF_LD:
    case BPF_LDX:
        switch (BPF_MODE(code))
        {
        case BPF_ABS:
        case BPF_IND:
        case BPF_MSH:
        case BPF_IMM:
        case BPF_MEM:
            return 1;

        default:
            return 0;
        }

    case BPF_ST:
    case BPF_STX:
        return 1;

    case BPF_JMP:
        if (code == (BPF_JMP | BPF_JA))
            return 1;
        /* fall through... */

    case BPF_ALU:
    case BPF_RET:
        return (BPF_SRC(code) == BPF_K);

    case BPF_MISC:
        return 0;

    default:
        return 0;
    }
}


static int
print_j(int code)
{
    switch (BPF_CLASS(code))
    {
    case BPF_JMP:
        return 1;

    default:
        return 0;
    }
    return 0;
}


static void
explain_buffer_sock_filter_array(explain_string_buffer_t *sb,
    const struct sock_filter *data, size_t len)
{
    const struct sock_filter *end;

    if (len > BPF_MAXINSNS)
        len = BPF_MAXINSNS;
    if (explain_is_efault_pointer(data, sizeof(*data) * len))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ ");
    for (end = data + len; data < end; ++data)
    {
        int             code;

        code = data->code;
        if (print_k(code))
        {
            if (print_j(code))
            {
                explain_string_buffer_puts(sb, "BPF_JUMP(");
                explain_buffer_filter_code(sb, code);
                explain_string_buffer_puts(sb, ", ");
                explain_buffer_int(sb, data->k);
                explain_string_buffer_puts(sb, ", ");
                explain_buffer_int(sb, data->jt);
                explain_string_buffer_puts(sb, ", ");
                explain_buffer_int(sb, data->jf);
                explain_string_buffer_putc(sb, ')');
            }
            else
            {
                explain_string_buffer_puts(sb, "BPF_STMT(");
                explain_buffer_filter_code(sb, code);
                explain_string_buffer_puts(sb, ", ");
                explain_buffer_int(sb, data->k);
                explain_string_buffer_putc(sb, ')');
            }
        }
        else
        {
            explain_string_buffer_puts(sb, "{ code = ");
            explain_buffer_filter_code(sb, code);
            if (print_j(code))
            {
                explain_string_buffer_puts(sb, ", jt = ");
                explain_buffer_int(sb, data->jt);
                explain_string_buffer_puts(sb, ", jf = ");
                explain_buffer_int(sb, data->jf);
            }
            if (print_k(code))
            {
                explain_string_buffer_puts(sb, ", k = ");
                explain_buffer_int(sb, data->k);
            }
            explain_string_buffer_puts(sb, " }");
        }
        explain_string_buffer_puts(sb, ", ");
    }
    explain_string_buffer_puts(sb, "}");
}


void
explain_buffer_sock_fprog(explain_string_buffer_t *sb,
    const struct sock_fprog *data)
{
    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }

    explain_string_buffer_puts(sb, "{ len = ");
    explain_buffer_uint(sb, data->len);
    explain_string_buffer_puts(sb, ", filter = ");
    explain_buffer_sock_filter_array(sb, data->filter, data->len);
    explain_string_buffer_puts(sb, " }");
}

#else

void
explain_buffer_sock_fprog(explain_string_buffer_t *sb,
    const struct sock_fprog *data)
{
    explain_buffer_pointer(sb, data);
}

#endif


/* vim: set ts=8 sw=4 et : */
