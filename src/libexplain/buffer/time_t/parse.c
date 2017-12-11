/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2012, 2013 Peter Miller
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

#include <libexplain/ac/assert.h>
#include <libexplain/ac/ctype.h>
#include <libexplain/ac/stdio.h>
#include <libexplain/ac/stdlib.h>
#include <libexplain/ac/string.h>

#include <libexplain/buffer/time_t.h>
#include <libexplain/output.h>
#include <libexplain/sizeof.h>


#define YEAR_BIT  (1 << 0)
#define MONTH_BIT (1 << 1)
#define MDAY_BIT  (1 << 2)
#define HOUR_BIT  (1 << 3)
#define MIN_BIT   (1 << 4)
#define SEC_BIT   (1 << 5)


static int
all_digits(const char *text)
{
    if (!*text)
        return 0;
    for (;;)
    {
        unsigned char c = *text++;
        if (!c)
            return 1;
        if (!isdigit(c))
            return 0;
    }
}


static unsigned
pull(const char *text, size_t len)
{
    unsigned        n;

    n = 0;
    while (len > 0)
    {
        unsigned char c = *text++;
        assert(isdigit(c));
        n = n * 10 + c - '0';
        --len;
    }
    return n;
}


static int
correct_year(const struct tm *build, int year)
{
    int             century;

    if (year >= 100)
        return (year - 1900);
    century = 0;
    for (;;)
    {
        if (abs(year + century - build->tm_year) <= 50)
            return (year + century);
        century += 100;
    }
}


static time_t
explain_parse_time_t(const char *text)
{
    static struct tm tzero;
    struct tm       build;
    unsigned        numbers[6];
    unsigned char   masks[SIZEOF(numbers)];
    time_t          now;
    unsigned        nnum;

    build = tzero;
    if (time(&now) != (time_t)(-1))
    {
        struct tm       *tmp;

        tmp = localtime(&now);
        if (tmp)
            build = *tmp;
    }

    if (all_digits(text))
    {
        size_t          len;

        len = strlen(text);
        if (len == 12)
        {
            build.tm_year = correct_year(&build, pull(text, 2));
            build.tm_mon = pull(text + 2, 2) - 1;
            if (build.tm_mon < 0 || build.tm_mon >= 12)
                return -1;
            build.tm_mday = pull(text + 4, 2);
            if (build.tm_mday < 1 || build.tm_mday > 31)
                return -1;
            build.tm_hour = pull(text + 6, 2);
            if (build.tm_hour >= 24)
                return -1;
            build.tm_min = pull(text + 8, 2);
            if (build.tm_min >= 60)
                return -1;
            build.tm_sec = pull(text + 10, 2);
            if (build.tm_sec >= 62)
                return -1;
            return mktime(&build);
        }

        if (len == 14)
        {
            build.tm_year = pull(text, 4) - 1900;
            if (build.tm_year < 0 || build.tm_year > 200)
                return -1;
            build.tm_mon = pull(text + 4, 2) - 1;
            if (build.tm_mon < 0 || build.tm_mon >= 12)
                return -1;
            build.tm_mday = pull(text + 6, 2);
            if (build.tm_mday < 1 || build.tm_mday > 31)
                return -1;
            build.tm_hour = pull(text + 8, 2);
            if (build.tm_hour >= 24)
                return -1;
            build.tm_min = pull(text + 10, 2);
            if (build.tm_min >= 60)
                return -1;
            build.tm_sec = pull(text + 12, 2);
            if (build.tm_sec >= 62)
                return -1;
            return mktime(&build);
        }

        return strtol(text, 0, 10);
    }

    /*
     * Break the string into a bunch of numbers,
     * looking for patterns.
     */
    nnum = 0;
    while (nnum < SIZEOF(numbers))
    {
        unsigned char c = *text++;
        if (!c)
            break;
        switch (c)
        {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            {
                unsigned        n;
                unsigned char   mask;
                unsigned        ndig;

                n = 0;
                ndig = 0;
                for (;;)
                {
                    n = n * 10 + c - '0';
                    ++ndig;
                    c = *text++;
                    switch (c)
                    {
                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                        continue;

                    default:
                        --text;
                        break;
                    }
                    break;
                }

                /*
                 * If the number is stupidly large just bail,
                 * because it can't be any part of a valid date.
                 */
                if (n >= 2100)
                    return -1;

                mask = 0;
                if (n < 100 && ndig == 2)
                    mask |= YEAR_BIT;
                if (n >= 1900 && n < 2100)
                    mask |= YEAR_BIT;
                if (n >= 1 && n <= 12)
                    mask |= MONTH_BIT;
                if (n >= 1 && n <= 31)
                    mask |= MDAY_BIT;
                if (ndig == 2 && n < 24)
                    mask |= HOUR_BIT;
                if (ndig == 2 && n < 60)
                    mask |= MIN_BIT;
                if (ndig == 2 && n <= 61)
                    mask |= SEC_BIT;

                if (mask)
                {
                    numbers[nnum] = n;
                    masks[nnum] = mask;
                    ++nnum;
                }
            }
            break;

        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
            {
                char            name[30];
                char            *cp;
                struct tm       probe;
                int             m;

                cp = name;
                for (;;)
                {
                    if (cp < ENDOF(name) - 1)
                        *cp++ = c;
                    c = *text++;
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
                        continue;

                    default:
                        --text;
                        break;
                    }
                    break;
                }
                *cp = '\0';

                /*
                 * Now look at possibilities for the month name.  If
                 * name doesn't match anything, throw it away (it is
                 * probably a weekday name or a timezone name).
                 */
                probe = tzero;
                probe.tm_year = 80;
                for (m = 1; m <= 12; ++m)
                {
                    char            candidate[sizeof(name)];

                    probe.tm_mon = m - 1;
                    strftime(candidate, sizeof(candidate), "%b", &probe);
                    if (strcasecmp(name, candidate) == 0)
                    {
                        numbers[nnum] = m;
                        masks[nnum] = MONTH_BIT;
                        ++nnum;
                        break;
                    }
                    strftime(candidate, sizeof(candidate), "%B", &probe);
                    if (strcasecmp(name, candidate) == 0)
                    {
                        numbers[nnum] = m;
                        masks[nnum] = MONTH_BIT;
                        ++nnum;
                        break;
                    }
                }
            }
            break;

        default:
            /* throw away all white space and punctuation */
            break;
        }
    }

