/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/sys/ioctl.h>

/* Keep this list sorted alphabetically ("C" locale, not "en"). */
#include <libexplain/iocontrol/blkbszget.h>
#include <libexplain/iocontrol/blkbszset.h>
#include <libexplain/iocontrol/blkdiscard.h>
#include <libexplain/iocontrol/blkelvget.h>
#include <libexplain/iocontrol/blkelvset.h>
#include <libexplain/iocontrol/blkflsbuf.h>
#include <libexplain/iocontrol/blkfraget.h>
#include <libexplain/iocontrol/blkfraset.h>
#include <libexplain/iocontrol/blkgetsize.h>
#include <libexplain/iocontrol/blkgetsize64.h>
#include <libexplain/iocontrol/blkpg.h>
#include <libexplain/iocontrol/blkraget.h>
#include <libexplain/iocontrol/blkraset.h>
#include <libexplain/iocontrol/blkroget.h>
#include <libexplain/iocontrol/blkroset.h>
#include <libexplain/iocontrol/blkrrpart.h>
#include <libexplain/iocontrol/blksectget.h>
#include <libexplain/iocontrol/blksectset.h>
#include <libexplain/iocontrol/blksszget.h>
#include <libexplain/iocontrol/blktracesetup.h>
#include <libexplain/iocontrol/blktracestart.h>
#include <libexplain/iocontrol/blktracestop.h>
#include <libexplain/iocontrol/blktraceteardown.h>
#include <libexplain/iocontrol/bmap_ioctl.h>
#include <libexplain/iocontrol/cdrom_changer_nslots.h>
#include <libexplain/iocontrol/cdrom_clear_options.h>
#include <libexplain/iocontrol/cdrom_debug.h>
#include <libexplain/iocontrol/cdrom_disc_status.h>
#include <libexplain/iocontrol/cdrom_drive_status.h>
#include <libexplain/iocontrol/cdrom_get_capability.h>
#include <libexplain/iocontrol/cdrom_get_mcn.h>
#include <libexplain/iocontrol/cdrom_get_upc.h>
#include <libexplain/iocontrol/cdrom_last_written.h>
#include <libexplain/iocontrol/cdrom_lockdoor.h>
#include <libexplain/iocontrol/cdrom_media_changed.h>
#include <libexplain/iocontrol/cdrom_next_writable.h>
#include <libexplain/iocontrol/cdrom_select_disc.h>
#include <libexplain/iocontrol/cdrom_select_speed.h>
#include <libexplain/iocontrol/cdrom_send_packet.h>
#include <libexplain/iocontrol/cdrom_set_options.h>
#include <libexplain/iocontrol/cdromaudiobufsiz.h>
#include <libexplain/iocontrol/cdromclosetray.h>
#include <libexplain/iocontrol/cdromeject.h>
#include <libexplain/iocontrol/cdromeject_sw.h>
#include <libexplain/iocontrol/cdromgetspindown.h>
#include <libexplain/iocontrol/cdrommultisession.h>
#include <libexplain/iocontrol/cdrompause.h>
#include <libexplain/iocontrol/cdromplayblk.h>
#include <libexplain/iocontrol/cdromplaymsf.h>
#include <libexplain/iocontrol/cdromplaytrkind.h>
#include <libexplain/iocontrol/cdromreadall.h>
#include <libexplain/iocontrol/cdromreadaudio.h>
#include <libexplain/iocontrol/cdromreadcooked.h>
#include <libexplain/iocontrol/cdromreadmode1.h>
#include <libexplain/iocontrol/cdromreadmode2.h>
#include <libexplain/iocontrol/cdromreadraw.h>
#include <libexplain/iocontrol/cdromreadtocentry.h>
#include <libexplain/iocontrol/cdromreadtochdr.h>
#include <libexplain/iocontrol/cdromreset.h>
#include <libexplain/iocontrol/cdromresume.h>
#include <libexplain/iocontrol/cdromseek.h>
#include <libexplain/iocontrol/cdromsetspindown.h>
#include <libexplain/iocontrol/cdromstart.h>
#include <libexplain/iocontrol/cdromstop.h>
#include <libexplain/iocontrol/cdromsubchnl.h>
#include <libexplain/iocontrol/cdromvolctrl.h>
#include <libexplain/iocontrol/cdromvolread.h>
#include <libexplain/iocontrol/cm206ctl_get_last_stat.h>
#include <libexplain/iocontrol/cm206ctl_get_stat.h>
#include <libexplain/iocontrol/cygetcd1400ver.h>
#include <libexplain/iocontrol/cygetdefthresh.h>
#include <libexplain/iocontrol/cygetdeftimeout.h>
#include <libexplain/iocontrol/cygetmon.h>
#include <libexplain/iocontrol/cygetrflow.h>
#include <libexplain/iocontrol/cygetrtsdtr_inv.h>
#include <libexplain/iocontrol/cygetthresh.h>
#include <libexplain/iocontrol/cygettimeout.h>
#include <libexplain/iocontrol/cygetwait.h>
#include <libexplain/iocontrol/cysetdefthresh.h>
#include <libexplain/iocontrol/cysetdeftimeout.h>
#include <libexplain/iocontrol/cysetrflow.h>
#include <libexplain/iocontrol/cysetrtsdtr_inv.h>
#include <libexplain/iocontrol/cysetthresh.h>
#include <libexplain/iocontrol/cysettimeout.h>
#include <libexplain/iocontrol/cysetwait.h>
#include <libexplain/iocontrol/cyzgetpollcycle.h>
#include <libexplain/iocontrol/cyzsetpollcycle.h>
#include <libexplain/iocontrol/dvd_auth.h>
#include <libexplain/iocontrol/dvd_read_struct.h>
#include <libexplain/iocontrol/dvd_write_struct.h>
#include <libexplain/iocontrol/eql_emancipate.h>
#include <libexplain/iocontrol/eql_enslave.h>
#include <libexplain/iocontrol/eql_getmastrcfg.h>
#include <libexplain/iocontrol/eql_getslavecfg.h>
#include <libexplain/iocontrol/eql_setmastrcfg.h>
#include <libexplain/iocontrol/eql_setslavecfg.h>
#include <libexplain/iocontrol/ext2_ioc_getrsvsz.h>
#include <libexplain/iocontrol/ext2_ioc_setrsvsz.h>
#include <libexplain/iocontrol/fdclrprm.h>
#include <libexplain/iocontrol/fddefmediaprm.h>
#include <libexplain/iocontrol/fddefprm.h>
#include <libexplain/iocontrol/fdeject.h>
#include <libexplain/iocontrol/fdflush.h>
#include <libexplain/iocontrol/fdfmtbeg.h>
#include <libexplain/iocontrol/fdfmtend.h>
#include <libexplain/iocontrol/fdfmttrk.h>
#include <libexplain/iocontrol/fdgetdrvprm.h>
#include <libexplain/iocontrol/fdgetdrvstat.h>
#include <libexplain/iocontrol/fdgetdrvtyp.h>
#include <libexplain/iocontrol/fdgetfdcstat.h>
#include <libexplain/iocontrol/fdgetmaxerrs.h>
#include <libexplain/iocontrol/fdgetmediaprm.h>
#include <libexplain/iocontrol/fdgetprm.h>
#include <libexplain/iocontrol/fdmsgoff.h>
#include <libexplain/iocontrol/fdmsgon.h>
#include <libexplain/iocontrol/fdpolldrvstat.h>
#include <libexplain/iocontrol/fdrawcmd.h>
#include <libexplain/iocontrol/fdreset.h>
#include <libexplain/iocontrol/fdsetdrvprm.h>
#include <libexplain/iocontrol/fdsetemsgtresh.h>
#include <libexplain/iocontrol/fdsetmaxerrs.h>
#include <libexplain/iocontrol/fdsetmediaprm.h>
#include <libexplain/iocontrol/fdsetprm.h>
#include <libexplain/iocontrol/fdtwaddle.h>
#include <libexplain/iocontrol/fdwerrorclr.h>
#include <libexplain/iocontrol/fdwerrorget.h>
#include <libexplain/iocontrol/fibmap.h>
#include <libexplain/iocontrol/figetbsz.h>
#include <libexplain/iocontrol/fioasync.h>
#include <libexplain/iocontrol/fioclex.h>
#include <libexplain/iocontrol/fiogetown.h>
#include <libexplain/iocontrol/fionbio.h>
#include <libexplain/iocontrol/fionclex.h>
#include <libexplain/iocontrol/fionread.h>
#include <libexplain/iocontrol/fioqsize.h>
#include <libexplain/iocontrol/fiosetown.h>
#include <libexplain/iocontrol/fs_ioc32_getflags.h>
#include <libexplain/iocontrol/fs_ioc32_getversion.h>
#include <libexplain/iocontrol/fs_ioc32_setflags.h>
#include <libexplain/iocontrol/fs_ioc32_setversion.h>
#include <libexplain/iocontrol/fs_ioc_fiemap.h>
#include <libexplain/iocontrol/fs_ioc_getflags.h>
#include <libexplain/iocontrol/fs_ioc_getversion.h>
#include <libexplain/iocontrol/fs_ioc_setflags.h>
#include <libexplain/iocontrol/fs_ioc_setversion.h>
#include <libexplain/iocontrol/gio_cmap.h>
#include <libexplain/iocontrol/gio_font.h>
#include <libexplain/iocontrol/gio_fontx.h>
#include <libexplain/iocontrol/gio_scrnmap.h>
#include <libexplain/iocontrol/gio_unimap.h>
#include <libexplain/iocontrol/gio_uniscrnmap.h>
#include <libexplain/iocontrol/hdio_drive_cmd.h>
#include <libexplain/iocontrol/hdio_drive_reset.h>
#include <libexplain/iocontrol/hdio_drive_task.h>
#include <libexplain/iocontrol/hdio_drive_taskfile.h>
#include <libexplain/iocontrol/hdio_get_32bit.h>
#include <libexplain/iocontrol/hdio_get_acoustic.h>
#include <libexplain/iocontrol/hdio_get_address.h>
#include <libexplain/iocontrol/hdio_get_busstate.h>
#include <libexplain/iocontrol/hdio_get_dma.h>
#include <libexplain/iocontrol/hdio_get_identity.h>
#include <libexplain/iocontrol/hdio_get_keepsettings.h>
#include <libexplain/iocontrol/hdio_get_multcount.h>
#include <libexplain/iocontrol/hdio_get_nice.h>
#include <libexplain/iocontrol/hdio_get_nowerr.h>
#include <libexplain/iocontrol/hdio_get_qdma.h>
#include <libexplain/iocontrol/hdio_get_unmaskintr.h>
#include <libexplain/iocontrol/hdio_get_wcache.h>
#include <libexplain/iocontrol/hdio_getgeo.h>
#include <libexplain/iocontrol/hdio_obsolete_identity.h>
#include <libexplain/iocontrol/hdio_scan_hwif.h>
#include <libexplain/iocontrol/hdio_set_32bit.h>
#include <libexplain/iocontrol/hdio_set_acoustic.h>
#include <libexplain/iocontrol/hdio_set_address.h>
#include <libexplain/iocontrol/hdio_set_busstate.h>
#include <libexplain/iocontrol/hdio_set_dma.h>
#include <libexplain/iocontrol/hdio_set_keepsettings.h>
#include <libexplain/iocontrol/hdio_set_multcount.h>
#include <libexplain/iocontrol/hdio_set_nice.h>
#include <libexplain/iocontrol/hdio_set_nowerr.h>
#include <libexplain/iocontrol/hdio_set_pio_mode.h>
#include <libexplain/iocontrol/hdio_set_qdma.h>
#include <libexplain/iocontrol/hdio_set_unmaskintr.h>
#include <libexplain/iocontrol/hdio_set_wcache.h>
#include <libexplain/iocontrol/hdio_set_xfer.h>
#include <libexplain/iocontrol/hdio_tristate_hwif.h>
#include <libexplain/iocontrol/hdio_unregister_hwif.h>
#include <libexplain/iocontrol/kdaddio.h>
#include <libexplain/iocontrol/kddelio.h>
#include <libexplain/iocontrol/kddisabio.h>
#include <libexplain/iocontrol/kdenabio.h>
#include <libexplain/iocontrol/kdfontop.h>
#include <libexplain/iocontrol/kdgetkeycode.h>
#include <libexplain/iocontrol/kdgetled.h>
#include <libexplain/iocontrol/kdgetmode.h>
#include <libexplain/iocontrol/kdgkbdiacr.h>
#include <libexplain/iocontrol/kdgkbdiacruc.h>
#include <libexplain/iocontrol/kdgkbent.h>
#include <libexplain/iocontrol/kdgkbled.h>
#include <libexplain/iocontrol/kdgkbmeta.h>
#include <libexplain/iocontrol/kdgkbmode.h>
#include <libexplain/iocontrol/kdgkbsent.h>
#include <libexplain/iocontrol/kdgkbtype.h>
#include <libexplain/iocontrol/kdkbdrep.h>
#include <libexplain/iocontrol/kdmapdisp.h>
#include <libexplain/iocontrol/kdmktone.h>
#include <libexplain/iocontrol/kdsetkeycode.h>
#include <libexplain/iocontrol/kdsetled.h>
#include <libexplain/iocontrol/kdsetmode.h>
#include <libexplain/iocontrol/kdsigaccept.h>
#include <libexplain/iocontrol/kdskbdiacr.h>
#include <libexplain/iocontrol/kdskbdiacruc.h>
#include <libexplain/iocontrol/kdskbent.h>
#include <libexplain/iocontrol/kdskbled.h>
#include <libexplain/iocontrol/kdskbmeta.h>
#include <libexplain/iocontrol/kdskbmode.h>
#include <libexplain/iocontrol/kdskbsent.h>
#include <libexplain/iocontrol/kdunmapdisp.h>
#include <libexplain/iocontrol/kiocsound.h>
#include <libexplain/iocontrol/lpabort.h>
#include <libexplain/iocontrol/lpabortopen.h>
#include <libexplain/iocontrol/lpcareful.h>
#include <libexplain/iocontrol/lpchar.h>
#include <libexplain/iocontrol/lpgetflags.h>
#include <libexplain/iocontrol/lpgetirq.h>
#include <libexplain/iocontrol/lpgetstats.h>
#include <libexplain/iocontrol/lpgetstatus.h>
#include <libexplain/iocontrol/lpreset.h>
#include <libexplain/iocontrol/lpsetirq.h>
#include <libexplain/iocontrol/lpsettimeout.h>
#include <libexplain/iocontrol/lptime.h>
#include <libexplain/iocontrol/lpwait.h>
#include <libexplain/iocontrol/mtiocget.h>
#include <libexplain/iocontrol/mtiocgetconfig.h>
#include <libexplain/iocontrol/mtiocpos.h>
#include <libexplain/iocontrol/mtiocsetconfig.h>
#include <libexplain/iocontrol/mtioctop.h>
#include <libexplain/iocontrol/pio_cmap.h>
#include <libexplain/iocontrol/pio_font.h>
#include <libexplain/iocontrol/pio_fontreset.h>
#include <libexplain/iocontrol/pio_fontx.h>
#include <libexplain/iocontrol/pio_scrnmap.h>
#include <libexplain/iocontrol/pio_unimap.h>
#include <libexplain/iocontrol/pio_unimapclr.h>
#include <libexplain/iocontrol/pio_uniscrnmap.h>
#include <libexplain/iocontrol/pppiocattach.h>
#include <libexplain/iocontrol/pppiocattchan.h>
#include <libexplain/iocontrol/pppiocconnect.h>
#include <libexplain/iocontrol/pppiocdetach.h>
#include <libexplain/iocontrol/pppiocdisconn.h>
#include <libexplain/iocontrol/pppiocgasyncmap.h>
#include <libexplain/iocontrol/pppiocgchan.h>
#include <libexplain/iocontrol/pppiocgdebug.h>
#include <libexplain/iocontrol/pppiocgflags.h>
#include <libexplain/iocontrol/pppiocgidle.h>
#include <libexplain/iocontrol/pppiocgl2tpstats.h>
#include <libexplain/iocontrol/pppiocgmru.h>
#include <libexplain/iocontrol/pppiocgnpmode.h>
#include <libexplain/iocontrol/pppiocgrasyncmap.h>
#include <libexplain/iocontrol/pppiocgunit.h>
#include <libexplain/iocontrol/pppiocgxasyncmap.h>
#include <libexplain/iocontrol/pppiocnewunit.h>
#include <libexplain/iocontrol/pppiocsactive.h>
#include <libexplain/iocontrol/pppiocsasyncmap.h>
#include <libexplain/iocontrol/pppiocscompress.h>
#include <libexplain/iocontrol/pppiocsdebug.h>
#include <libexplain/iocontrol/pppiocsflags.h>
#include <libexplain/iocontrol/pppiocsmaxcid.h>
#include <libexplain/iocontrol/pppiocsmrru.h>
#include <libexplain/iocontrol/pppiocsmru.h>
#include <libexplain/iocontrol/pppiocsnpmode.h>
#include <libexplain/iocontrol/pppiocspass.h>
#include <libexplain/iocontrol/pppiocsrasyncmap.h>
#include <libexplain/iocontrol/pppiocsxasyncmap.h>
#include <libexplain/iocontrol/pppiocxferunit.h>
#include <libexplain/iocontrol/siocadddlci.h>
#include <libexplain/iocontrol/siocaddmulti.h>
#include <libexplain/iocontrol/siocaddrt.h>
#include <libexplain/iocontrol/siocatmark.h>
#include <libexplain/iocontrol/siocbondchangeactive.h>
#include <libexplain/iocontrol/siocbondenslave.h>
#include <libexplain/iocontrol/siocbondinfoquery.h>
#include <libexplain/iocontrol/siocbondrelease.h>
#include <libexplain/iocontrol/siocbondsethwaddr.h>
#include <libexplain/iocontrol/siocbondslaveinfoquery.h>
#include <libexplain/iocontrol/siocbraddbr.h>
#include <libexplain/iocontrol/siocbraddif.h>
#include <libexplain/iocontrol/siocbrdelbr.h>
#include <libexplain/iocontrol/siocbrdelif.h>
#include <libexplain/iocontrol/siocdarp.h>
#include <libexplain/iocontrol/siocdeldlci.h>
#include <libexplain/iocontrol/siocdelmulti.h>
#include <libexplain/iocontrol/siocdelrt.h>
#include <libexplain/iocontrol/siocdifaddr.h>
#include <libexplain/iocontrol/siocdrarp.h>
#include <libexplain/iocontrol/siocethtool.h>
#include <libexplain/iocontrol/siocgarp.h>
#include <libexplain/iocontrol/siocgifaddr.h>
#include <libexplain/iocontrol/siocgifbr.h>
#include <libexplain/iocontrol/siocgifbrdaddr.h>
#include <libexplain/iocontrol/siocgifconf.h>
#include <libexplain/iocontrol/siocgifcount.h>
#include <libexplain/iocontrol/siocgifdivert.h>
#include <libexplain/iocontrol/siocgifdstaddr.h>
#include <libexplain/iocontrol/siocgifencap.h>
#include <libexplain/iocontrol/siocgifflags.h>
#include <libexplain/iocontrol/siocgifhwaddr.h>
#include <libexplain/iocontrol/siocgifindex.h>
#include <libexplain/iocontrol/siocgifmap.h>
#include <libexplain/iocontrol/siocgifmem.h>
#include <libexplain/iocontrol/siocgifmetric.h>
#include <libexplain/iocontrol/siocgifmtu.h>
#include <libexplain/iocontrol/siocgifname.h>
#include <libexplain/iocontrol/siocgifnetmask.h>
#include <libexplain/iocontrol/siocgifpflags.h>
#include <libexplain/iocontrol/siocgifslave.h>
#include <libexplain/iocontrol/siocgiftxqlen.h>
#include <libexplain/iocontrol/siocgifvlan.h>
#include <libexplain/iocontrol/siocgmiiphy.h>
#include <libexplain/iocontrol/siocgmiireg.h>
#include <libexplain/iocontrol/siocgpgrp.h>
#include <libexplain/iocontrol/siocgpppcstats.h>
#include <libexplain/iocontrol/siocgpppstats.h>
#include <libexplain/iocontrol/siocgpppver.h>
#include <libexplain/iocontrol/siocgrarp.h>
#include <libexplain/iocontrol/siocgstamp.h>
#include <libexplain/iocontrol/siocgstampns.h>
#include <libexplain/iocontrol/siocinq.h>
#include <libexplain/iocontrol/siocoutq.h>
#include <libexplain/iocontrol/siocrtmsg.h>
#include <libexplain/iocontrol/siocsarp.h>
#include <libexplain/iocontrol/siocscccal.h>
#include <libexplain/iocontrol/siocscccfg.h>
#include <libexplain/iocontrol/siocsccchanini.h>
#include <libexplain/iocontrol/siocsccgkiss.h>
#include <libexplain/iocontrol/siocsccgstat.h>
#include <libexplain/iocontrol/siocsccini.h>
#include <libexplain/iocontrol/siocsccskiss.h>
#include <libexplain/iocontrol/siocsccsmem.h>
#include <libexplain/iocontrol/siocshwtstamp.h>
#include <libexplain/iocontrol/siocsifaddr.h>
#include <libexplain/iocontrol/siocsifbr.h>
#include <libexplain/iocontrol/siocsifbrdaddr.h>
#include <libexplain/iocontrol/siocsifdivert.h>
#include <libexplain/iocontrol/siocsifdstaddr.h>
#include <libexplain/iocontrol/siocsifencap.h>
#include <libexplain/iocontrol/siocsifflags.h>
#include <libexplain/iocontrol/siocsifhwaddr.h>
#include <libexplain/iocontrol/siocsifhwbroadcast.h>
#include <libexplain/iocontrol/siocsiflink.h>
#include <libexplain/iocontrol/siocsifmap.h>
#include <libexplain/iocontrol/siocsifmem.h>
#include <libexplain/iocontrol/siocsifmetric.h>
#include <libexplain/iocontrol/siocsifmtu.h>
#include <libexplain/iocontrol/siocsifname.h>
#include <libexplain/iocontrol/siocsifnetmask.h>
#include <libexplain/iocontrol/siocsifpflags.h>
#include <libexplain/iocontrol/siocsifslave.h>
#include <libexplain/iocontrol/siocsiftxqlen.h>
#include <libexplain/iocontrol/siocsifvlan.h>
#include <libexplain/iocontrol/siocsmiireg.h>
#include <libexplain/iocontrol/siocspgrp.h>
#include <libexplain/iocontrol/siocsrarp.h>
#include <libexplain/iocontrol/siocwandev.h>
#include <libexplain/iocontrol/siogifindex.h>
#include <libexplain/iocontrol/table.h>
#include <libexplain/iocontrol/tcflsh.h>
#include <libexplain/iocontrol/tcgeta.h>
#include <libexplain/iocontrol/tcgets.h>
#include <libexplain/iocontrol/tcgets2.h>
#include <libexplain/iocontrol/tcgetx.h>
#include <libexplain/iocontrol/tcsbrk.h>
#include <libexplain/iocontrol/tcsbrkp.h>
#include <libexplain/iocontrol/tcseta.h>
#include <libexplain/iocontrol/tcsetaf.h>
#include <libexplain/iocontrol/tcsetaw.h>
#include <libexplain/iocontrol/tcsets.h>
#include <libexplain/iocontrol/tcsets2.h>
#include <libexplain/iocontrol/tcsetsf.h>
#include <libexplain/iocontrol/tcsetsf2.h>
#include <libexplain/iocontrol/tcsetsw.h>
#include <libexplain/iocontrol/tcsetsw2.h>
#include <libexplain/iocontrol/tcsetx.h>
#include <libexplain/iocontrol/tcsetxf.h>
#include <libexplain/iocontrol/tcsetxw.h>
#include <libexplain/iocontrol/tcxonc.h>
#include <libexplain/iocontrol/tioccbrk.h>
#include <libexplain/iocontrol/tioccons.h>
#include <libexplain/iocontrol/tiocdrain.h>
#include <libexplain/iocontrol/tiocexcl.h>
#include <libexplain/iocontrol/tiocgdev.h>
#include <libexplain/iocontrol/tiocgetc.h>
#include <libexplain/iocontrol/tiocgetd.h>
#include <libexplain/iocontrol/tiocgetp.h>
#include <libexplain/iocontrol/tiocgetx.h>
#include <libexplain/iocontrol/tiocghayesesp.h>
#include <libexplain/iocontrol/tiocgicount.h>
#include <libexplain/iocontrol/tiocglcktrmios.h>
#include <libexplain/iocontrol/tiocgltc.h>
#include <libexplain/iocontrol/tiocgpgrp.h>
#include <libexplain/iocontrol/tiocgptn.h>
#include <libexplain/iocontrol/tiocgrs485.h>
#include <libexplain/iocontrol/tiocgserial.h>
#include <libexplain/iocontrol/tiocgsid.h>
#include <libexplain/iocontrol/tiocgsoftcar.h>
#include <libexplain/iocontrol/tiocgwinsz.h>
#include <libexplain/iocontrol/tiocinq.h>
#include <libexplain/iocontrol/tioclget.h>
#include <libexplain/iocontrol/tioclinux.h>
#include <libexplain/iocontrol/tiocmbic.h>
#include <libexplain/iocontrol/tiocmbis.h>
#include <libexplain/iocontrol/tiocmget.h>
#include <libexplain/iocontrol/tiocmiwait.h>
#include <libexplain/iocontrol/tiocmset.h>
#include <libexplain/iocontrol/tiocnotty.h>
#include <libexplain/iocontrol/tiocnxcl.h>
#include <libexplain/iocontrol/tiocoutq.h>
#include <libexplain/iocontrol/tiocpkt.h>
#include <libexplain/iocontrol/tiocsbrk.h>
#include <libexplain/iocontrol/tiocsctty.h>
#include <libexplain/iocontrol/tiocserconfig.h>
#include <libexplain/iocontrol/tiocsergetlsr.h>
#include <libexplain/iocontrol/tiocsergetmulti.h>
#include <libexplain/iocontrol/tiocsergstruct.h>
#include <libexplain/iocontrol/tiocsergwild.h>
#include <libexplain/iocontrol/tiocsersetmulti.h>
#include <libexplain/iocontrol/tiocserswild.h>
#include <libexplain/iocontrol/tiocsetd.h>
#include <libexplain/iocontrol/tiocshayesesp.h>
#include <libexplain/iocontrol/tiocsig.h>
#include <libexplain/iocontrol/tiocslcktrmios.h>
#include <libexplain/iocontrol/tiocspgrp.h>
#include <libexplain/iocontrol/tiocsptlck.h>
#include <libexplain/iocontrol/tiocsrs485.h>
#include <libexplain/iocontrol/tiocsserial.h>
#include <libexplain/iocontrol/tiocssoftcar.h>
#include <libexplain/iocontrol/tiocstart.h>
#include <libexplain/iocontrol/tiocsti.h>
#include <libexplain/iocontrol/tiocstop.h>
#include <libexplain/iocontrol/tiocswinsz.h>
#include <libexplain/iocontrol/tiocttygstruct.h>
#include <libexplain/iocontrol/vidioc_cropcap.h>
#include <libexplain/iocontrol/vidioc_dbg_g_chip_ident.h>
#include <libexplain/iocontrol/vidioc_dbg_g_register.h>
#include <libexplain/iocontrol/vidioc_dbg_s_register.h>
#include <libexplain/iocontrol/vidioc_dqbuf.h>
#include <libexplain/iocontrol/vidioc_dqevent.h>
#include <libexplain/iocontrol/vidioc_encoder_cmd.h>
#include <libexplain/iocontrol/vidioc_enum_dv_presets.h>
#include <libexplain/iocontrol/vidioc_enum_fmt.h>
#include <libexplain/iocontrol/vidioc_enum_frameintervals.h>
#include <libexplain/iocontrol/vidioc_enum_framesizes.h>
#include <libexplain/iocontrol/vidioc_enumaudio.h>
#include <libexplain/iocontrol/vidioc_enumaudout.h>
#include <libexplain/iocontrol/vidioc_enuminput.h>
#include <libexplain/iocontrol/vidioc_enumoutput.h>
#include <libexplain/iocontrol/vidioc_enumstd.h>
#include <libexplain/iocontrol/vidioc_g_audio.h>
#include <libexplain/iocontrol/vidioc_g_audout.h>
#include <libexplain/iocontrol/vidioc_g_crop.h>
#include <libexplain/iocontrol/vidioc_g_ctrl.h>
#include <libexplain/iocontrol/vidioc_g_dv_preset.h>
#include <libexplain/iocontrol/vidioc_g_dv_timings.h>
#include <libexplain/iocontrol/vidioc_g_enc_index.h>
#include <libexplain/iocontrol/vidioc_g_ext_ctrls.h>
#include <libexplain/iocontrol/vidioc_g_fbuf.h>
#include <libexplain/iocontrol/vidioc_g_fmt.h>
#include <libexplain/iocontrol/vidioc_g_frequency.h>
#include <libexplain/iocontrol/vidioc_g_input.h>
#include <libexplain/iocontrol/vidioc_g_jpegcomp.h>
#include <libexplain/iocontrol/vidioc_g_modulator.h>
#include <libexplain/iocontrol/vidioc_g_output.h>
#include <libexplain/iocontrol/vidioc_g_parm.h>
#include <libexplain/iocontrol/vidioc_g_priority.h>
#include <libexplain/iocontrol/vidioc_g_sliced_vbi_cap.h>
#include <libexplain/iocontrol/vidioc_g_std.h>
#include <libexplain/iocontrol/vidioc_g_tuner.h>
#include <libexplain/iocontrol/vidioc_log_status.h>
#include <libexplain/iocontrol/vidioc_overlay.h>
#include <libexplain/iocontrol/vidioc_qbuf.h>
#include <libexplain/iocontrol/vidioc_query_dv_preset.h>
#include <libexplain/iocontrol/vidioc_querybuf.h>
#include <libexplain/iocontrol/vidioc_querycap.h>
#include <libexplain/iocontrol/vidioc_queryctrl.h>
#include <libexplain/iocontrol/vidioc_querymenu.h>
#include <libexplain/iocontrol/vidioc_querystd.h>
#include <libexplain/iocontrol/vidioc_reqbufs.h>
#include <libexplain/iocontrol/vidioc_s_audio.h>
#include <libexplain/iocontrol/vidioc_s_audout.h>
#include <libexplain/iocontrol/vidioc_s_crop.h>
#include <libexplain/iocontrol/vidioc_s_ctrl.h>
#include <libexplain/iocontrol/vidioc_s_dv_preset.h>
#include <libexplain/iocontrol/vidioc_s_dv_timings.h>
#include <libexplain/iocontrol/vidioc_s_ext_ctrls.h>
#include <libexplain/iocontrol/vidioc_s_fbuf.h>
#include <libexplain/iocontrol/vidioc_s_fmt.h>
#include <libexplain/iocontrol/vidioc_s_frequency.h>
#include <libexplain/iocontrol/vidioc_s_hw_freq_seek.h>
#include <libexplain/iocontrol/vidioc_s_input.h>
#include <libexplain/iocontrol/vidioc_s_jpegcomp.h>
#include <libexplain/iocontrol/vidioc_s_modulator.h>
#include <libexplain/iocontrol/vidioc_s_output.h>
#include <libexplain/iocontrol/vidioc_s_parm.h>
#include <libexplain/iocontrol/vidioc_s_priority.h>
#include <libexplain/iocontrol/vidioc_s_std.h>
#include <libexplain/iocontrol/vidioc_s_tuner.h>
#include <libexplain/iocontrol/vidioc_streamoff.h>
#include <libexplain/iocontrol/vidioc_streamon.h>
#include <libexplain/iocontrol/vidioc_subscribe_event.h>
#include <libexplain/iocontrol/vidioc_try_fmt.h>
#include <libexplain/iocontrol/vidioccapture.h>
#include <libexplain/iocontrol/vidiocgaudio.h>
#include <libexplain/iocontrol/vidiocgcap.h>
#include <libexplain/iocontrol/vidiocgchan.h>
#include <libexplain/iocontrol/vidiocgfbuf.h>
#include <libexplain/iocontrol/vidiocgfreq.h>
#include <libexplain/iocontrol/vidiocgmbuf.h>
#include <libexplain/iocontrol/vidiocgpict.h>
#include <libexplain/iocontrol/vidiocgtuner.h>
#include <libexplain/iocontrol/vidiocgvbifmt.h>
#include <libexplain/iocontrol/vidiocgwin.h>
#include <libexplain/iocontrol/vidiocmcapture.h>
#include <libexplain/iocontrol/vidiocsaudio.h>
#include <libexplain/iocontrol/vidiocschan.h>
#include <libexplain/iocontrol/vidiocsfbuf.h>
#include <libexplain/iocontrol/vidiocsfreq.h>
#include <libexplain/iocontrol/vidiocspict.h>
#include <libexplain/iocontrol/vidiocstuner.h>
#include <libexplain/iocontrol/vidiocsvbifmt.h>
#include <libexplain/iocontrol/vidiocswin.h>
#include <libexplain/iocontrol/vidiocsync.h>
#include <libexplain/iocontrol/vt_activate.h>
#include <libexplain/iocontrol/vt_disallocate.h>
#include <libexplain/iocontrol/vt_gethifontmask.h>
#include <libexplain/iocontrol/vt_getmode.h>
#include <libexplain/iocontrol/vt_getstate.h>
#include <libexplain/iocontrol/vt_lockswitch.h>
#include <libexplain/iocontrol/vt_openqry.h>
#include <libexplain/iocontrol/vt_reldisp.h>
#include <libexplain/iocontrol/vt_resize.h>
#include <libexplain/iocontrol/vt_resizex.h>
#include <libexplain/iocontrol/vt_sendsig.h>
#include <libexplain/iocontrol/vt_setmode.h>
#include <libexplain/iocontrol/vt_unlockswitch.h>
#include <libexplain/iocontrol/vt_waitactive.h>
#include <libexplain/sizeof.h>

