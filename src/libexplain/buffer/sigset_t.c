/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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

#include <libexplain/ac/signal.h>

#include <libexplain/buffer/sigset_t.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/signal.h>
#include <libexplain/is_efault.h>


void
explain_buffer_sigset_t(explain_string_buffer_t *sb, const sigset_t *data)
{
    int             j;
    int             first;
    int             nsig;

    if (explain_is_efault_pointer(data, sizeof(*data)))
    {
        explain_buffer_pointer(sb, data);
        return;
    }
    explain_string_buffer_puts(sb, "{ ");
    first = 1;
#if defined(NSIG)
    nsig = NSIG;
#elif defined(_NSIG)
    nsig = _NSIG;
#elif defined(MAXSIG)
    nsig = MAXSIG + 1;
#else
    nsig = _SIG_MAXSIG + 1;
#endif
    for (j = 1; j < nsig; ++j)
    {
        if (sigismember(data, j))
        {
            if (first)
                first = 0;
            else
                explain_string_buffer_puts(sb, ", ");
            explain_buffer_signal(sb, j);
        }
    }
    if (!first)
        explain_string_buffer_putc(sb, ' ');
    explain_string_buffer_putc(sb, '}');
}


/* vim: set ts=8 sw=4 et : */
