/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013, 2014 Peter Miller
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

#include <libexplain/ac/sys/time.h>
#include <libexplain/ac/sys/resource.h>

#include <libexplain/buffer/enomem.h>
#include <libexplain/buffer/gettext.h>


/*
 * return 0 if print nothing
 * return 1 if printed anything
 */
int
explain_buffer_enomem_exhausting_swap(explain_string_buffer_t *sb)
{
    explain_string_buffer_puts(sb, ", ");
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext:  This message is used when supplementing an explation an
         * ENOMEM error, when it is specific to user space memory, and the
         * process has an infinite memory limit, meaning that a system limit on
         * the total amout of user space memory available to all processes has
         * been exhausted.
         */
        i18n("probably by exhausting swap space")
    );
    return 1;
}


/* vim: set ts=8 sw=4 et : */
