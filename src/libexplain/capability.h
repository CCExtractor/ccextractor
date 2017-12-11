/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_CAPABILITY_H
#define LIBEXPLAIN_CAPABILITY_H

/**
  * The explain_capability function may be used to determine whether
  * or not the process has the given capability; or, on systems without
  * capabilities, euid == root.
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability(int cap);

/**
  * In a system with the [_POSIX_CHOWN_RESTRICTED] option defined, this
  * overrides the restriction of changing file ownership and group
  * ownership.
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_chown(void);

/**
  * The explain_capability_dac_override function may be used to determine
  * whether or not the process has the CAP_DAC_OVERRIDE capability
  *
  * Override all DAC access, including ACL execute access if
  * [_POSIX_ACL] is defined. Excluding DAC access covered by
  * CAP_LINUX_IMMUTABLE.
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_dac_override(void);

/**
  * The explain_capability_dac_read_search function may be used to determine
  * whether or not the process has the CAP_DAC_READ_SEARCH capability
  *
  * Overrides all DAC restrictions regarding read and search on files
  * and directories, including ACL restrictions if [_POSIX_ACL] is
  * defined. Excluding DAC access covered by CAP_LINUX_IMMUTABLE.
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_dac_read_search(void);

/**
  * The explain_capability_fowner function may be used to determine
  * whether or not the process has the CAP_FOWNER capability
  *
  * Overrides all restrictions about allowed operations on files, where
  * file owner ID must be equal to the user ID, except where CAP_FSETID
  * is applicable. It doesn't override MAC and DAC restrictions.
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_fowner(void);

/**
  * The explain_capability_fsetid function may be used to determine
  * whether or not the process has the CAP_FSETID capability
  *
  * Overrides the following restrictions that the effective user ID
  * shall match the file owner ID when setting the S_ISUID and S_ISGID
  * bits on that file; that the effective group ID (or one of the
  * supplementary group IDs) shall match the file owner ID when setting
  * the S_ISGID bit on that file; that the S_ISUID and S_ISGID bits are
  * cleared on successful return from chown(2) (not implemented).
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_fsetid(void);

/**
  * The explain_capability_kill function may be used to determine
  * whether or not the process has the CAP_KILL capability
  *
  * Overrides the restriction that the real or effective user ID of a
  * process sending a signal must match the real or effective user ID
  * of the process receiving the signal.
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_kill(void);

/**
  * The explain_capability_setgid function may be used to determine
  * whether or not the process has the CAP_SETGID capability
  *
  * Allows setgid(2) manipulation
  * Allows setgroups(2)
  * Allows forged gids on socket credentials passing.
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_setgid(void);

/**
  * The explain_capability_setuid function may be used to determine
  * whether or not the process has the CAP_SETUID capability
  *
  * Allows set*uid(2) manipulation (including fsuid).
  * Allows forged pids on socket credentials passing.
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_setuid(void);

/**
  * The explain_capability_setpcap function may be used to determine
  * whether or not the process has the CAP_SETPCAP capability
  *
  * Transfer any capability in your permitted set to any pid,
  * remove any capability in your permitted set from any pid
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_setpcap(void);

/**
  * The explain_capability_linux_immutable function may be used to determine
  * whether or not the process has the CAP_LINUX_IMMUTABLE capability
  *
  * Allow modification of S_IMMUTABLE and S_APPEND file attributes
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_linux_immutable(void);

/**
  * The explain_capability_net_bind_service function may be used to determine
  * whether or not the process has the CAP_NET_BIND_SERVICE capability
  *
  * Allows binding to TCP/UDP sockets below 1024
  * Allows binding to ATM VCIs below 32
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_net_bind_service(void);

/**
  * The explain_capability_net_broadcast function may be used to determine
  * whether or not the process has the CAP_NET_BROADCAST capability
  *
  * Allow broadcasting, listen to multicast
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_net_broadcast(void);

/**
  * The explain_capability_net_admin function may be used to determine
  * whether or not the process has the CAP_NET_ADMIN capability
  *
  * Allow interface configuration
  * Allow administration of IP firewall, masquerading and accounting
  * Allow setting debug option on sockets
  * Allow modification of routing tables
  * Allow setting arbitrary process / process group ownership on
  *   sockets
  * Allow binding to any address for transparent proxying
  * Allow setting TOS (type of service)
  * Allow setting promiscuous mode
  * Allow clearing driver statistics
  * Allow multicasting
  * Allow read/write of device-specific registers
  * Allow activation of ATM control sockets
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_net_admin(void);

/**
  * The explain_capability_net_raw function may be used to determine
  * whether or not the process has the CAP_NET_RAW capability
  *
  * Allow use of RAW sockets
  * Allow use of PACKET sockets
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_net_raw(void);

/**
  * The explain_capability_ipc_lock function may be used to determine
  * whether or not the process has the CAP_IPC_LOCK capability
  *
  * Allow locking of shared memory segments
  * Allow mlock and mlockall (which doesn't really have anything to do
  *   with IPC)
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_ipc_lock(void);

/**
  * The explain_capability_ipc_owner function may be used to determine
  * whether or not the process has the CAP_IPC_OWNER capability
  *
  * Override IPC ownership checks
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_ipc_owner(void);

/**
  * The explain_capability_sys_module function may be used to determine
  * whether or not the process has the CAP_SYS_MODULE capability
  *
  * Insert and remove kernel modules - modify kernel without limit
  * Modify cap_bset
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_module(void);

/**
  * The explain_capability_sys_rawio function may be used to determine
  * whether or not the process has the CAP_SYS_RAWIO capability
  *
  * Allow ioperm/iopl access
  * Allow sending USB messages to any device via /proc/bus/usb
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_rawio(void);

/**
  * The explain_capability_sys_chroot function may be used to determine
  * whether or not the process has the CAP_SYS_CHROOT capability
  *
  * Allow use of chroot()
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_chroot(void);

/**
  * The explain_capability_sys_ptrace function may be used to determine
  * whether or not the process has the CAP_SYS_PTRACE capability
  *
  * Allow ptrace() of any process
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_ptrace(void);

/**
  * The explain_capability_sys_pacct function may be used to determine
  * whether or not the process has the CAP_SYS_PACCT capability
  *
  * Allow configuration of process accounting
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_pacct(void);

/**
  * The explain_capability_sys_admin function may be used to determine
  * whether or not the process has the CAP_SYS_ADMIN capability
  *
  * Allow configuration of the secure attention key
  * Allow administration of the random device
  * Allow examination and configuration of disk quotas
  * Allow configuring the kernel's syslog (printk behaviour)
  * Allow setting the domainname
  * Allow setting the hostname
  * Allow calling bdflush()
  * Allow mount() and umount(), setting up new smb connection
  * Allow some autofs root ioctls
  * Allow nfsservctl
  * Allow VM86_REQUEST_IRQ
  * Allow to read/write pci config on alpha
  * Allow irix_prctl on mips (setstacksize)
  * Allow flushing all cache on m68k (sys_cacheflush)
  * Allow removing semaphores
  * Used instead of CAP_CHOWN to "chown" IPC message queues, semaphores
  *   and shared memory
  * Allow locking/unlocking of shared memory segment
  * Allow turning swap on/off
  * Allow forged pids on socket credentials passing
  * Allow setting readahead and flushing buffers on block devices
  * Allow setting geometry in floppy driver
  * Allow turning DMA on/off in xd driver
  * Allow administration of md devices (mostly the above, but some
  *   extra ioctls)
  * Allow tuning the ide driver
  * Allow access to the nvram device
  * Allow administration of apm_bios, serial and bttv (TV) device
  * Allow manufacturer commands in isdn CAPI support driver
  * Allow reading non-standardized portions of pci configuration space
  * Allow DDI debug ioctl on sbpcd driver
  * Allow setting up serial ports
  * Allow sending raw qic-117 commands
  * Allow enabling/disabling tagged queuing on SCSI controllers and sending
  *   arbitrary SCSI commands
  * Allow setting encryption key on loopback filesystem
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_admin(void);

/**
  * The explain_capability_sys_boot function may be used to determine
  * whether or not the process has the CAP_SYS_BOOT capability
  *
  * Allow use of reboot()
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_boot(void);

/**
  * The explain_capability_sys_nice function may be used to determine
  * whether or not the process has the CAP_SYS_NICE capability
  *
  * Allow raising priority and setting priority on other (different
  *   UID) processes
  * Allow use of FIFO and round-robin (realtime) scheduling on own
  *   processes and setting the scheduling algorithm used by another
  *   process.
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_nice(void);

/**
  * The explain_capability_sys_resource function may be used to determine
  * whether or not the process has the CAP_SYS_RESOURCE capability
  *
  * Override resource limits. Set resource limits.
  * Override quota limits.
  * Override reserved space on ext2 filesystem
  * NOTE: ext2 honors fsuid when checking for resource overrides, so
  *   you can override using fsuid too
  * Override size restrictions on IPC message queues
  * Allow more than 64hz interrupts from the real-time clock
  * Override max number of consoles on console allocation
  * Override max number of keymaps
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_resource(void);

/**
  * The explain_capability_sys_time function may be used to determine
  * whether or not the process has the CAP_SYS_TIME capability
  *
  * Allow manipulation of system clock
  * Allow irix_stime on mips
  * Allow setting the real-time clock
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_time(void);

/**
  * The explain_capability_sys_tty_config function may be used to determine
  * whether or not the process has the CAP_SYS_TTY_CONFIG capability
  *
  * Allow configuration of tty devices
  * Allow vhangup() of tty
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_sys_tty_config(void);

/**
  * The explain_capability_mknod function may be used to determine
  * whether or not the process has the CAP_SYS_MKNOD capability
  *
  * Allow the privileged aspects of mknod()
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_mknod(void);

/**
  * The explain_capability_lease function may be used to determine
  * whether or not the process has the CAP_LEASE capability
  *
  * Allow taking of leases on files
  *
  * @returns
  *    int; non-zero (true) if have the capability, zero (false) if not
  */
int explain_capability_lease(void);


#endif /* LIBEXPLAIN_CAPABILITY_H */
/* vim: set ts=8 sw=4 et : */