#if 0
    {
        unsigned        n;

        /*
         * This debug is too useful to just throw away.
         */
        for (n = 0; n < nnum; ++n)
        {
            char m[10];
            char *mp = m;
            int mask = masks[n];
            if (mask & YEAR_BIT) *mp++ = 'Y';
            if (mask & MONTH_BIT) *mp++ = 'm';
            if (mask & MDAY_BIT) *mp++ = 'd';
            if (mask & HOUR_BIT) *mp++ = 'H';
            if (mask & MIN_BIT) *mp++ = 'M';
            if (mask & SEC_BIT) *mp++ = 'S';
            *mp = '\0';
            fprintf
            (
                stderr,
                "%s: %d: %4d %s\n",
                __FILE__,
                __LINE__,
                numbers[n],
                m
            );
        }
    }
#endif

    if (nnum == 6)
    {
        /* %Y%m%d%H%M%S */
        if
        (
            (masks[0] & YEAR_BIT)
        &&
            (masks[1] & MONTH_BIT)
        &&
            (masks[2] & MDAY_BIT)
        &&
            (masks[3] & HOUR_BIT)
        &&
            (masks[4] & MIN_BIT)
        &&
            (masks[5] & SEC_BIT)
        )
        {
            build.tm_year = correct_year(&build, numbers[0]);
            build.tm_mon = numbers[1] - 1;
            build.tm_mday = numbers[2];
            build.tm_hour = numbers[3];
            build.tm_min = numbers[4];
            build.tm_sec = numbers[5];
            return mktime(&build);
        }

        /* %m%d%Y%H%M%S */
        if
        (
            ((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            (masks[2] & YEAR_BIT)
        &&
            (masks[3] & HOUR_BIT)
        &&
            (masks[4] & MIN_BIT)
        &&
            (masks[5] & SEC_BIT)
        )
        {
            build.tm_mon = numbers[0] - 1;
            build.tm_mday = numbers[1];
            build.tm_year = correct_year(&build, numbers[2]);
            build.tm_hour = numbers[3];
            build.tm_min = numbers[4];
            build.tm_sec = numbers[5];
            return mktime(&build);
        }

        /* %d%m%Y%H%M%S */
        if
        (
            ((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            (masks[2] & YEAR_BIT)
        &&
            (masks[3] & HOUR_BIT)
        &&
            (masks[4] & MIN_BIT)
        &&
            (masks[5] & SEC_BIT)
        )
        {
            build.tm_mday = numbers[0];
            build.tm_mon = numbers[1] - 1;
            build.tm_year = correct_year(&build, numbers[2]);
            build.tm_hour = numbers[3];
            build.tm_min = numbers[4];
            build.tm_sec = numbers[5];
            return mktime(&build);
        }

        /* %d%m%H%M%S%Y */
        if
        (
            ((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            (masks[2] & HOUR_BIT)
        &&
            (masks[3] & MIN_BIT)
        &&
            (masks[4] & SEC_BIT)
        &&
            (masks[5] & YEAR_BIT)
        )
        {
            build.tm_mday = numbers[0];
            build.tm_mon = numbers[1] - 1;
            build.tm_hour = numbers[2];
            build.tm_min = numbers[3];
            build.tm_sec = numbers[4];
            build.tm_year = correct_year(&build, numbers[5]);
            return mktime(&build);
        }

        /* %m%d%H%M%S%Y */
        if
        (
            ((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            (masks[2] & HOUR_BIT)
        &&
            (masks[3] & MIN_BIT)
        &&
            (masks[4] & SEC_BIT)
        &&
            (masks[5] & YEAR_BIT)
        )
        {
            build.tm_mon = numbers[0] - 1;
            build.tm_mday = numbers[1];
            build.tm_hour = numbers[2];
            build.tm_min = numbers[3];
            build.tm_sec = numbers[4];
            build.tm_year = correct_year(&build, numbers[5]);
            return mktime(&build);
        }

        /* dunno */
        return -1;
    }
    if (nnum == 5)
    {
        /* %Y%m%d%H%M */
        if
        (
            (masks[0] & YEAR_BIT)
        &&
            (masks[1] & MONTH_BIT)
        &&
            (masks[2] & MDAY_BIT)
        &&
            (masks[3] & HOUR_BIT)
        &&
            (masks[4] & MIN_BIT)
        )
        {
            build.tm_year = correct_year(&build, numbers[0]);
            build.tm_mon = numbers[1] - 1;
            build.tm_mday = numbers[2];
            build.tm_hour = numbers[3];
            build.tm_min = numbers[4];
            build.tm_sec = 0;
            return mktime(&build);
        }

        /* %m%d%Y%H%M */
        if
        (
            ((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            (masks[2] & YEAR_BIT)
        &&
            (masks[3] & HOUR_BIT)
        &&
            (masks[4] & MIN_BIT)
        )
        {
            build.tm_mon = numbers[0] - 1;
            build.tm_mday = numbers[1];
            build.tm_year = correct_year(&build, numbers[2]);
            build.tm_hour = numbers[3];
            build.tm_min = numbers[4];
            build.tm_sec = 0;
            return mktime(&build);
        }

        /* %d%m%Y%H%M */
        if
        (
            ((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            (masks[2] & YEAR_BIT)
        &&
            (masks[3] & HOUR_BIT)
        &&
            (masks[4] & MIN_BIT)
        )
        {
            build.tm_mday = numbers[0];
            build.tm_mon = numbers[1] - 1;
            build.tm_year = correct_year(&build, numbers[2]);
            build.tm_hour = numbers[3];
            build.tm_min = numbers[4];
            build.tm_sec = 0;
            return mktime(&build);
        }

        /* %d%m%H%M%Y */
        if
        (
            ((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            (masks[2] & HOUR_BIT)
        &&
            (masks[3] & MIN_BIT)
        &&
            (masks[4] & YEAR_BIT)
        )
        {
            build.tm_mday = numbers[0];
            build.tm_mon = numbers[1] - 1;
            build.tm_hour = numbers[2];
            build.tm_min = numbers[3];
            build.tm_sec = 0;
            build.tm_year = correct_year(&build, numbers[4]);
            return mktime(&build);
        }

        /* %m%d%H%M%Y */
        if
        (
            ((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            (masks[2] & HOUR_BIT)
        &&
            (masks[3] & MIN_BIT)
        &&
            (masks[4] & YEAR_BIT)
        )
        {
            build.tm_mon = numbers[0] - 1;
            build.tm_mday = numbers[1];
            build.tm_hour = numbers[2];
            build.tm_min = numbers[3];
            build.tm_sec = 0;
            build.tm_year = correct_year(&build, numbers[4]);
            return mktime(&build);
        }

        /* dunno */
        return -1;
    }
    if (nnum == 3)
    {
        /* %Y%m%d */
        if
        (
            (masks[0] & YEAR_BIT)
        &&
            (masks[1] & MONTH_BIT)
        &&
            (masks[2] & MDAY_BIT)
        )
        {
            build.tm_year = correct_year(&build, numbers[0]);
            build.tm_mon = numbers[1] - 1;
            build.tm_mday = numbers[2];
            return mktime(&build);
        }

        /* %d%m%Y */
        if
        (
            ((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            (masks[2] & YEAR_BIT)
        )
        {
            build.tm_mday = numbers[0];
            build.tm_mon = numbers[1] - 1;
            build.tm_year = correct_year(&build, numbers[2]);
            return mktime(&build);
        }

        /* %m%d%Y */
        if
        (
            ((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            (masks[2] & YEAR_BIT)
        )
        {
            build.tm_mday = numbers[0];
            build.tm_mon = numbers[1] - 1;
            build.tm_year = correct_year(&build, numbers[2]);
            return mktime(&build);
        }

        /* %H%M%S */
        if
        (
            (masks[0] & HOUR_BIT)
        &&
            (masks[1] & MIN_BIT)
        &&
            (masks[2] & SEC_BIT)
        )
        {
            build.tm_hour = numbers[0];
            build.tm_min = numbers[1];
            build.tm_sec = numbers[2];
            return mktime(&build);
        }

        /* dunno */
        return -1;
    }

    if (nnum == 2)
    {
        /* %H%M */
        if ((masks[0] & HOUR_BIT) && (masks[1] & MIN_BIT))
        {
            build.tm_hour = numbers[0];
            build.tm_min = numbers[1];
            build.tm_sec = 0;
            return mktime(&build);
        }

        /* %m%d */
        if
        (
            ((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        )
        {
            build.tm_mon = numbers[0] - 1;
            build.tm_mday = numbers[1];
            return mktime(&build);
        }

        /* %d%m */
        if
        (
            ((masks[0] & MDAY_BIT) && (masks[1] & MONTH_BIT))
        &&
            /* avoid ambiguity */
            !((masks[0] & MONTH_BIT) && (masks[1] & MDAY_BIT))
        )
        {
            build.tm_mday = numbers[0];
            build.tm_mon = numbers[1] - 1;
            return mktime(&build);
        }

        /* dunno */
        return -1;
    }

    /* dunno */
    return -1;
}


static time_t
explain_parse_time_t_on_error(const char *text, const char *caption)
{
    time_t          result;

    result = explain_parse_time_t(text);
    if (result == (time_t)(-1))
    {
        char            qtext[900];
        explain_string_buffer_t qtext_sb;
        char            message[1000];
        explain_string_buffer_t message_sb;

        explain_string_buffer_init(&qtext_sb, qtext, sizeof(qtext));
        explain_string_buffer_puts_quoted(&qtext_sb, text);
        explain_string_buffer_init(&message_sb, message, sizeof(message));

        /* FIXME: i18n */
        if (caption && *caption)
        {
            explain_string_buffer_printf
            (
                &message_sb,
                "%s: can't parse %s into a date and time",
                caption,
                qtext
            );
        }
        else
        {
            explain_string_buffer_printf
            (
                &message_sb,
                "unable to parse %s into a date and time",
                qtext
            );
        }

        explain_output_error("%s", message);
    }
    return result;
}


time_t
explain_parse_time_t_or_die(const char *text, const char *caption)
{
    time_t          result;

    result = explain_parse_time_t_on_error(text, caption);
    if (result == (time_t)(-1))
    {
        explain_output_exit_failure();
    }
    return result;
}


/* vim: set ts=8 sw=4 et : */
