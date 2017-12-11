/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2012, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>

#include <libexplain/buffer/eacces.h>
#include <libexplain/buffer/eloop.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enoent.h>
#include <libexplain/buffer/enotdir.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/path_resolution.h>
#include <libexplain/buffer/errno/pathconf.h>
#include <libexplain/buffer/pathconf_name.h>
#include <libexplain/buffer/pathname.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_pathconf_system_call(explain_string_buffer_t *sb,
    int errnum, const char *pathname, int name)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "pathconf(pathname = ");
    explain_buffer_pathname(sb, pathname);
    explain_string_buffer_puts(sb, ", name = ");
    explain_buffer_pathconf_name(sb, name);
    explain_string_buffer_putc(sb, ')');
}


void
explain_buffer_pathconf_einval(explain_string_buffer_t *sb,
    const char *arg1_caption, int name, const char *name_caption)
{
    if (!explain_valid_pathconf_name(name))
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an EINVAL error
             * reported by the pathconf system call.
             *
             * %1$s => the name of the offending system call argument.
             */
            i18n("%s does not refer to a known file configuration value"),
            name_caption
        );
    }
    else
    {
        explain_string_buffer_printf_gettext
        (
            sb,
            /*
             * xgettext: This message is used to explain an EINVAL error
             * reported by the pathconf system call.
             *
             * %1$s => the name of the system call argument containing
             *         the 'name' selector, e.g. _PC_NAME_MAX
             * %2$s => the name of the first argument, "pathname" or "fildes"
             */
            i18n("the implementation does not support an association of "
                "%s with %s"),
            name_caption,
            arg1_caption
        );
    }
}


static void
explain_buffer_errno_pathconf_explanation(explain_string_buffer_t *sb,
    int errnum, const char *pathname, int name)
{
    explain_final_t final_component;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/pathconf.html
     */
    (void)name;
    explain_final_init(&final_component);
    switch (errnum)
    {
    case ELOOP:
        explain_buffer_eloop(sb, pathname, "pathname", &final_component);
        break;

    case EACCES:
        explain_buffer_eacces(sb, pathname, "pathname", &final_component);
        break;

    case EINVAL:
    case ENOSYS: /* many systems say this for EINVAL */
#ifdef EOPNOTSUPP
    case EOPNOTSUPP: /* HPUX says this for EINVAL */
#endif
        explain_buffer_pathconf_einval(sb, "pathname", name, "name");
        break;

    case ENAMETOOLONG:
        explain_buffer_enametoolong
        (
            sb,
            pathname,
            "pathname",
            &final_component
        );
        break;

    case ENOENT:
        explain_buffer_enoent(sb, pathname, "pathname", &final_component);
        break;

    case ENOTDIR:
        explain_buffer_enotdir(sb, pathname, "pathname", &final_component);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "pathconf");
        break;
    }
}


void
explain_buffer_errno_pathconf(explain_string_buffer_t *sb, int errnum,
    const char *pathname, int name)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_pathconf_system_call
    (
        &exp.system_call_sb,
        errnum,
        pathname,
        name
    );
    explain_buffer_errno_pathconf_explanation
    (
        &exp.explanation_sb,
        errnum,
        pathname,
        name
    );
    explain_explanation_assemble(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
