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

#include <libexplain/ac/assert.h>
#include <libexplain/ac/errno.h>
#include <libexplain/ac/unistd.h>
#include <libexplain/ac/limits.h>

#include <libexplain/buffer/dac.h>
#include <libexplain/buffer/efault.h>
#include <libexplain/buffer/einval.h>
#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/sethostname.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/size_t.h>
#include <libexplain/buffer/string_n.h>
#include <libexplain/explanation.h>
#include <libexplain/host_name_max.h>
#include <libexplain/is_efault.h>


static void
explain_buffer_errno_sethostname_system_call(explain_string_buffer_t *sb,
    int errnum, const char *name, size_t name_size)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "sethostname(name = ");
    explain_buffer_string_n(sb, name, name_size);
    explain_string_buffer_puts(sb, ", name_size = ");
    explain_buffer_size_t(sb, name_size);
    explain_string_buffer_putc(sb, ')');
}


static int
name_is_ok(const char *name, size_t name_size)
{
    const char      *name_end;
    int             state;

    /* The Linux kernel permits zero-length hostname. */
    if (name_size == 0)
        return 1;

    /*
     * using RFC1035 terminology (section 2.3.1)
     *
     * $accept := <subdomain> $end
     * <subdomain> ::= <label>
     * <subdomain> ::= <subdomain> "." <label>
     * <label> ::= <letter>
     * <label> ::= <letter> <let-dig>
     * <label> ::= <letter> <ldh-str> <let-dig>
     * <ldh-str> ::= <let-dig-hyp>
     * <ldh-str> ::= <ldh-str> <let-dig-hyp>
     * <let-dig-hyp> ::= <let-dig>
     * <let-dig-hyp> ::= "-"
     * <let-dig> ::= <letter>
     * <let-dig> ::= <digit>
     */
    name_end = name + name_size;
    state = 0;
    for (;;)
    {
        int             c;

        /*
         * Note: we don't use isdigit, isalpha, etc, because the current
         * locale could be different than the "C" locale.
         *
         * We distinguish end-of-string with -1, because a NUL character
         * would be invalid in a hostname.
         */
        c = (name < name_end ? (unsigned char)*name++ : -1);
        switch (state)
        {
        case 0:
            /*
             * initial state OR after dot
             *
             *                      $accept := @ <subdomain> $end
             *                 <subdomain> ::= @ <subdomain> "." <label>
             *                 <subdomain> ::= @ <label>
             * <subdomain> ::= <subdomain> "." @ <label>
             *                     <label> ::= @ <letter>
             *                     <label> ::= @ <letter> <let-dig>
             *                     <label> ::= @ <letter> <ldh-str> <let-dig>
             */
            switch (c)
            {
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
            case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
            case 's': case 't': case 'u': case 'v': case 'w': case 'x':
            case 'y': case 'z':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
            case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
            case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
            case 'Y': case 'Z':
                state = 1;
                break;

            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                return 0;

            case '-':
                return 0;
                break;

            case '.':
                return 0;
                break;

            case -1:
                return 0;

            default:
                return 0;
            }
            break;

        case 1:
            /*
             * within a label
             *
             *         $accept := <subdomain> @ $end
             *        <subdomain> ::= <label> @
             *    <subdomain> ::= <subdomain> @ "." <label>
             *           <label> ::= <letter> @
             *           <label> ::= <letter> @ <let-dig>
             * <label> ::= <letter> <let-dig> @
             *           <label> ::= <letter> @ <ldh-str> <let-dig>
             * <label> ::= <letter> <ldh-str> @ <let-dig>
             *                  <ldh-str> ::= @ <let-dig-hyp>
             *    <ldh-str> ::= <let-dig-hyp> @
             *                  <ldh-str> ::= @ <ldh-str> <let-dig-hyp>
             *        <ldh-str> ::= <ldh-str> @ <let-dig-hyp>
             *              <let-dig-hyp> ::= @ <let-dig>
             *              <let-dig-hyp> ::= @ "-"
             *                  <let-dig> ::= @ <letter>
             *                  <let-dig> ::= @ <digit>
             */
            switch (c)
            {
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
            case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
            case 's': case 't': case 'u': case 'v': case 'w': case 'x':
            case 'y': case 'z':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
            case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
            case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
            case 'Y': case 'Z':
                state = 1;
                break;

            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                state = 1;
                break;

            case '-':
                state = 2;
                break;

            case '.':
                state = 0;
                break;

            case -1:
                return 1;

            default:
                return 0;
            }
            break;

        case 2:
            /*
             * within label, after hyphen
             *
             *        <label> ::= <letter> <ldh-str> @ <let-dig>
             *           <ldh-str> ::= <let-dig-hyp> @
             *               <ldh-str> ::= <ldh-str> @ <let-dig-hyp>
             * <ldh-str> ::= <ldh-str> <let-dig-hyp> @
             *                 <let-dig-hyp> ::= "-" @
             *                         <let-dig> ::= @ <letter>
             *                         <let-dig> ::= @ <digit>
             */
            switch (c)
            {
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
            case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
            case 's': case 't': case 'u': case 'v': case 'w': case 'x':
            case 'y': case 'z':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
            case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
            case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
            case 'Y': case 'Z':
                state = 1;
                break;

            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                state = 1;
                break;

            case '-':
                state = 2;
                break;

            case '.':
                return 0;

            case -1:
                return 0;

            default:
                return 0;
            }
            break;

        default:
            assert(!"unknown state");
            return 0;
        }
    }
}


