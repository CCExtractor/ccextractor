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
#include <libexplain/ac/sys/types.h>
#include <libexplain/ac/grp.h>

#include <libexplain/getgrouplist.h>
#include <libexplain/output.h>


void
explain_getgrouplist_or_die(const char *user, gid_t group, gid_t *groups, int
    *ngroups)
{
    if (explain_getgrouplist_on_error(user, group, groups, ngroups) < 0)
    {
        explain_output_exit_failure();
    }
}


#if 0 /* ifndef HAVE_GETGROUPLIST */

static int
is_a_group_member(struct group *gp, const char *user)
{
    int n;
    for (n = 0; gr->gr_members[n]; ++n)
    {
        if (0 == strcmp(user, gr->gr_members[n])
            return 1;
    }
    rturn 0;
}


static int
is_a_duplicate(gid_t *groups, int groups_size, gid_t gid)
{
    for (n = 0; n < groups_size; ++n)
    {
        if (groups[n] == gid)
            return 1;
    }
    return 0;
}


static int
getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups)
{
    int             n;

    n = 0;
    if (n < *ngroups)
        groups[n++] = gid;
    setgrent();
    for (;;)
    {
        struct group    *gr;

        gr = getgrent();
        if (!gr)
            break;
        if
        (
            is_a_group_member(gr, user)
        &&
            !is_a_duplicate(groups, n, gr->gr_gid)
        )
        {
            if (n < *ngroups)
                groups[n] = gr->gr_gid;
            ++n;
        }

    }
    endgrent();
    if (n > *ngroups)
    {
        *ngroups = n;
        return -1;
    }
    *ngroups = n;
    return n;
}

#endif


int
explain_getgrouplist_on_error(const char *user, gid_t group, gid_t *groups,
    int *ngroups)
{
    int             result;
    int             hold_errno;

    hold_errno = errno;
    errno = 0;
#ifdef HAVE_GETGROUPLIST
    result = getgrouplist(user, group, groups, ngroups);
#else
    errno = ENOSYS;
    result = -1;
#endif
    /*
     * If the number of groups of which user is a member is less than or
     * equal to *ngroups, then the value *ngroups is returned.
     *
     * If the user is a member of more than *ngroups groups, then
     * getgrouplist() returns -1.  In this case the value returned in
     * *ngroups can be used to resize the buffer passed to a further
     * call getgrouplist().
     */
    if (result == -1)
        errno = ERANGE;
    if (errno != 0)
    {
        hold_errno = errno;
        explain_output_error
        (
            "%s",
            explain_errno_getgrouplist(hold_errno, user, group, groups, ngroups)
        );
    }
    errno = hold_errno;
    return result;
}


/* vim: set ts=8 sw=4 et : */