/*
 * The ugly thing about ioctl(2) is that, in effect, each ioctl request is a
 * separate and unique system call.
 *
 * This information is not kept in a single table for all values, like every
 * other set of constants, because (a) some values are ambiguous, and (b) the
 * include files have bugs making it impossible to include all of them in the
 * same combilation unit.
 *
 * By just storing pointers to our own data structure, there is no need to
 * include them all at once.
 *
 * Keep this array sorted alphabetically (C locale, not en).
 */

const explain_iocontrol_t *const explain_iocontrol_table[] =
{
    &explain_iocontrol_blkbszget,
    &explain_iocontrol_blkbszset,
    &explain_iocontrol_blkdiscard,
    &explain_iocontrol_blkelvget,
    &explain_iocontrol_blkelvset,
    &explain_iocontrol_blkflsbuf,
    &explain_iocontrol_blkfraget,
    &explain_iocontrol_blkfraset,
    &explain_iocontrol_blkgetsize,
    &explain_iocontrol_blkgetsize64,
    &explain_iocontrol_blkpg,
    &explain_iocontrol_blkraget,
    &explain_iocontrol_blkraset,
    &explain_iocontrol_blkroget,
    &explain_iocontrol_blkroset,
    &explain_iocontrol_blkrrpart,
    &explain_iocontrol_blksectget,
    &explain_iocontrol_blksectset,
    &explain_iocontrol_blksszget,
    &explain_iocontrol_blktracesetup,
    &explain_iocontrol_blktracestart,
    &explain_iocontrol_blktracestop,
    &explain_iocontrol_blktraceteardown,
    &explain_iocontrol_bmap_ioctl,
    &explain_iocontrol_cdrom_changer_nslots,
    &explain_iocontrol_cdrom_clear_options,
    &explain_iocontrol_cdrom_debug,
    &explain_iocontrol_cdrom_disc_status,
    &explain_iocontrol_cdrom_drive_status,
    &explain_iocontrol_cdrom_get_capability,
    &explain_iocontrol_cdrom_get_mcn,
    &explain_iocontrol_cdrom_get_upc,
    &explain_iocontrol_cdrom_last_written,
    &explain_iocontrol_cdrom_lockdoor,
    &explain_iocontrol_cdrom_media_changed,
    &explain_iocontrol_cdrom_next_writable,
    &explain_iocontrol_cdrom_select_disc,
    &explain_iocontrol_cdrom_select_speed,
    &explain_iocontrol_cdrom_send_packet,
    &explain_iocontrol_cdrom_set_options,
    &explain_iocontrol_cdromaudiobufsiz,
    &explain_iocontrol_cdromclosetray,
    &explain_iocontrol_cdromeject,
    &explain_iocontrol_cdromeject_sw,
    &explain_iocontrol_cdromgetspindown,
    &explain_iocontrol_cdrommultisession,
    &explain_iocontrol_cdrompause,
    &explain_iocontrol_cdromplayblk,
    &explain_iocontrol_cdromplaymsf,
    &explain_iocontrol_cdromplaytrkind,
    &explain_iocontrol_cdromreadall,
    &explain_iocontrol_cdromreadaudio,
    &explain_iocontrol_cdromreadcooked,
    &explain_iocontrol_cdromreadmode1,
    &explain_iocontrol_cdromreadmode2,
    &explain_iocontrol_cdromreadraw,
    &explain_iocontrol_cdromreadtocentry,
    &explain_iocontrol_cdromreadtochdr,
    &explain_iocontrol_cdromreset,
    &explain_iocontrol_cdromresume,
    &explain_iocontrol_cdromseek,
    &explain_iocontrol_cdromsetspindown,
    &explain_iocontrol_cdromstart,
    &explain_iocontrol_cdromstop,
    &explain_iocontrol_cdromsubchnl,
    &explain_iocontrol_cdromvolctrl,
    &explain_iocontrol_cdromvolread,
    &explain_iocontrol_cm206ctl_get_last_stat,
    &explain_iocontrol_cm206ctl_get_stat,
    &explain_iocontrol_cygetcd1400ver,
    &explain_iocontrol_cygetdefthresh,
    &explain_iocontrol_cygetdeftimeout,
    &explain_iocontrol_cygetmon,
    &explain_iocontrol_cygetrflow,
    &explain_iocontrol_cygetrtsdtr_inv,
    &explain_iocontrol_cygetthresh,
    &explain_iocontrol_cygettimeout,
    &explain_iocontrol_cygetwait,
    &explain_iocontrol_cysetdefthresh,
    &explain_iocontrol_cysetdeftimeout,
    &explain_iocontrol_cysetrflow,
    &explain_iocontrol_cysetrtsdtr_inv,
    &explain_iocontrol_cysetthresh,
    &explain_iocontrol_cysettimeout,
    &explain_iocontrol_cysetwait,
    &explain_iocontrol_cyzgetpollcycle,
    &explain_iocontrol_cyzsetpollcycle,
    &explain_iocontrol_dvd_auth,
    &explain_iocontrol_dvd_read_struct,
    &explain_iocontrol_dvd_write_struct,
    &explain_iocontrol_eql_emancipate,
    &explain_iocontrol_eql_enslave,
    &explain_iocontrol_eql_getmastrcfg,
    &explain_iocontrol_eql_getslavecfg,
    &explain_iocontrol_eql_setmastrcfg,
    &explain_iocontrol_eql_setslavecfg,
    &explain_iocontrol_ext2_ioc_getrsvsz,
    &explain_iocontrol_ext2_ioc_setrsvsz,
    &explain_iocontrol_fdclrprm,
    &explain_iocontrol_fddefmediaprm,
    &explain_iocontrol_fddefprm,
    &explain_iocontrol_fdeject,
    &explain_iocontrol_fdflush,
    &explain_iocontrol_fdfmtbeg,
    &explain_iocontrol_fdfmtend,
    &explain_iocontrol_fdfmttrk,
    &explain_iocontrol_fdgetdrvprm,
    &explain_iocontrol_fdgetdrvstat,
    &explain_iocontrol_fdgetdrvtyp,
    &explain_iocontrol_fdgetfdcstat,
    &explain_iocontrol_fdgetmaxerrs,
    &explain_iocontrol_fdgetmediaprm,
    &explain_iocontrol_fdgetprm,
    &explain_iocontrol_fdmsgoff,
    &explain_iocontrol_fdmsgon,
    &explain_iocontrol_fdpolldrvstat,
    &explain_iocontrol_fdrawcmd,
    &explain_iocontrol_fdreset,
    &explain_iocontrol_fdsetdrvprm,
    &explain_iocontrol_fdsetemsgtresh,
    &explain_iocontrol_fdsetmaxerrs,
    &explain_iocontrol_fdsetmediaprm,
    &explain_iocontrol_fdsetprm,
    &explain_iocontrol_fdtwaddle,
    &explain_iocontrol_fdwerrorclr,
    &explain_iocontrol_fdwerrorget,
    &explain_iocontrol_fibmap,
    &explain_iocontrol_figetbsz,
    &explain_iocontrol_fioasync,
    &explain_iocontrol_fioclex,
    &explain_iocontrol_fiogetown,
    &explain_iocontrol_fionbio,
    &explain_iocontrol_fionclex,
    &explain_iocontrol_fionread,
    &explain_iocontrol_fioqsize,
    &explain_iocontrol_fiosetown,
    &explain_iocontrol_fs_ioc32_getflags,
    &explain_iocontrol_fs_ioc32_getversion,
    &explain_iocontrol_fs_ioc32_setflags,
    &explain_iocontrol_fs_ioc32_setversion,
    &explain_iocontrol_fs_ioc_fiemap,
    &explain_iocontrol_fs_ioc_getflags,
    &explain_iocontrol_fs_ioc_getversion,
    &explain_iocontrol_fs_ioc_setflags,
    &explain_iocontrol_fs_ioc_setversion,
    &explain_iocontrol_gio_cmap,
    &explain_iocontrol_gio_font,
    &explain_iocontrol_gio_fontx,
    &explain_iocontrol_gio_scrnmap,
    &explain_iocontrol_gio_unimap,
    &explain_iocontrol_gio_uniscrnmap,
    &explain_iocontrol_hdio_drive_cmd,
    &explain_iocontrol_hdio_drive_reset,
    &explain_iocontrol_hdio_drive_task,
    &explain_iocontrol_hdio_drive_taskfile,
    &explain_iocontrol_hdio_get_32bit,
    &explain_iocontrol_hdio_get_acoustic,
    &explain_iocontrol_hdio_get_address,
    &explain_iocontrol_hdio_get_busstate,
    &explain_iocontrol_hdio_get_dma,
    &explain_iocontrol_hdio_get_identity,
    &explain_iocontrol_hdio_get_keepsettings,
    &explain_iocontrol_hdio_get_multcount,
    &explain_iocontrol_hdio_get_nice,
    &explain_iocontrol_hdio_get_nowerr,
    &explain_iocontrol_hdio_get_qdma,
    &explain_iocontrol_hdio_get_unmaskintr,
    &explain_iocontrol_hdio_get_wcache,
    &explain_iocontrol_hdio_getgeo,
    &explain_iocontrol_hdio_obsolete_identity,
    &explain_iocontrol_hdio_scan_hwif,
    &explain_iocontrol_hdio_set_32bit,
    &explain_iocontrol_hdio_set_acoustic,
    &explain_iocontrol_hdio_set_address,
    &explain_iocontrol_hdio_set_busstate,
    &explain_iocontrol_hdio_set_dma,
    &explain_iocontrol_hdio_set_keepsettings,
    &explain_iocontrol_hdio_set_multcount,
    &explain_iocontrol_hdio_set_nice,
    &explain_iocontrol_hdio_set_nowerr,
    &explain_iocontrol_hdio_set_pio_mode,
    &explain_iocontrol_hdio_set_qdma,
    &explain_iocontrol_hdio_set_unmaskintr,
    &explain_iocontrol_hdio_set_wcache,
    &explain_iocontrol_hdio_set_xfer,
    &explain_iocontrol_hdio_tristate_hwif,
    &explain_iocontrol_hdio_unregister_hwif,
    &explain_iocontrol_kdaddio,
    &explain_iocontrol_kddelio,
    &explain_iocontrol_kddisabio,
    &explain_iocontrol_kdenabio,
    &explain_iocontrol_kdfontop,
    &explain_iocontrol_kdgetkeycode,
    &explain_iocontrol_kdgetled,
    &explain_iocontrol_kdgetmode,
    &explain_iocontrol_kdgkbdiacr,
    &explain_iocontrol_kdgkbdiacruc,
    &explain_iocontrol_kdgkbent,
    &explain_iocontrol_kdgkbled,
    &explain_iocontrol_kdgkbmeta,
    &explain_iocontrol_kdgkbmode,
    &explain_iocontrol_kdgkbsent,
    &explain_iocontrol_kdgkbtype,
    &explain_iocontrol_kdkbdrep,
    &explain_iocontrol_kdmapdisp,
    &explain_iocontrol_kdmktone,
    &explain_iocontrol_kdsetkeycode,
    &explain_iocontrol_kdsetled,
    &explain_iocontrol_kdsetmode,
    &explain_iocontrol_kdsigaccept,
    &explain_iocontrol_kdskbdiacr,
    &explain_iocontrol_kdskbdiacruc,
    &explain_iocontrol_kdskbent,
    &explain_iocontrol_kdskbled,
    &explain_iocontrol_kdskbmeta,
    &explain_iocontrol_kdskbmode,
    &explain_iocontrol_kdskbsent,
    &explain_iocontrol_kdunmapdisp,
    &explain_iocontrol_kiocsound,
    &explain_iocontrol_lpabort,
    &explain_iocontrol_lpabortopen,
    &explain_iocontrol_lpcareful,
    &explain_iocontrol_lpchar,
    &explain_iocontrol_lpgetflags,
    &explain_iocontrol_lpgetirq,
    &explain_iocontrol_lpgetstats,
    &explain_iocontrol_lpgetstatus,
    &explain_iocontrol_lpreset,
    &explain_iocontrol_lpsetirq,
    &explain_iocontrol_lpsettimeout,
    &explain_iocontrol_lptime,
    &explain_iocontrol_lpwait,
    &explain_iocontrol_mtiocget,
    &explain_iocontrol_mtiocgetconfig,
    &explain_iocontrol_mtiocpos,
    &explain_iocontrol_mtiocsetconfig,
    &explain_iocontrol_mtioctop,
    &explain_iocontrol_pio_cmap,
    &explain_iocontrol_pio_font,
    &explain_iocontrol_pio_fontreset,
    &explain_iocontrol_pio_fontx,
    &explain_iocontrol_pio_scrnmap,
    &explain_iocontrol_pio_unimap,
    &explain_iocontrol_pio_unimapclr,
    &explain_iocontrol_pio_uniscrnmap,
    &explain_iocontrol_pppiocattach,
    &explain_iocontrol_pppiocattchan,
    &explain_iocontrol_pppiocconnect,
    &explain_iocontrol_pppiocdetach,
    &explain_iocontrol_pppiocdisconn,
    &explain_iocontrol_pppiocgasyncmap,
    &explain_iocontrol_pppiocgchan,
    &explain_iocontrol_pppiocgdebug,
    &explain_iocontrol_pppiocgflags,
    &explain_iocontrol_pppiocgidle,
    &explain_iocontrol_pppiocgl2tpstats,
    &explain_iocontrol_pppiocgmru,
    &explain_iocontrol_pppiocgnpmode,
    &explain_iocontrol_pppiocgrasyncmap,
    &explain_iocontrol_pppiocgunit,
    &explain_iocontrol_pppiocgxasyncmap,
    &explain_iocontrol_pppiocnewunit,
    &explain_iocontrol_pppiocsactive,
    &explain_iocontrol_pppiocsasyncmap,
    &explain_iocontrol_pppiocscompress,
    &explain_iocontrol_pppiocsdebug,
    &explain_iocontrol_pppiocsflags,
    &explain_iocontrol_pppiocsmaxcid,
    &explain_iocontrol_pppiocsmrru,
    &explain_iocontrol_pppiocsmru,
    &explain_iocontrol_pppiocsnpmode,
    &explain_iocontrol_pppiocspass,
    &explain_iocontrol_pppiocsrasyncmap,
    &explain_iocontrol_pppiocsxasyncmap,
    &explain_iocontrol_pppiocxferunit,
    &explain_iocontrol_siocadddlci,
    &explain_iocontrol_siocaddmulti,
    &explain_iocontrol_siocaddrt,
    &explain_iocontrol_siocatmark,
    &explain_iocontrol_siocbondchangeactive,
    &explain_iocontrol_siocbondenslave,
    &explain_iocontrol_siocbondinfoquery,
    &explain_iocontrol_siocbondrelease,
    &explain_iocontrol_siocbondsethwaddr,
    &explain_iocontrol_siocbondslaveinfoquery,
    &explain_iocontrol_siocbraddbr,
    &explain_iocontrol_siocbraddif,
    &explain_iocontrol_siocbrdelbr,
    &explain_iocontrol_siocbrdelif,
    &explain_iocontrol_siocdarp,
    &explain_iocontrol_siocdeldlci,
    &explain_iocontrol_siocdelmulti,
    &explain_iocontrol_siocdelrt,
    &explain_iocontrol_siocdifaddr,
    &explain_iocontrol_siocdrarp,
    &explain_iocontrol_siocethtool,
    &explain_iocontrol_siocgarp,
    &explain_iocontrol_siocgifaddr,
    &explain_iocontrol_siocgifbr,
    &explain_iocontrol_siocgifbrdaddr,
    &explain_iocontrol_siocgifconf,
    &explain_iocontrol_siocgifcount,
    &explain_iocontrol_siocgifdivert,
    &explain_iocontrol_siocgifdstaddr,
    &explain_iocontrol_siocgifencap,
    &explain_iocontrol_siocgifflags,
    &explain_iocontrol_siocgifhwaddr,
    &explain_iocontrol_siocgifindex,
    &explain_iocontrol_siocgifmap,
    &explain_iocontrol_siocgifmem,
    &explain_iocontrol_siocgifmetric,
    &explain_iocontrol_siocgifmtu,
    &explain_iocontrol_siocgifname,
    &explain_iocontrol_siocgifnetmask,
    &explain_iocontrol_siocgifpflags,
    &explain_iocontrol_siocgifslave,
    &explain_iocontrol_siocgiftxqlen,
    &explain_iocontrol_siocgifvlan,
    &explain_iocontrol_siocgmiiphy,
    &explain_iocontrol_siocgmiireg,
    &explain_iocontrol_siocgpgrp,
    &explain_iocontrol_siocgpppcstats,
    &explain_iocontrol_siocgpppstats,
    &explain_iocontrol_siocgpppver,
    &explain_iocontrol_siocgrarp,
    &explain_iocontrol_siocgstamp,
    &explain_iocontrol_siocgstampns,
    &explain_iocontrol_siocinq,
    &explain_iocontrol_siocoutq,
    &explain_iocontrol_siocrtmsg,
    &explain_iocontrol_siocsarp,
    &explain_iocontrol_siocscccal,
    &explain_iocontrol_siocscccfg,
    &explain_iocontrol_siocsccchanini,
    &explain_iocontrol_siocsccgkiss,
    &explain_iocontrol_siocsccgstat,
    &explain_iocontrol_siocsccini,
    &explain_iocontrol_siocsccskiss,
    &explain_iocontrol_siocsccsmem,
    &explain_iocontrol_siocshwtstamp,
    &explain_iocontrol_siocsifaddr,
    &explain_iocontrol_siocsifbr,
    &explain_iocontrol_siocsifbrdaddr,
    &explain_iocontrol_siocsifdivert,
    &explain_iocontrol_siocsifdstaddr,
    &explain_iocontrol_siocsifencap,
    &explain_iocontrol_siocsifflags,
    &explain_iocontrol_siocsifhwaddr,
    &explain_iocontrol_siocsifhwbroadcast,
    &explain_iocontrol_siocsiflink,
    &explain_iocontrol_siocsifmap,
    &explain_iocontrol_siocsifmem,
    &explain_iocontrol_siocsifmetric,
    &explain_iocontrol_siocsifmtu,
    &explain_iocontrol_siocsifname,
    &explain_iocontrol_siocsifnetmask,
    &explain_iocontrol_siocsifpflags,
    &explain_iocontrol_siocsifslave,
    &explain_iocontrol_siocsiftxqlen,
    &explain_iocontrol_siocsifvlan,
    &explain_iocontrol_siocsmiireg,
    &explain_iocontrol_siocspgrp,
    &explain_iocontrol_siocsrarp,
    &explain_iocontrol_siocwandev,
    &explain_iocontrol_siogifindex,
    &explain_iocontrol_tcflsh,
    &explain_iocontrol_tcgeta,
    &explain_iocontrol_tcgets,
    &explain_iocontrol_tcgets2,
    &explain_iocontrol_tcgetx,
    &explain_iocontrol_tcsbrk,
    &explain_iocontrol_tcsbrkp,
    &explain_iocontrol_tcseta,
    &explain_iocontrol_tcsetaf,
    &explain_iocontrol_tcsetaw,
    &explain_iocontrol_tcsets,
    &explain_iocontrol_tcsets2,
    &explain_iocontrol_tcsetsf,
    &explain_iocontrol_tcsetsf2,
    &explain_iocontrol_tcsetsw,
    &explain_iocontrol_tcsetsw2,
    &explain_iocontrol_tcsetx,
    &explain_iocontrol_tcsetxf,
    &explain_iocontrol_tcsetxw,
    &explain_iocontrol_tcxonc,
    &explain_iocontrol_tioccbrk,
    &explain_iocontrol_tioccons,
    &explain_iocontrol_tiocdrain,
    &explain_iocontrol_tiocexcl,
    &explain_iocontrol_tiocgdev,
    &explain_iocontrol_tiocgetc,
    &explain_iocontrol_tiocgetd,
    &explain_iocontrol_tiocgetp,
    &explain_iocontrol_tiocgetx,
    &explain_iocontrol_tiocghayesesp,
    &explain_iocontrol_tiocgicount,
    &explain_iocontrol_tiocglcktrmios,
    &explain_iocontrol_tiocgltc,
    &explain_iocontrol_tiocgpgrp,
    &explain_iocontrol_tiocgptn,
    &explain_iocontrol_tiocgrs485,
    &explain_iocontrol_tiocgserial,
    &explain_iocontrol_tiocgsid,
    &explain_iocontrol_tiocgsoftcar,
    &explain_iocontrol_tiocgwinsz,
    &explain_iocontrol_tiocinq,
    &explain_iocontrol_tioclget,
    &explain_iocontrol_tioclinux,
    &explain_iocontrol_tiocmbic,
    &explain_iocontrol_tiocmbis,
    &explain_iocontrol_tiocmget,
    &explain_iocontrol_tiocmiwait,
    &explain_iocontrol_tiocmset,
    &explain_iocontrol_tiocnotty,
    &explain_iocontrol_tiocnxcl,
    &explain_iocontrol_tiocoutq,
    &explain_iocontrol_tiocpkt,
    &explain_iocontrol_tiocsbrk,
    &explain_iocontrol_tiocsctty,
    &explain_iocontrol_tiocserconfig,
    &explain_iocontrol_tiocsergetlsr,
    &explain_iocontrol_tiocsergetmulti,
    &explain_iocontrol_tiocsergstruct,
    &explain_iocontrol_tiocsergwild,
    &explain_iocontrol_tiocsersetmulti,
    &explain_iocontrol_tiocserswild,
    &explain_iocontrol_tiocsetd,
    &explain_iocontrol_tiocshayesesp,
    &explain_iocontrol_tiocsig,
    &explain_iocontrol_tiocslcktrmios,
    &explain_iocontrol_tiocspgrp,
    &explain_iocontrol_tiocsptlck,
    &explain_iocontrol_tiocsrs485,
    &explain_iocontrol_tiocsserial,
    &explain_iocontrol_tiocssoftcar,
    &explain_iocontrol_tiocstart,
    &explain_iocontrol_tiocsti,
    &explain_iocontrol_tiocstop,
    &explain_iocontrol_tiocswinsz,
    &explain_iocontrol_tiocttygstruct,
    &explain_iocontrol_vidioc_cropcap,
    &explain_iocontrol_vidioc_dbg_g_chip_ident,
    &explain_iocontrol_vidioc_dbg_g_register,
    &explain_iocontrol_vidioc_dbg_s_register,
    &explain_iocontrol_vidioc_dqbuf,
    &explain_iocontrol_vidioc_dqevent,
    &explain_iocontrol_vidioc_encoder_cmd,
    &explain_iocontrol_vidioc_enum_dv_presets,
    &explain_iocontrol_vidioc_enum_fmt,
    &explain_iocontrol_vidioc_enum_frameintervals,
    &explain_iocontrol_vidioc_enum_framesizes,
    &explain_iocontrol_vidioc_enumaudio,
    &explain_iocontrol_vidioc_enumaudout,
    &explain_iocontrol_vidioc_enuminput,
    &explain_iocontrol_vidioc_enumoutput,
    &explain_iocontrol_vidioc_enumstd,
    &explain_iocontrol_vidioc_g_audio,
    &explain_iocontrol_vidioc_g_audout,
    &explain_iocontrol_vidioc_g_crop,
    &explain_iocontrol_vidioc_g_ctrl,
    &explain_iocontrol_vidioc_g_dv_preset,
    &explain_iocontrol_vidioc_g_dv_timings,
    &explain_iocontrol_vidioc_g_enc_index,
    &explain_iocontrol_vidioc_g_ext_ctrls,
    &explain_iocontrol_vidioc_g_fbuf,
    &explain_iocontrol_vidioc_g_fmt,
    &explain_iocontrol_vidioc_g_frequency,
    &explain_iocontrol_vidioc_g_input,
    &explain_iocontrol_vidioc_g_jpegcomp,
    &explain_iocontrol_vidioc_g_modulator,
    &explain_iocontrol_vidioc_g_output,
    &explain_iocontrol_vidioc_g_parm,
    &explain_iocontrol_vidioc_g_priority,
    &explain_iocontrol_vidioc_g_sliced_vbi_cap,
    &explain_iocontrol_vidioc_g_std,
    &explain_iocontrol_vidioc_g_tuner,
    &explain_iocontrol_vidioc_log_status,
    &explain_iocontrol_vidioc_overlay,
    &explain_iocontrol_vidioc_qbuf,
    &explain_iocontrol_vidioc_query_dv_preset,
    &explain_iocontrol_vidioc_querybuf,
    &explain_iocontrol_vidioc_querycap,
    &explain_iocontrol_vidioc_queryctrl,
    &explain_iocontrol_vidioc_querymenu,
    &explain_iocontrol_vidioc_querystd,
    &explain_iocontrol_vidioc_reqbufs,
    &explain_iocontrol_vidioc_s_audio,
    &explain_iocontrol_vidioc_s_audout,
    &explain_iocontrol_vidioc_s_crop,
    &explain_iocontrol_vidioc_s_ctrl,
    &explain_iocontrol_vidioc_s_dv_preset,
    &explain_iocontrol_vidioc_s_dv_timings,
    &explain_iocontrol_vidioc_s_ext_ctrls,
    &explain_iocontrol_vidioc_s_fbuf,
    &explain_iocontrol_vidioc_s_fmt,
    &explain_iocontrol_vidioc_s_frequency,
    &explain_iocontrol_vidioc_s_hw_freq_seek,
    &explain_iocontrol_vidioc_s_input,
    &explain_iocontrol_vidioc_s_jpegcomp,
    &explain_iocontrol_vidioc_s_modulator,
    &explain_iocontrol_vidioc_s_output,
    &explain_iocontrol_vidioc_s_parm,
    &explain_iocontrol_vidioc_s_priority,
    &explain_iocontrol_vidioc_s_std,
    &explain_iocontrol_vidioc_s_tuner,
    &explain_iocontrol_vidioc_streamoff,
    &explain_iocontrol_vidioc_streamon,
    &explain_iocontrol_vidioc_subscribe_event,
    &explain_iocontrol_vidioc_try_encoder_cmd,
    &explain_iocontrol_vidioc_try_ext_ctrls,
    &explain_iocontrol_vidioc_try_fmt,
    &explain_iocontrol_vidioc_unsubscribe_event,
    &explain_iocontrol_vidioccapture,
    &explain_iocontrol_vidiocgaudio,
    &explain_iocontrol_vidiocgcap,
    &explain_iocontrol_vidiocgchan,
    &explain_iocontrol_vidiocgfbuf,
    &explain_iocontrol_vidiocgfreq,
    &explain_iocontrol_vidiocgmbuf,
    &explain_iocontrol_vidiocgpict,
    &explain_iocontrol_vidiocgtuner,
    &explain_iocontrol_vidiocgvbifmt,
    &explain_iocontrol_vidiocgwin,
    &explain_iocontrol_vidiocmcapture,
    &explain_iocontrol_vidiocsaudio,
    &explain_iocontrol_vidiocschan,
    &explain_iocontrol_vidiocsfbuf,
    &explain_iocontrol_vidiocsfreq,
    &explain_iocontrol_vidiocspict,
    &explain_iocontrol_vidiocstuner,
    &explain_iocontrol_vidiocsvbifmt,
    &explain_iocontrol_vidiocswin,
    &explain_iocontrol_vidiocsync,
    &explain_iocontrol_vt_activate,
    &explain_iocontrol_vt_disallocate,
    &explain_iocontrol_vt_gethifontmask,
    &explain_iocontrol_vt_getmode,
    &explain_iocontrol_vt_getstate,
    &explain_iocontrol_vt_lockswitch,
    &explain_iocontrol_vt_openqry,
    &explain_iocontrol_vt_reldisp,
    &explain_iocontrol_vt_resize,
    &explain_iocontrol_vt_resizex,
    &explain_iocontrol_vt_sendsig,
    &explain_iocontrol_vt_setmode,
    &explain_iocontrol_vt_unlockswitch,
    &explain_iocontrol_vt_waitactive,
};

const size_t explain_iocontrol_table_size = SIZEOF(explain_iocontrol_table);


/* vim: set ts=8 sw=4 et : */
