/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2012, 2013 Peter Miller
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

#include <libexplain/ac/stdio.h>

#include <libexplain/buffer/errno/pclose.h>
#include <libexplain/buffer/wait_status.h>
#include <libexplain/common_message_buffer.h>
#include <libexplain/pclose.h>
#include <libexplain/string_buffer.h>
#include <libexplain/output.h>


int
explain_pclose_success(FILE *fp)
{
    int             status;

    status = pclose(fp);
    if (status < 0)
    {
        explain_output_error("%s", explain_pclose(fp));
    }
    else if (status != 0)
    {
        explain_string_buffer_t sb;

        explain_string_buffer_init
        (
            &sb,
            explain_common_message_buffer,
            explain_common_message_buffer_size
        );
        explain_buffer_errno_pclose(&sb, 0, fp);

        /* FIXME: i18n */
        explain_string_buffer_puts(&sb, ", but ");
        explain_buffer_wait_status(&sb, status);
        explain_output_error("%s", explain_common_message_buffer);
    }
    return status;
}


void
explain_pclose_success_or_die(FILE *fp)
{
    if (explain_pclose_success(fp))
        explain_output_exit_failure();
}


/* vim: set ts=8 sw=4 et : */
