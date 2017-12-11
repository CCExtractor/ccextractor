/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2011, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
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

#ifndef LIBEXPLAIN_AC_MNTENT_H
#define LIBEXPLAIN_AC_MNTENT_H

/**
  * @file
  * @brief Insulate <mntent.h> and <sys/mnttab.h> differences
  */

#include <libexplain/ac/stdio.h>

#if HAVE_MNTENT_H
#include <mntent.h>
#endif
#ifdef HAVE_SYS_MNTENT_H
#include <sys/mntent.h>
#endif
#ifdef HAVE_SYS_MNTTAB_H
#include <sys/mnttab.h>
#endif

#if !defined(HAVE_MNTENT_H) && !defined(HAVE_SYS_MNTTAB_H)

struct mntent
{
    char            *mnt_fsname;    /* Device or server for filesystem.  */
    char            *mnt_dir;       /* Directory mounted on.  */
    char            *mnt_type;      /* Type of filesystem: ufs, nfs, etc.  */
    char            *mnt_opts;      /* Comma-separated options for fs.  */
    int             mnt_freq;       /* Dump frequency (in days).  */
    int             mnt_passno;     /* Pass number for `fsck'.  */
};

FILE *setmntent(const char *filename, const char *mode);
struct mntent *getmntent(FILE *fp);
int endmntent(FILE *fp);

#endif

#ifndef MOUNTED
# ifdef _PATH_MOUNTED
#  define MOUNTED _PATH_MOUNTED
# else
#  ifdef MNTTAB
#   define MOUNTED MNTTAB
#  else
#   define MOUNTED "/etc/mtab"
#  endif
# endif
#endif

/* Solaris is fugly. */
#if defined(HAVE_GETMNTENT) && !defined(HAVE_SETMNTENT)
#define setmntent fopen
#define endmntent fclose
#endif

#endif /* LIBEXPLAIN_AC_MNTENT_H */
/* vim: set ts=8 sw=4 et : */
