/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/gettext.h>
#include <libexplain/translate.h>


const char *
explain_translate_effective_gid(void)
{
    return
        explain_gettext
        (
            /*
             * xgettext: This phrase is used to distinguish which of the
             * process's GIDs are in use during the permissions check.  In this
             * case, the effective GID.
             */
            i18n("effective GID")
        );
}


/* vim: set ts=8 sw=4 et : */
