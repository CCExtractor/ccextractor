/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2012, 2013 Peter Miller
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

#include <libexplain/ac/grp.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>

#include <libexplain/fstrcmp.h>
#include <libexplain/output.h>
#include <libexplain/string_to_thing.h>


gid_t
explain_parse_gid_t_or_die(const char *text)
{
    /* see if it is a group name */
    {
        struct group    *gr;

        setgrent();
        gr = getgrnam(text);
        if (gr)
            return gr->gr_gid;
    }

    /* see if it is a number */
    {
        char            *ep;
        long            result;

        result = strtol(text, &ep, 0);
        if (ep != text && *ep == '\0')
            return result;
    }

    /* fuzzy name match for nicer error messages */
    {
        double          best_weight = 0.6;
        int             best_gid = -1;
        char            best_name[100];

        setgrent();
        for (;;)
        {
            double          w;
            struct group    *gr;

            gr = getgrent();
            if (!gr)
                break;
            w = explain_fstrcmp(text, gr->gr_name);
            if (w > best_weight)
            {
                best_weight = w;
                explain_strendcpy
                (
                    best_name,
                    gr->gr_name,
                    best_name + sizeof(best_name)
                );
                best_gid = gr->gr_gid;
            }
        }
        if (best_gid > 0)
        {
            explain_output_error_and_die
            (
                /* FIXME: i18n */
                "unable to interpret \"%s\" as a group name, "
                    "did you mean the \"%s\" group instead?",
                text,
                best_name
            );
        }
        else
        {
            explain_output_error_and_die
            (
                /* FIXME: i18n */
                "unable to interpret \"%s\" as a group name",
                text
            );
        }
    }

    /* I give up */
    return -1;
}


/* vim: set ts=8 sw=4 et : */
