/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/errno.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/errno/generic.h>
#include <libexplain/buffer/errno/gethostbyname.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/explanation.h>


static void
explain_buffer_errno_gethostbyname_system_call(explain_string_buffer_t *sb,
    int errnum, const char *name)
{
    (void)errnum;
    explain_string_buffer_puts(sb, "gethostbyname(name = ");
    explain_string_buffer_puts_quoted(sb, name);
    explain_string_buffer_putc(sb, ')');
}


/* RFC 2181 */
#define ALLOW_UNDERSCORE 1
#define ALLOW_LEADING_UNDERSCORE 1

/* RFC 1123 */
#define ALLOW_LEADING_HYPHEN 1
#define ALLOW_LEADING_DIGIT 1
#define ALLOW_ALL_NUMERIC 1


typedef struct label label;
struct label
{
    const char      *begin;
    size_t          size;
    const char      *end;
};


/**
  * The valid_label function is used to determien whether or not a
  * hostname label is correctly formed.
  *
  * @param lp
  *     the label in question.
  * @returns
  *     0 (false) if not valid, non-zero (true) if valid.
  */
static int
valid_label(const label *lp)
{
    const char      *name;
    size_t          size;
    size_t          seen_alpha;
    size_t          seen_digit;
    size_t          seen_punct;

    /*
     * Each label must be between 1 and 63 characters long.
     */
    size = lp->size;
    if (size < 1 || size > 63)
        return 0;

    /*
     * The original specification of hostnames in RFC 952, mandated that
     * labels could not start with a digit or with a hyphen, and must
     * not end with a hyphen.  However, a subsequent specification (RFC
     * 1123) permitted hostname labels to start with digits.
     *
     * That means they still can't start with a hyphen.
     */
    name = lp->begin;
    switch (name[0])
    {
#ifndef ALLOW_LEADING_DIGIT
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return 0;
#endif

#ifndef ALLOW_LEADING_HYPHEN
    case '-':
        return 0;
#endif

#ifdef ALLOW_UNDERSCORE
#ifndef ALLOW_LEADING_UNDERSCORE
    case '_':
        /* no *leading* underscore */
        return 0;
#endif
#endif

    default:
        break;
    }

    /*
     * Check each character.
     */
    seen_alpha = 0;
    seen_digit = 0;
    seen_punct = 0;
    while (size-- > 0)
    {
        unsigned char c = *name++;
        switch (c)
        {
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
            /*
             * Hostname labels may contain only the ASCII letters 'a' through
             * 'z' (in a case-insensitive manner), the digits '0' through '9',
             * and the hyphen ('-').
             */
            ++seen_alpha;
            break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            /*
             * The original specification of hostnames in RFC 952, mandated that
             * labels could not start with a digit or with a hyphen, and must
             * not end with a hyphen.  However, a subsequent specification (RFC
             * 1123) permitted hostname labels to start with digits.
             */
            ++seen_digit;
            break;

        case '-':
            if (size == 0)
            {
                /* must not end with a hyphen */
                return 0;
            }
            ++seen_punct;
            break;

#ifdef ALLOW_UNDERSCORE
        case '_':
            if (size == 0)
            {
                /* must not end with an underscore */
                return 0;
            }
            ++seen_punct;
            break;
#endif

        default:
            /*
             * No other symbols, punctuation characters, or white space are
             * permitted.
             */
            return 0;
        }
    }
#ifndef ALLOW_ALL_NUMERIC
    if (seen_digit == lp->size)
        return 0;
#endif

    /*
     * Found nothing wrong.
     */
    return 1;
}


/**
  * The validate_hostname function is used to check a hostname.
  *
  * @param name
  *     The hostname to be checked.
  * @returns
  *     0 if valid; or a field number (1 based) if invalid.
  */
