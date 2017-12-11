/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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

#include <libexplain/ac/string.h>

#include <libexplain/explanation/assemble_common.h>
#include <libexplain/gettext.h>


static long
count_non_format_bytes(const char *msgid)
{
    const char      *msgstr;
    long            result;

    msgstr = explain_gettext(msgid);
    result = 0;
    for (;;)
    {
        unsigned char   c;

        c = *msgstr++;
        if (c == '\0')
            return result;
        if (c != '%')
        {
            ++result;
            continue;
        }

        for (;;)
        {
            c = *msgstr++;
            switch (c)
            {
            case '\0': /*  oops, end of string */
                return result;

            case ' ': /* positive prefix request */
            case '#': /* alternate format request */
            case '$': /* poisition introducer */
            case '\'': /* SUSv2 only: use thousands separator */
                continue;

            case '%': /* literal */
                ++result;
                break;

            case '+': /* positive prefix request */
            case '-': /* left aligned */
            case '.': /* precision intriducer */
            case '0': /* position, width or precision */
            case '1': /* position, width or precision */
            case '2': /* position, width or precision */
            case '3': /* position, width or precision */
            case '4': /* position, width or precision */
            case '5': /* position, width or precision */
            case '6': /* position, width or precision */
            case '7': /* position, width or precision */
            case '8': /* position, width or precision */
            case '9': /* position, width or precision */
                continue;

            case 'A': /* floating point format, hex notation */
            case 'a': /* floating point format, hex notation */
            case 'C': /* SUSv2 only: synonly for %lc */
            case 'c': /* character format */
            case 'd': /* integer format */
            case 'E': /* floating point format */
            case 'e': /* floating point format */
            case 'F': /* floating point format */
            case 'f': /* floating point format */
            case 'G': /* floating point format */
            case 'g': /* floating point format */
                break;

            case 'h': /* short size */
            case 'I': /* use alternate locale digits */
                continue;

            case 'i': /* integer format */
                break;

            case 'j': /* uintmax_t size */
            case 'L': /* long size */
            case 'l': /* long size */
                continue;

            case 'm': /* strerror(errno).  Glibc only */
            case 'n': /* num chars so far */
            case 'o': /* octal format */
            case 'p': /* pointer format */
                break;

            case 'q':
                continue;

            case 's': /* string format */
                break;

            case 't': /* ptrdiff_t size */
                continue;

            case 'X': /* hex format */
            case 'x': /* hex format */
                break;

            case 'z': /* size_t size */
                continue;

            default:
                break;
            }
            break;
        }
    }
    return result;
}


void
explain_explanation_assemble_common(explain_explanation_t *exp,
    const char *strerror_text, explain_string_buffer_t *result)
{
    long            overhead;
    long            prob_len;
    long            exp_len;
    int             err_len;

    if (exp->errnum == 0)
    {
        explain_string_buffer_printf_gettext
        (
            result,
            /*
             * xgettext: this message is issued when a system call
             * succeeds, when there was, in fact, no error.
             *
             * %1$s => the C text of the system call and its arguments
             */
            i18n("%s: success"),
            exp->system_call
        );
        explain_string_buffer_puts(result, exp->footnotes);
        return;
    }

    /*
     * If there is no extended explanation available,
     * use a shortened form of the message.
     */
    prob_len = exp->system_call_sb.position;
    err_len = strlen(strerror_text);
    if (exp->explanation_sb.position == 0)
    {
        /*
         * NOTE: this string MUST be exactly the same as the one used,
         * below, to glue the explanation parts together.
         */
        use_short_form:
        overhead = count_non_format_bytes("%s failed, %s");

        if (prob_len + err_len + overhead > (long)result->maximum)
        {
            long new_len = (long)result->maximum - (prob_len + overhead);
            explain_string_buffer_truncate(&exp->explanation_sb, new_len);
        }

        explain_string_buffer_printf_gettext
        (
            result,
            /*
             * xgettext: this message is printed when there is no
             * extended explanation available.  In english, the stuff
             * to the left of "because" is a statement of the problem,
             * including function name and function argument names and
             * values.
             *
             * Usually a longer message, including a prose explanation, is
             * used.  This shorter message is used when there is no extended
             * explanation, or when the user-supplied message buffer is too
             * small.
             *
             * Depending on the grammar of the natural language being
             * translated to, you may need to rearrange these two pieces
             * using positional arguments.
             *
             * %1$s => the C text of the system call and its arguments
             *         e.g. "open(pathname = "foo/bar", flags = O_RDONLY)"
             * %2$s => the strerror text, plus the name and number of
             *         the errno.h constant
             *         e.g. "No such file or directory (2, ENOENT)"
             */
            i18n("%s failed, %s"),
            exp->system_call,
            strerror_text
        );
        explain_string_buffer_puts(result, exp->footnotes);
        return;
    }

    /*
     * NOTE: this string MUST be exactly the same as the one used
     * below, to glue the explanation parts together.
     */
    overhead = count_non_format_bytes("%s failed, %s because %s");

    exp_len = exp->explanation_sb.position;
    if (prob_len + err_len + exp_len + overhead > (long)result->maximum)
    {
        long new_exp_len =
            (long)result->maximum - (prob_len + err_len + overhead);
        if (new_exp_len <= 0)
        {
            goto use_short_form;
        }
        explain_string_buffer_truncate(&exp->explanation_sb, new_exp_len);
    }

    explain_string_buffer_printf_gettext
    (
        result,
        /*
         * xgettext: This message is used to join the problem to the
         * explanation.  In english, the stuff to the left of "because"
         * is a statement of the problem, including function name and
         * function argument names and values; and the stuff to the
         * right of "because" is the explanation text.
         *
         * Depending on the grammar of the language being translated to,
         * you may need to rearrange these two pieces using positional
         * arguments.
         *
         * %1$s => the C text of the system call and its arguments
         *         e.g. "open(pathname = 'foo/bar', flags = O_RDONLY)"
         * %2$s => the strerror text, plus the name and number of
         *         the errno.h constant
         *         e.g. "No such file or directory (2, ENOENT)"
         * %3$s => the explanation text
         *         e.g. "there is no 'bar' file in the pathname
         *         'foo'; directory"
         *
         * For example:
         *
         * msgid "%s failed, %s because %s"
         * msgstr "%3$s caused %2$s to be returned by %1$s"
         *
         * msgid "%s failed, %s because %s"
         * msgstr "a %2$s error, due to %3$s, was reported by %1$s"
         *
         * This has a follow-on effect for how the explanations themselves
         * are translated, to ensure that sensible sentences result.  In
         * particular, the explanation portion should only ever be one
         * sentence, so that a clause (e.g. above) may be appended.
         */
        i18n("%s failed, %s because %s"),
        exp->system_call,
        strerror_text,
        exp->explanation
    );
    explain_string_buffer_puts(result, exp->footnotes);
}


/* vim: set ts=8 sw=4 et : */