static void
explain_buffer_errno_sethostname_explanation(explain_string_buffer_t *sb,
    int errnum, const char *name, size_t name_size)
{
    size_t          host_name_max;
    ssize_t         name_ssize;

    /*
     * http://www.opengroup.org/onlinepubs/009695399/functions/sethostname.html
     */
    (void)name;
    name_ssize = name_size;
    switch (errnum)
    {
    case EFAULT:
        explain_buffer_efault(sb, "name");
        break;

    case EINVAL:
        /* A size of zero is valid. */
        if (name_ssize < 0)
        {
            explain_buffer_einval_too_small(sb, "name_size", name_ssize);
            break;
        }
        host_name_max = explain_get_host_name_max();
        if (host_name_max && name_size > host_name_max)
        {
            /* The Linux kernel checks this */
            goto enametoolong;
        }
        if
        (
            name_size
        &&
            !explain_is_efault_pointer(name, name_size)
        &&
            !name_is_ok(name, name_size)
        )
        {
            /*
             * Note: the Linux kernel does not check this.
             */
            explain_buffer_gettext
            (
                sb,
                /*
                 * xgettext: this error message is issued when a process
                 * attempts to set the hostname, and the hostname contains
                 * characters not in the RFC1035 spec (section 2.3.1).
                 */
                i18n("the hostname specified contains invalid characters")
            );
            break;
        }
        /* just guessing */
        goto enametoolong;

    case ENAMETOOLONG:
#ifdef EOVERFLOW
    case EOVERFLOW:
#endif
        enametoolong:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: this error message is issued when a process
             * attempts to set the hostname, and the name is too long.
             */
            i18n("the hostname specified is too long")
        );
        host_name_max = explain_get_host_name_max();
        if (host_name_max && name_size > host_name_max)
        {
            explain_string_buffer_printf
            (
                sb,
                " (%lu > %lu)",
                (unsigned long)name_size,
                (unsigned long)host_name_max
            );
        }
        break;

    case EPERM:
        explain_buffer_gettext
        (
            sb,
            /*
             * xgettext: this error message is issued when a process
             * attempts to set the hostname without sufficient privilege.
             */
            i18n("the process does not have permission to set the hostname")
        );
        explain_buffer_dac_sys_admin(sb);
        break;

    default:
        explain_buffer_errno_generic(sb, errnum, "sethostname");
        break;
    }
}


void
explain_buffer_errno_sethostname(explain_string_buffer_t *sb, int errnum,
    const char *name, size_t name_size)
{
    explain_explanation_t exp;

    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_sethostname_system_call
    (
        &exp.system_call_sb,
        errnum,
        name,
        name_size
    );
    explain_buffer_errno_sethostname_explanation
    (
        &exp.explanation_sb,
        errnum,
        name,
        name_size
    );
    explain_explanation_assemble(&exp, sb);
}

/* vim: set ts=8 sw=4 et : */
