/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/ctype.h>
#include <libexplain/ac/mntent.h>
#include <libexplain/ac/stdlib.h>

#include <libexplain/gcc_attributes.h>


#ifdef HAVE_GETMNTINFO
#include <stdio.h>
#include <sys/mount.h>

/* FreeBSD, and probably others */

static struct statfs *data;
static int      data_size;
static int      data_pos;


FILE *
setmntent(const char *filename, const char *mode)
{
    static FILE junk;

    (void)filename;
    (void)mode;
    int flags = 0; /* do not hang on broken mounts */
    data_size = getmntinfo(&data, flags);
    data_pos = 0;
    return &junk;
}


struct mntent *
getmntent(FILE *fp)
{
    static struct mntent fudge;
    struct statfs   *p;

    (void)fp;
    if (data_pos >= data_size)
        return 0;
    p = data + data_pos++;
    fudge.mnt_fsname = p->f_mntfromname;
    fudge.mnt_dir = p->f_mntonname;
    fudge.mnt_type = p->f_fstypename;
    fudge.mnt_opts = "";
    fudge.mnt_freq = 0;
    fudge.mnt_passno = 0;
    return &fudge;
}


int
endmntent(FILE *fp)
{
    (void)fp;
    data = 0;
    data_size = 0;
    data_pos = 0;
    return 0;
}

#elif !defined(HAVE_SETMNTENT) && \
      !defined(HAVE_GETMNTENT) && \
      !defined(HAVE_ENDMNTENT)


typedef struct bogus_t bogus_t;
struct bogus_t
{
    FILE            *fp;
    char            line[1000];
    struct mntent   data;
};


LIBEXPLAIN_LINKAGE_HIDDEN
FILE *
setmntent(const char *filename, const char *mode)
{
    bogus_t         *bp;

    bp = malloc(sizeof(bogus_t));
    if (!bp)
        return 0;
    bp->fp = fopen(filename, mode);
    return (FILE *)bp;
}


LIBEXPLAIN_LINKAGE_HIDDEN
struct mntent *
getmntent(FILE *p)
{
    bogus_t         *bp;

    if (!p)
        return 0;
    bp = (bogus_t *)p;
    if (!bp->fp)
        return 0;

    for (;;)
    {
        char            *lp;
        int             ac;
        char            *av[6];

        if (!fgets(bp->line, sizeof(bp->line), bp->fp))
            return 0;
        lp = bp->line;
        ac = 0;
        while (ac < 6)
        {
            if (!*lp)
                break;
            if (isspace((unsigned char)*lp))
            {
                ++lp;
                continue;
            }
            if (*lp == '#')
            {
                while (*lp)
                    ++lp;
                break;
            }
            av[ac++] = lp;
            while (*lp && !isspace((unsigned char)*lp))
                ++lp;
            if (!*lp)
                break;
            *lp++ = '\0';
        }
        if (ac < 2)
            continue;
        while (ac < 6)
            av[ac++] = lp;
        bp->data.mnt_fsname = av[0];
#ifdef __solaris__
        bp->data.mnt_dir = av[2];
        bp->data.mnt_type = av[3];
        bp->data.mnt_opts = av[4];
        bp->data.mnt_freq = 0;
        bp->data.mnt_passno = 0;
#else
        bp->data.mnt_dir = av[1];
        bp->data.mnt_type = av[2];
        bp->data.mnt_opts = av[3];
        bp->data.mnt_freq = atoi(av[4]);
        bp->data.mnt_passno = atoi(av[5]);
#endif
        return &bp->data;
    }
}


LIBEXPLAIN_LINKAGE_HIDDEN
int
endmntent(FILE *p)
{
    if (p)
    {
        bogus_t         *bp;

        bp = (bogus_t *)p;
        if (bp->fp)
        {
            fclose(bp->fp);
            bp->fp = 0;
        }
        free(bp);
    }
    return 0;
}

#endif /* HAVE_MNTENT_H */

#ifndef HAVE_HASMNTOPT

#include <libexplain/ac/string.h>

LIBEXPLAIN_LINKAGE_HIDDEN
char *
hasmntopt(const struct mntent *mnt, const char *opt)
{
    const char      *cp;
    size_t          opt_len;

    opt_len = strlen(opt);
    cp = mnt->mnt_opts;
    for (;;)
    {
        unsigned char   c;
        const char      *start;
        const char      *end;
        size_t          len;

        c = *cp;
        if (!cp)
            return 0;
        if (isspace(c) || c == ',')
        {
            ++cp;
            continue;
        }
        start = cp;
        for (;;)
        {
            ++cp;
            c = *cp;
            if (!c || isspace(c) || c == ',')
                break;
        }
        end = cp;
        len = end - start;
        if
        (
            opt_len < len
        &&
            start[opt_len] == '='
        &&
            0 == memcmp(opt, start, opt_len)
        )
            return (char *)start;
        if (opt_len == len && 0 == memcmp(opt, start, opt_len))
            return (char *)start;
    }
}

#endif


/* vim: set ts=8 sw=4 et : */