static label *
validate_hostname(const char *name)
{
    static label    labels[(255 + 1) / 2];
    size_t          nlabels;
    size_t          len;
    const char      *name_end;
    label           *lp;
    size_t          j;

    /*
     * Hostnames are composed of series of labels concatenated with
     * dots, as are all domain names. For example, "en.wikipedia.org"
     * is a hostname. Each label must be between 1 and 63 characters
     * long, and the entire hostname (including the delimiting dots)
     * has a maximum of 255 characters.
     *
     * The Internet standards (Request for Comments) for protocols
     * mandate that component hostname labels may contain only the
     * ASCII letters 'a' through 'z' (in a case-insensitive manner),
     * the digits '0' through '9', and the hyphen ('-'). The original
     * specification of hostnames in RFC 952, mandated that labels
     * could not start with a digit or with a hyphen, and must not end
     * with a hyphen. However, a subsequent specification (RFC 1123)
     * permitted hostname labels to start with digits. No other symbols,
     * punctuation characters, or white space are permitted.
     *
     * [http://en.wikipedia.org/wiki/Hostname]
     */
    len = strlen(name);
    if (len < 1 || len > 255)
        return (label *)(-1);
    name_end = name + len;

    nlabels = 0;
    for (;;)
    {
        const char      *dot;

        dot = memchr(name, '.', name_end - name);
        if (!dot)
        {
            lp = &labels[nlabels++];
            lp->begin = name;
            lp->size = name_end - name;
            lp->end = name_end;
            break;
        }
        lp = &labels[nlabels++];
        lp->begin = name;
        lp->size = dot - name;
        lp->end = dot;
        name = dot + 1;
    }

    /*
     * If a trailing dot was used, the last "label" will be empty, so toss it.
     * No other labels are permitted to be empty.
     */
    if (nlabels > 0 && labels[nlabels - 1].size == 0)
        --nlabels;
    if (nlabels == 0)
        return (label *)(-1);

    /*
     * Check each label for validity.
     */
    for (j = 0; j < nlabels; ++j)
    {
        lp = &labels[j];
        if (!valid_label(lp))
            return lp;
    }

    /*
     * Couldn't find anything wrong.
     */
    return 0;
}


static int
check_valid_hostname(explain_string_buffer_t *sb, const char *name)
{
    label           *lp;
    explain_string_buffer_t sb2;
    char            buffer[1000];

    lp = validate_hostname(name);
    if (!lp)
        return 0;
    if (sb == sb->footnotes)
        explain_string_buffer_puts(sb, "; ");
    if (lp == (label *)(-1))
    {
        explain_buffer_gettext
        (
            sb,
            i18n
            (
                /*
                 * xgettext: this error message is used to explain a
                 * NO_RECOVERY error returned by the gethostbyname system
                 * call, in the case where the host name is not a valid
                 * length.
                 */
                "the host name is not a valid length"
            )
        );
        return 1;
    }

    if (lp->size == 0)
    {
        explain_buffer_gettext
        (
            sb,

            /*
             * xgettext: this error message is used to explain a
             * NO_RECOVERY error returned by the gethostbyname
             * system call, in the case where the host name contains
             * at least one empty label.
             */
            i18n("host names may not have empty parts")
        );
        return 1;
    }

    explain_string_buffer_init(&sb2, buffer, sizeof(buffer));
    explain_string_buffer_puts_quoted_n(&sb2, lp->begin, lp->size);
    explain_string_buffer_printf_gettext
    (
        sb,
        i18n
        (
            /*
             * xgettext: this error message is used to explain a NO_RECOVERY
             * error returned by the gethostbyname system call, in the case
             * where the host name is not properly formed.
             *
             * %1$s => text of the label (the text between the dots)
             */
            "the host name (at %s) is not properly formed"
        ),
        buffer
    );
    return 1;
}


