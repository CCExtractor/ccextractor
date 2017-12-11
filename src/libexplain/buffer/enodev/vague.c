/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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

#include <libexplain/buffer/enodev.h>
#include <libexplain/buffer/gettext.h>


void
explain_buffer_enodev(explain_string_buffer_t *sb)
{
    /* The super-vague version */
    explain_buffer_gettext
    (
        sb,
        /* FIXME: i18n */
        "the device referenced does not exist on this system"
    );
}


/* vim: set ts=8 sw=4 et : */
