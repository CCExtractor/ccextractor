/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2012, 2013 Peter Miller
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

#include <libexplain/buffer/dac.h>
#include <libexplain/capability.h>
#include <libexplain/option.h>


void
explain_buffer_dac_setuid(explain_string_buffer_t *sb)
{
    if (explain_capability_setuid())
        return;
    explain_string_buffer_puts(sb, ", ");
    explain_buffer_and_the_process_is_not_privileged(sb);
#ifdef HAVE_SYS_CAPABILITY_H
    if (explain_option_dialect_specific())
    {
        explain_buffer_does_not_have_capability(sb, "CAP_SETUID");
    }
#endif
}


/* vim: set ts=8 sw=4 et : */
