/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
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
#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/gettext.h>

#ifdef BPF_ALU

static int
instruction_ok(const struct sock_filter *ftest, unsigned pc, unsigned flen)
{
    /* Linux: net/core/filter.c, sk_chk_filter */
    switch (ftest->code)
    {
    case BPF_ALU|BPF_ADD|BPF_K:
    case BPF_ALU|BPF_ADD|BPF_X:
    case BPF_ALU|BPF_SUB|BPF_K:
    case BPF_ALU|BPF_SUB|BPF_X:
    case BPF_ALU|BPF_MUL|BPF_K:
    case BPF_ALU|BPF_MUL|BPF_X:
    case BPF_ALU|BPF_DIV|BPF_X:
    case BPF_ALU|BPF_AND|BPF_K:
    case BPF_ALU|BPF_AND|BPF_X:
    case BPF_ALU|BPF_OR|BPF_K:
    case BPF_ALU|BPF_OR|BPF_X:
    case BPF_ALU|BPF_LSH|BPF_K:
    case BPF_ALU|BPF_LSH|BPF_X:
    case BPF_ALU|BPF_RSH|BPF_K:
    case BPF_ALU|BPF_RSH|BPF_X:
    case BPF_ALU|BPF_NEG:
    case BPF_LD|BPF_W|BPF_ABS:
    case BPF_LD|BPF_H|BPF_ABS:
    case BPF_LD|BPF_B|BPF_ABS:
    case BPF_LD|BPF_W|BPF_LEN:
    case BPF_LD|BPF_W|BPF_IND:
    case BPF_LD|BPF_H|BPF_IND:
    case BPF_LD|BPF_B|BPF_IND:
    case BPF_LD|BPF_IMM:
    case BPF_LDX|BPF_W|BPF_LEN:
    case BPF_LDX|BPF_B|BPF_MSH:
    case BPF_LDX|BPF_IMM:
    case BPF_MISC|BPF_TAX:
    case BPF_MISC|BPF_TXA:
    case BPF_RET|BPF_K:
    case BPF_RET|BPF_A:
        break;

    /* Some instructions need special checks */

    case BPF_ALU|BPF_DIV|BPF_K:
        /* check for division by zero */
        if (ftest->k == 0)
            return 0;
        break;

    case BPF_LD|BPF_MEM:
    case BPF_LDX|BPF_MEM:
    case BPF_ST:
    case BPF_STX:
        /* check for invalid memory addresses */
        if (ftest->k >= BPF_MEMWORDS)
            return 0;
        break;

    case BPF_JMP|BPF_JA:
        if (ftest->k >= (unsigned)(flen - pc - 1))
            return 0;
        break;

    case BPF_JMP|BPF_JEQ|BPF_K:
    case BPF_JMP|BPF_JEQ|BPF_X:
    case BPF_JMP|BPF_JGE|BPF_K:
    case BPF_JMP|BPF_JGE|BPF_X:
    case BPF_JMP|BPF_JGT|BPF_K:
    case BPF_JMP|BPF_JGT|BPF_X:
    case BPF_JMP|BPF_JSET|BPF_K:
    case BPF_JMP|BPF_JSET|BPF_X:
        /* for conditionals both must be safe */
        if (pc + ftest->jt + 1 >= flen || pc + ftest->jf + 1 >= flen)
            return 0;
        break;

    default:
        return 0;
    }
    return 1;
}


void
explain_buffer_einval_sock_fprog(explain_string_buffer_t *sb,
    const struct sock_fprog *data)
{
    unsigned        pc;

    /* Linux: net/core/filter.c, sk_chk_filter() */
    if (data->len == 0 || data->len > BPF_MAXINSNS)
        explain_buffer_einval_vague(sb, "data->len");

    for (pc = 0; pc < data->len; ++pc)
    {
        const struct sock_filter *ftest;

        ftest = &data->filter[pc];
        if (!instruction_ok(ftest, pc, data->len))
        {
            char buffer[30];
            snprintf(buffer, sizeof(buffer), "data->filter[%u]", pc);
            explain_buffer_einval_vague(sb, buffer);
            return;
        }
    }
    explain_buffer_einval_vague(sb, "data");
}

#else

void
explain_buffer_einval_sock_fprog(explain_string_buffer_t *sb,
    const struct sock_fprog *data)
{
    (void)data;
    explain_buffer_einval_vague(sb, "data");
}

#endif


/* vim: set ts=8 sw=4 et : */