void
explain_buffer_errno_gethostbyname_explanation(explain_string_buffer_t *sb,
    int errnum, const char *syscall_name, const char *name)
{
    /*
    * http://www.opengroup.org/onlinepubs/009695399/functions/gethostbyname.html
     *
     * In this case, errnum has the h_error value, not the errno value.
     */
    switch (errnum)
    {
    case HOST_NOT_FOUND:
        /*
         * FIXME: what about if it is shorter, e.g. www.example.com vs
         * example.com.  We need to let the user know what part of the name is
         * not found.
         */
        explain_buffer_gettext
        (
            sb,
            i18n
            (
                /*
                 * xgettext: this error message is used to explain
                 * HOST_NOT_FOUND errors returned by the gethostbyname system
                 * call.  Authoritative Answer Host not found.  The specified
                 * host is unknown.
                 */
                "an authoritative DNS server was reached and the given host "
                "name does not exist"
            )
        );
        check_valid_hostname(sb->footnotes, name);
        break;

    case TRY_AGAIN:
        /*
         * FIXME: what about if it is shorter, e.g. work1.fixed.example.com
         * vs fixed.example.com. vs example.com.  We need to let the user know
         * what part of the name is not found.
         */
        explain_buffer_gettext
        (
            sb,
            i18n
            (
                /*
                 * xgettext: this error message is used to explain TRY_AGAIN
                 * errors returned by the gethostbyname system call.
                 * Authoritative Answer Host not found, or SERVERFAIL.  A
                 * temporary error occurred on an authoritative name server.
                 * The specified host is unknown.
                 */
                "an authoritative DNS server could not be reached and so the "
                "given host name does not appear to exist"
            )
        );
        check_valid_hostname(sb->footnotes, name);
        /* FIXME: i18n */
        explain_string_buffer_puts(sb->footnotes, "try again later");
        break;

    case NO_RECOVERY:
        /* Non recoverable error: FORMERR */
        if (check_valid_hostname(sb, name))
            break;

        /* FIXME: which? */
        explain_buffer_gettext
        (
            sb,
            i18n
            (
                /*
                 * xgettext: this error message is used to explain NO_RECOVERY
                 * errors returned by the gethostbyname system call.
                 * Non recoverable errors, FORMERR, REFUSED, NOTIMP.
                 */
                "the operation was refused, or "
                "the operation is not implemented on this system"
            )
        );
        break;

    case NO_DATA:
        explain_buffer_gettext
        (
            sb,
            i18n
            (
                /*
                 * xgettext: this error message is used to explain NO_DATA
                 * errors returned by the gethostbyname system call.
                 * Valid name, no data record of requested type.
                 * The requested name is valid but does not have an IP address.
                 */
                "the host name does not have any DNS data"
            )
        );
        check_valid_hostname(sb->footnotes, name);
        break;

#if defined(NO_ADDRESS) && defined(NO_DATA) && NO_ADDRESS != NO_DATA
    case NO_ADDRESS:
        explain_buffer_gettext
        (
            sb,
            i18n
            (
                /*
                 * xgettext: this error message is used to explain NO_ADDRESS
                 * errors returned by the gethostbyname system call.
                 * No address, look for MX record.
                 */
                "the host name has DNS data but does not have an IP address"
            )
        );
        explain_buffer_puts(sb->footnote, "look for an MX record");
        check_value_hostname(sb->footnotes, name);
        break;
#endif

#ifdef NETDB_INTERNAL
    case NETDB_INTERNAL:
#endif
    default:
        explain_buffer_errno_generic(sb, errno, syscall_name);
        break;
    }
}


void
explain_buffer_errno_gethostbyname(explain_string_buffer_t *sb, int errnum,
    const char *name)
{
    explain_explanation_t exp;
    int                   hold_errno;

    hold_errno = errno;
    explain_explanation_init(&exp, errnum);
    explain_buffer_errno_gethostbyname_system_call
    (
        &exp.system_call_sb,
        errnum,
        name
    );
    explain_buffer_errno_gethostbyname_explanation
    (
        &exp.explanation_sb,
        errnum,
        "gethostbyname",
        name
    );
    errno = hold_errno; /* for NETDB_INTERNAL error case */
    explain_explanation_assemble_netdb(&exp, sb);
}


/* vim: set ts=8 sw=4 et : */
