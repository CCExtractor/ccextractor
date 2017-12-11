/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2011, 2013 Peter Miller
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
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/enametoolong.h>
#include <libexplain/buffer/enosys.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/gethostname.h>
#include <libexplain/buffer/is_the_null_pointer.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/buffer/software_error.h>
#include <libexplain/explanation.h>
#include <libexplain/host_name_max.h>


static void
explain_buffer_errno_gethostname_system_call(explain_string_buffer_t *sb,
    int errnum, char *data, size_t data_size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "gethostname(data = ");
    explain_buffer_pointer(sb, data);
    explain_string_buffer_puts(sb, ", data_size = ");
    explain_buffer_size_t(sb, data_size);
    explain_string_buffer_putc(sb, ')');
}


static size_t
get_actual_hostname_size(void)
{
#ifdef HAVE_GETHOSTNAME
    size_t name_size = explain_get_host_name_max();
    char *name = malloc(name_size + 1);
    if (name)
    {
        if (gethostname(name, name_size) == 0)
        {
            size_t          result;

            /* paranoia */
            name[name_size] = '\0';

            result = strlen(name);
            free(name);
            return result;
        }
        free(name);
    }
#endif

    /*
     * no idea what the length is
     */
    return 0;
}


static void
explain_buffer_errno_gethostname_explanation(explain_string_buffer_t *sb,
    int errnum, char *data, size_t data_size)
{
    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/gethostname.html
     */
    (void)data;
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "data");
        break;

    case EINVAL:
        if (data && data_size + 1 < get_actual_hostname_size())
        {
          /* OSX/Darwin does this when the buffer is not large enough */
          goto bad_size;
        }

        if (!data)
        {
            explain_buffer_is_the_null_pointer(sb, "data");
            break;
        }
        if ((int)data_size <= 0)
        {
            explain_buffer_einval_too_small(sb, "data_size", data_size);
            break;
        }
        /* fall through... */

    case ENAMETOOLONG:
        /* data_size is smaller than the actual size.  */
        {
            size_t actual_size;
            bad_size:
            /* OSX/Darwin does this when the buffer is not large enough */
            actual_size = get_actual_hostname_size();
            if (actual_size > 0 && data_size < actual_size + 1)
            {
                explain_buffer_enametoolong_gethostname
                (
                    sb,
                    actual_size,
                    "data_size"
                );
            }
            else
            {
                explain_buffer_einval_too_small(sb, "data_size", data_size);
            }
            explain_buffer_software_error(sb);
        }
        break;

    case ENOSYS:
        explain_buffer_enosys_vague(sb, "gethostname");
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "gethostname");
        break;
    }
}


void
explain_buffer_errno_gethostname(explain_string_buffer_t *sb, int errnum,
    char *data, size_t data_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_gethostname_system_call
    (
        &exp.system_call_sb,
        errnum,
        data,
        data_size
    );
    explain_buffer_errno_gethostname_explanation
    (
        &exp.explanation_sb,
        errnum,
        data,
        data_size
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
