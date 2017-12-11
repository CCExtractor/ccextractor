/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2012, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_BUFFER_DAC_H
#define LIBEXPLAIN_BUFFER_DAC_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_dac_chown function may be used to explain the
  * absence of the DAC_CHOWN capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_chown(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_override function may be used to
  * explain the absence of the DAC_OVERRIDE capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_override(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_read_search function may be used to
  * explain the absence of the DAC_READ_SEARCH capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_read_search(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_fowner function may be used to
  * explain the absence of the DAC_FOWNER capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_fowner(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_fsetid function may be used to
  * explain the absence of the DAC_FSETID capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_fsetid(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_kill function may be used to
  * explain the absence of the CAP_KILL capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_kill(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_setgid function may be used to
  * explain the absence of the CAP_SETGID capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_setgid(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_setuid function may be used to
  * explain the absence of the CAP_SETUID capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_setuid(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_setpcap function may be used to
  * explain the absence of the CAP_SETPCAP capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_setpcap(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_linux_immutable function may be used to
  * explain the absence of the CAP_LINUX_IMMUTABLE capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_linux_immutable(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_net_bind_service function may be used to
  * explain the absence of the DAC_NET_BIND_SERVICE capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_net_bind_service(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_net_broadcast function may be used to
  * explain the absence of the CAP_NET_BROADCAST capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_broadcast(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_net_admin function may be used to
  * explain the absence of the CAP_NET_ADMIN capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_admin(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_net_admin function may be used to
  * explain the absence of the DAC_NET_ADNIB capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_net_admin(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_net_raw function may be used to
  * explain the absence of the DAC_NET_RAW capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_net_raw(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_ipc_lock function may be used to
  * explain the absence of the CAP_IPC_LOCK capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_ipc_lock(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_ipc_owner function may be used to
  * explain the absence of the CAP_IPC_OWNER capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_ipc_owner(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_module function may be used to
  * explain the absence of the CAP_SYS_MODULE capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_module(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_rawio function may be used to
  * explain the absence of the CAP_SYS_RAWIO capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_rawio(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_chroot function may be used to explain the
  * absence of the CAP_SYS_CHROOT capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_chroot(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_ptrace function may be used to explain the
  * absence of the CAP_SYS_PTRACE capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_ptrace(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_pacct function may be used to explain the
  * absence of the CAP_SYS_PACCT capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_pacct(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_admin function may be used to explain the
  * absence of the CAP_SYS_ADMIN capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_admin(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_boot function may be used to explain the
  * absence of the CAP_SYS_BOOT capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_boot(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_nice function may be used to explain the
  * absence of the CAP_SYS_NICE capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_nice(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_resource function may be used to explain the
  * absence of the CAP_SYS_RESOURCE capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_resource(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_time function may be used to explain the
  * absence of the CAP_SYS_TIME capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_time(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_tty_config function may be used to explain the
  * absence of the CAP_SYS_TTY_CONFIG capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_tty_config(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_mknod function may be used to explain the
  * absence of the CAP_SYS_MKNOD capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_mknod(explain_string_buffer_t *sb);

/**
  * The explain_buffer_dac_sys_lease function may be used to explain the
  * absence of the CAP_SYS_LEASE capability.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_dac_sys_lease(explain_string_buffer_t *sb);

/**
  * The explain_buffer_and_the_process_is_not_privileged function is
  * called by the above functions to insert the text "and the process is
  * not privileged" or its translation.  This isolates the string to a
  * single function.
  *
  * @param sb
  *    The string buffer to print into.
  */
void explain_buffer_and_the_process_is_not_privileged(
    explain_string_buffer_t *sb);

/**
  * The explain_buffer_does_not_have_capability function is called
  * by the above functions to insert text naming the absent specific
  * capability.
  *
  * @param sb
  *    The string buffer to print into.
  * @param cap_name
  *     The name of the required capability.
  */
void explain_buffer_does_not_have_capability(explain_string_buffer_t *sb,
    const char *cap_name);

#endif /* LIBEXPLAIN_BUFFER_DAC_H */
/* vim: set ts=8 sw=4 et : */
