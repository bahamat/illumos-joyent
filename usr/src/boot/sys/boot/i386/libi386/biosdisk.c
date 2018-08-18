/*
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
 * Copyright (c) 2012 Andrey V. Elsukov <ae@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

/*
 * BIOS disk device handling.
 *
 * Ideas and algorithms from:
 *
 * - NetBSD libi386/biosdisk.c
 * - FreeBSD biosboot/disk.c
 *
 */

#include <sys/disk.h>
#include <sys/limits.h>
#include <stand.h>
#include <machine/bootinfo.h>
#include <stdarg.h>

#include <bootstrap.h>
#include <btxv86.h>
#include <edd.h>
#include "disk.h"
#include "libi386.h"

#define	BIOS_NUMDRIVES		0x475
#define	BIOSDISK_SECSIZE	512
#define	BUFSIZE			(1 * BIOSDISK_SECSIZE)

#define	DT_ATAPI	0x10	/* disk type for ATAPI floppies */
#define	WDMAJOR		0	/* major numbers for devices we frontend for */
#define	WFDMAJOR	1
#define	FDMAJOR		2
#define	DAMAJOR		4

#ifdef DISK_DEBUG
#define	DEBUG(fmt, args...)	printf("%s: " fmt "\n", __func__, ## args)
#else
#define	DEBUG(fmt, args...)
#endif

/*
 * List of BIOS devices, translation from disk unit number to
 * BIOS unit number.
 */
static struct bdinfo
{
	int		bd_unit;	/* BIOS unit number */
	int		bd_cyl;		/* BIOS geometry */
	int		bd_hds;
	int		bd_sec;
	int		bd_flags;
#define	BD_MODEINT13	0x0000
#define	BD_MODEEDD1	0x0001
#define	BD_MODEEDD3	0x0002
#define	BD_MODEMASK	0x0003
#define	BD_FLOPPY	0x0004
	int		bd_type;	/* BIOS 'drive type' (floppy only) */
	uint16_t	bd_sectorsize;	/* Sector size */
	uint64_t	bd_sectors;	/* Disk size */
	int		bd_open;	/* reference counter */
	void		*bd_bcache;	/* buffer cache data */
} bdinfo [MAXBDDEV];
static int nbdinfo = 0;

#define	BD(dev)		(bdinfo[(dev)->dd.d_unit])

static void bd_io_workaround(struct disk_devdesc *dev);

static int bd_io(struct disk_devdesc *, daddr_t, int, caddr_t, int);
static int bd_int13probe(struct bdinfo *bd);

static int bd_init(void);
static int bd_strategy(void *devdata, int flag, daddr_t dblk, size_t size,
    char *buf, size_t *rsize);
static int bd_realstrategy(void *devdata, int flag, daddr_t dblk, size_t size,
    char *buf, size_t *rsize);
static int bd_open(struct open_file *f, ...);
static int bd_close(struct open_file *f);
static int bd_ioctl(struct open_file *f, u_long cmd, void *data);
static int bd_print(int verbose);

struct devsw biosdisk = {
	"disk",
	DEVT_DISK,
	bd_init,
	bd_strategy,
	bd_open,
	bd_close,
	bd_ioctl,
	bd_print,
	NULL
};

/*
 * Translate between BIOS device numbers and our private unit numbers.
 */
int
bd_bios2unit(int biosdev)
{
	int i;

	DEBUG("looking for bios device 0x%x", biosdev);
	for (i = 0; i < nbdinfo; i++) {
		DEBUG("bd unit %d is BIOS device 0x%x", i, bdinfo[i].bd_unit);
		if (bdinfo[i].bd_unit == biosdev)
			return (i);
	}
	return (-1);
}

int
bd_unit2bios(int unit)
{

	if ((unit >= 0) && (unit < nbdinfo))
		return (bdinfo[unit].bd_unit);
	return (-1);
}

/*
 * Quiz the BIOS for disk devices, save a little info about them.
 */
static int
bd_init(void)
{
	int base, unit, nfd = 0;

	/* sequence 0, 0x80 */
	for (base = 0; base <= 0x80; base += 0x80) {
		for (unit = base; (nbdinfo < MAXBDDEV); unit++) {
#ifndef VIRTUALBOX
			/*
			 * Check the BIOS equipment list for number
			 * of fixed disks.
			 */
			if (base == 0x80 &&
			    (nfd >= *(unsigned char *)PTOV(BIOS_NUMDRIVES)))
				break;
#endif
			bdinfo[nbdinfo].bd_open = 0;
			bdinfo[nbdinfo].bd_bcache = NULL;
			bdinfo[nbdinfo].bd_unit = unit;
			bdinfo[nbdinfo].bd_flags = unit < 0x80 ? BD_FLOPPY: 0;
			if (!bd_int13probe(&bdinfo[nbdinfo]))
				break;

#ifndef BOOT2
			/* XXX we need "disk aliases" to make this simpler */
			printf("BIOS drive %c: is disk%d\n", (unit < 0x80) ?
			    ('A' + unit): ('C' + unit - 0x80), nbdinfo);
#endif
			nbdinfo++;
			if (base == 0x80)
				nfd++;
		}
	}
	bcache_add_dev(nbdinfo);
	return (0);
}

/*
 * Try to detect a device supported by the legacy int13 BIOS
 */
static int
bd_int13probe(struct bdinfo *bd)
{
	struct edd_params params;
	int ret = 1;	/* assume success */

	v86.ctl = V86_FLAGS;
	v86.addr = 0x13;
	v86.eax = 0x800;
	v86.edx = bd->bd_unit;
	v86int();

	/* Don't error out if we get bad sector number, try EDD as well */
	if (V86_CY(v86.efl) ||	/* carry set */
	    (v86.edx & 0xff) <= (unsigned)(bd->bd_unit & 0x7f))	/* unit # bad */
		return (0);	/* skip device */

	if ((v86.ecx & 0x3f) == 0)	/* absurd sector number */
		ret = 0;	/* set error */

	/* Convert max cyl # -> # of cylinders */
	bd->bd_cyl = ((v86.ecx & 0xc0) << 2) + ((v86.ecx & 0xff00) >> 8) + 1;
	/* Convert max head # -> # of heads */
	bd->bd_hds = ((v86.edx & 0xff00) >> 8) + 1;
	bd->bd_sec = v86.ecx & 0x3f;
	bd->bd_type = v86.ebx & 0xff;
	bd->bd_flags |= BD_MODEINT13;

	/* Calculate sectors count from the geometry */
	bd->bd_sectors = bd->bd_cyl * bd->bd_hds * bd->bd_sec;
	bd->bd_sectorsize = BIOSDISK_SECSIZE;
	DEBUG("unit 0x%x geometry %d/%d/%d", bd->bd_unit, bd->bd_cyl,
	    bd->bd_hds, bd->bd_sec);

	/* Determine if we can use EDD with this device. */
	v86.ctl = V86_FLAGS;
	v86.addr = 0x13;
	v86.eax = 0x4100;
	v86.edx = bd->bd_unit;
	v86.ebx = 0x55aa;
	v86int();
	if (V86_CY(v86.efl) ||	/* carry set */
	    (v86.ebx & 0xffff) != 0xaa55 || /* signature */
	    (v86.ecx & EDD_INTERFACE_FIXED_DISK) == 0)
		return (ret);	/* return code from int13 AH=08 */

	/* EDD supported */
	bd->bd_flags |= BD_MODEEDD1;
	if ((v86.eax & 0xff00) >= 0x3000)
		bd->bd_flags |= BD_MODEEDD3;
	/* Get disk params */
	params.len = sizeof (struct edd_params);
	v86.ctl = V86_FLAGS;
	v86.addr = 0x13;
	v86.eax = 0x4800;
	v86.edx = bd->bd_unit;
	v86.ds = VTOPSEG(&params);
	v86.esi = VTOPOFF(&params);
	v86int();
	if (!V86_CY(v86.efl)) {
		uint64_t total;

		/*
		 * Sector size must be a multiple of 512 bytes.
		 * An alternate test would be to check power of 2,
		 * powerof2(params.sector_size).
		 */
		if (params.sector_size % BIOSDISK_SECSIZE)
			bd->bd_sectorsize = BIOSDISK_SECSIZE;
		else
			bd->bd_sectorsize = params.sector_size;

		total = bd->bd_sectorsize * params.sectors;
		if (params.sectors != 0) {
			/* Only update if we did not overflow. */
			if (total > params.sectors)
				bd->bd_sectors = params.sectors;
		}

		total = (uint64_t)params.cylinders *
		    params.heads * params.sectors_per_track;
		if (bd->bd_sectors < total)
			bd->bd_sectors = total;

		ret = 1;
	}
	DEBUG("unit 0x%x flags %x, sectors %llu, sectorsize %u",
	    bd->bd_unit, bd->bd_flags, bd->bd_sectors, bd->bd_sectorsize);
	return (ret);
}

/*
 * Print information about disks
 */
static int
bd_print(int verbose)
{
	static char line[80];
	struct disk_devdesc dev;
	int i, ret = 0;

	if (nbdinfo == 0)
		return (0);

	printf("%s devices:", biosdisk.dv_name);
	if ((ret = pager_output("\n")) != 0)
		return (ret);

	for (i = 0; i < nbdinfo; i++) {
		snprintf(line, sizeof (line),
		    "    disk%d:   BIOS drive %c (%ju X %u):\n", i,
		    (bdinfo[i].bd_unit < 0x80) ? ('A' + bdinfo[i].bd_unit):
		    ('C' + bdinfo[i].bd_unit - 0x80),
		    (uintmax_t)bdinfo[i].bd_sectors,
		    bdinfo[i].bd_sectorsize);
		if ((ret = pager_output(line)) != 0)
			break;

		dev.dd.d_dev = &biosdisk;
		dev.dd.d_unit = i;
		dev.d_slice = -1;
		dev.d_partition = -1;
		if (disk_open(&dev,
		    bdinfo[i].bd_sectorsize * bdinfo[i].bd_sectors,
		    bdinfo[i].bd_sectorsize) == 0) {
			snprintf(line, sizeof (line), "    disk%d", i);
			ret = disk_print(&dev, line, verbose);
			disk_close(&dev);
			if (ret != 0)
				break;
		}
	}
	return (ret);
}

/*
 * Attempt to open the disk described by (dev) for use by (f).
 *
 * Note that the philosophy here is "give them exactly what
 * they ask for".  This is necessary because being too "smart"
 * about what the user might want leads to complications.
 * (eg. given no slice or partition value, with a disk that is
 *  sliced - are they after the first BSD slice, or the DOS
 *  slice before it?)
 */
static int
bd_open(struct open_file *f, ...)
{
	struct disk_devdesc *dev;
	struct disk_devdesc disk;
	va_list ap;
	uint64_t size;
	int rc;

	va_start(ap, f);
	dev = va_arg(ap, struct disk_devdesc *);
	va_end(ap);

	if (dev->dd.d_unit < 0 || dev->dd.d_unit >= nbdinfo)
		return (EIO);
	BD(dev).bd_open++;
	if (BD(dev).bd_bcache == NULL)
	    BD(dev).bd_bcache = bcache_allocate();

	/*
	 * Read disk size from partition.
	 * This is needed to work around buggy BIOS systems returning
	 * wrong (truncated) disk media size.
	 * During bd_probe() we tested if the mulitplication of bd_sectors
	 * would overflow so it should be safe to perform here.
	 */
	disk.dd.d_dev = dev->dd.d_dev;
	disk.dd.d_unit = dev->dd.d_unit;
	disk.d_slice = -1;
	disk.d_partition = -1;
	disk.d_offset = 0;

	if (disk_open(&disk, BD(dev).bd_sectors * BD(dev).bd_sectorsize,
	    BD(dev).bd_sectorsize) == 0) {

		if (disk_ioctl(&disk, DIOCGMEDIASIZE, &size) == 0) {
			size /= BD(dev).bd_sectorsize;
			if (size > BD(dev).bd_sectors)
				BD(dev).bd_sectors = size;
		}
		disk_close(&disk);
	}

	rc = disk_open(dev, BD(dev).bd_sectors * BD(dev).bd_sectorsize,
	    BD(dev).bd_sectorsize);
	if (rc != 0) {
		BD(dev).bd_open--;
		if (BD(dev).bd_open == 0) {
			bcache_free(BD(dev).bd_bcache);
			BD(dev).bd_bcache = NULL;
		}
	}
	return (rc);
}

static int
bd_close(struct open_file *f)
{
	struct disk_devdesc *dev;

	dev = (struct disk_devdesc *)f->f_devdata;
	BD(dev).bd_open--;
	if (BD(dev).bd_open == 0) {
	    bcache_free(BD(dev).bd_bcache);
	    BD(dev).bd_bcache = NULL;
	}
	return (disk_close(dev));
}

static int
bd_ioctl(struct open_file *f, u_long cmd, void *data)
{
	struct disk_devdesc *dev;
	int rc;

	dev = (struct disk_devdesc *)f->f_devdata;

	rc = disk_ioctl(dev, cmd, data);
	if (rc != ENOTTY)
		return (rc);

	switch (cmd) {
	case DIOCGSECTORSIZE:
		*(uint32_t *)data = BD(dev).bd_sectorsize;
		break;
	case DIOCGMEDIASIZE:
		*(uint64_t *)data = BD(dev).bd_sectors * BD(dev).bd_sectorsize;
		break;
	default:
		return (ENOTTY);
	}
	return (0);
}

static int
bd_strategy(void *devdata, int rw, daddr_t dblk, size_t size,
    char *buf, size_t *rsize)
{
	struct bcache_devdata bcd;
	struct disk_devdesc *dev;

	dev = (struct disk_devdesc *)devdata;
	bcd.dv_strategy = bd_realstrategy;
	bcd.dv_devdata = devdata;
	bcd.dv_cache = BD(dev).bd_bcache;
	return (bcache_strategy(&bcd, rw, dblk + dev->d_offset, size,
	    buf, rsize));
}

static int
bd_realstrategy(void *devdata, int rw, daddr_t dblk, size_t size,
    char *buf, size_t *rsize)
{
	struct disk_devdesc *dev = (struct disk_devdesc *)devdata;
	uint64_t disk_blocks, offset;
	size_t blks, blkoff, bsize, rest;
	caddr_t bbuf;
	int rc;

	/*
	 * First make sure the IO size is a multiple of 512 bytes. While we do
	 * process partial reads below, the strategy mechanism is built
	 * assuming IO is a multiple of 512B blocks. If the request is not
	 * a multiple of 512B blocks, it has to be some sort of bug.
	 */
	if (size == 0 || (size % BIOSDISK_SECSIZE) != 0) {
		printf("bd_strategy: %d bytes I/O not multiple of %d\n",
		    size, BIOSDISK_SECSIZE);
		return (EIO);
	}

	DEBUG("open_disk %p", dev);

	offset = dblk * BIOSDISK_SECSIZE;
	dblk = offset / BD(dev).bd_sectorsize;
	blkoff = offset % BD(dev).bd_sectorsize;

	/*
	 * Check the value of the size argument. We do have quite small
	 * heap (64MB), but we do not know good upper limit, so we check against
	 * INT_MAX here. This will also protect us against possible overflows
	 * while translating block count to bytes.
	 */
	if (size > INT_MAX) {
		DEBUG("too large I/O: %zu bytes", size);
		return (EIO);
	}

	blks = size / BD(dev).bd_sectorsize;
	if (blks == 0 || (size % BD(dev).bd_sectorsize) != 0)
		blks++;

	if (dblk > dblk + blks)
		return (EIO);

	if (rsize)
		*rsize = 0;

	/*
	 * Get disk blocks, this value is either for whole disk or for
	 * partition.
	 */
	if (disk_ioctl(dev, DIOCGMEDIASIZE, &disk_blocks) == 0) {
		/* DIOCGMEDIASIZE does return bytes. */
		disk_blocks /= BD(dev).bd_sectorsize;
	} else {
		/* We should not get here. Just try to survive. */
		disk_blocks = BD(dev).bd_sectors - dev->d_offset;
	}

	/* Validate source block address. */
	if (dblk < dev->d_offset || dblk >= dev->d_offset + disk_blocks)
		return (EIO);

	/*
	 * Truncate if we are crossing disk or partition end.
	 */
	if (dblk + blks >= dev->d_offset + disk_blocks) {
		blks = dev->d_offset + disk_blocks - dblk;
		size = blks * BD(dev).bd_sectorsize;
		DEBUG("short I/O %d", blks);
	}

	if (V86_IO_BUFFER_SIZE / BD(dev).bd_sectorsize == 0)
		panic("BUG: Real mode buffer is too small\n");

	bbuf = PTOV(V86_IO_BUFFER);
	rest = size;

	while (blks > 0) {
		int x = min(blks, V86_IO_BUFFER_SIZE / BD(dev).bd_sectorsize);

		switch (rw & F_MASK) {
		case F_READ:
			DEBUG("read %d from %lld to %p", x, dblk, buf);
			bsize = BD(dev).bd_sectorsize * x - blkoff;
			if (rest < bsize)
				bsize = rest;

			if ((rc = bd_io(dev, dblk, x, bbuf, 0)) != 0)
				return (EIO);

			bcopy(bbuf + blkoff, buf, bsize);
			break;
		case F_WRITE :
			DEBUG("write %d from %lld to %p", x, dblk, buf);
			if (blkoff != 0) {
				/*
				 * We got offset to sector, read 1 sector to
				 * bbuf.
				 */
				x = 1;
				bsize = BD(dev).bd_sectorsize - blkoff;
				bsize = min(bsize, rest);
				rc = bd_io(dev, dblk, x, bbuf, 0);
			} else if (rest < BD(dev).bd_sectorsize) {
				/*
				 * The remaining block is not full
				 * sector. Read 1 sector to bbuf.
				 */
				x = 1;
				bsize = rest;
				rc = bd_io(dev, dblk, x, bbuf, 0);
			} else {
				/* We can write full sector(s). */
				bsize = BD(dev).bd_sectorsize * x;
			}
			/*
			 * Put your Data In, Put your Data out,
			 * Put your Data In, and shake it all about
			 */
			bcopy(buf, bbuf + blkoff, bsize);
			if ((rc = bd_io(dev, dblk, x, bbuf, 1)) != 0)
				return (EIO);

			break;
		default:
			/* DO NOTHING */
			return (EROFS);
		}

		blkoff = 0;
		buf += bsize;
		rest -= bsize;
		blks -= x;
		dblk += x;
	}

	if (rsize != NULL)
		*rsize = size;
	return (0);
}

static int
bd_edd_io(struct disk_devdesc *dev, daddr_t dblk, int blks, caddr_t dest,
    int dowrite)
{
	static struct edd_packet packet;

	packet.len = sizeof (struct edd_packet);
	packet.count = blks;
	packet.off = VTOPOFF(dest);
	packet.seg = VTOPSEG(dest);
	packet.lba = dblk;
	v86.ctl = V86_FLAGS;
	v86.addr = 0x13;
	/* Should we Write with verify ?? 0x4302 ? */
	if (dowrite)
		v86.eax = 0x4300;
	else
		v86.eax = 0x4200;
	v86.edx = BD(dev).bd_unit;
	v86.ds = VTOPSEG(&packet);
	v86.esi = VTOPOFF(&packet);
	v86int();
	if (V86_CY(v86.efl))
		return (v86.eax >> 8);
	return (0);
}

static int
bd_chs_io(struct disk_devdesc *dev, daddr_t dblk, int blks, caddr_t dest,
    int dowrite)
{
	uint32_t x, bpc, cyl, hd, sec;

	bpc = BD(dev).bd_sec * BD(dev).bd_hds;	/* blocks per cylinder */
	x = dblk;
	cyl = x / bpc;			/* block # / blocks per cylinder */
	x %= bpc;				/* block offset into cylinder */
	hd = x / BD(dev).bd_sec;		/* offset / blocks per track */
	sec = x % BD(dev).bd_sec;		/* offset into track */

	/* correct sector number for 1-based BIOS numbering */
	sec++;

	if (cyl > 1023) {
		/* CHS doesn't support cylinders > 1023. */
		return (1);
	}

	v86.ctl = V86_FLAGS;
	v86.addr = 0x13;
	if (dowrite)
		v86.eax = 0x300 | blks;
	else
		v86.eax = 0x200 | blks;
	v86.ecx = ((cyl & 0xff) << 8) | ((cyl & 0x300) >> 2) | sec;
	v86.edx = (hd << 8) | BD(dev).bd_unit;
	v86.es = VTOPSEG(dest);
	v86.ebx = VTOPOFF(dest);
	v86int();
	if (V86_CY(v86.efl))
		return (v86.eax >> 8);
	return (0);
}

static void
bd_io_workaround(struct disk_devdesc *dev)
{
	uint8_t buf[8 * 1024];

	bd_edd_io(dev, 0xffffffff, 1, (caddr_t)buf, 0);
}

static int
bd_io(struct disk_devdesc *dev, daddr_t dblk, int blks, caddr_t dest,
    int dowrite)
{
	int result, retry;

	/* Just in case some idiot actually tries to read/write -1 blocks... */
	if (blks < 0)
		return (-1);

	/*
	 * Workaround for a problem with some HP ProLiant BIOS failing to work
	 * out the boot disk after installation. hrs and kuriyama discovered
	 * this problem with an HP ProLiant DL320e Gen 8 with a 3TB HDD, and
	 * discovered that an int13h call seems to cause a buffer overrun in
	 * the bios. The problem is alleviated by doing an extra read before
	 * the buggy read. It is not immediately known whether other models
	 * are similarly affected.
	 * Loop retrying the operation a couple of times.  The BIOS
	 * may also retry.
	 */
	if (dowrite == 0 && dblk >= 0x100000000)
		bd_io_workaround(dev);
	for (retry = 0; retry < 3; retry++) {
		/* if retrying, reset the drive */
		if (retry > 0) {
			v86.ctl = V86_FLAGS;
			v86.addr = 0x13;
			v86.eax = 0;
			v86.edx = BD(dev).bd_unit;
			v86int();
		}

		if (BD(dev).bd_flags & BD_MODEEDD1)
			result = bd_edd_io(dev, dblk, blks, dest, dowrite);
		else
			result = bd_chs_io(dev, dblk, blks, dest, dowrite);

		if (result == 0)
			break;
	}

	/*
	 * 0x20 - Controller failure. This is common error when the
	 * media is not present.
	 */
	if (result != 0 && result != 0x20) {
		if (dowrite != 0) {
			printf("%s%d: Write %d sector(s) from %p (0x%x) "
			    "to %lld: 0x%x\n", dev->dd.d_dev->dv_name,
			    dev->dd.d_unit, blks, dest, VTOP(dest), dblk,
			    result);
		} else {
			printf("%s%d: Read %d sector(s) from %lld to %p "
			    "(0x%x): 0x%x\n", dev->dd.d_dev->dv_name,
			    dev->dd.d_unit, blks, dblk, dest, VTOP(dest),
			    result);
		}
	}

	return (result);
}

/*
 * Return the BIOS geometry of a given "fixed drive" in a format
 * suitable for the legacy bootinfo structure.  Since the kernel is
 * expecting raw int 0x13/0x8 values for N_BIOS_GEOM drives, we
 * prefer to get the information directly, rather than rely on being
 * able to put it together from information already maintained for
 * different purposes and for a probably different number of drives.
 *
 * For valid drives, the geometry is expected in the format (31..0)
 * "000000cc cccccccc hhhhhhhh 00ssssss"; and invalid drives are
 * indicated by returning the geometry of a "1.2M" PC-format floppy
 * disk.  And, incidentally, what is returned is not the geometry as
 * such but the highest valid cylinder, head, and sector numbers.
 */
uint32_t
bd_getbigeom(int bunit)
{

	v86.ctl = V86_FLAGS;
	v86.addr = 0x13;
	v86.eax = 0x800;
	v86.edx = 0x80 + bunit;
	v86int();
	if (V86_CY(v86.efl))
		return (0x4f010f);
	return (((v86.ecx & 0xc0) << 18) | ((v86.ecx & 0xff00) << 8) |
	    (v86.edx & 0xff00) | (v86.ecx & 0x3f));
}

/*
 * Return a suitable dev_t value for (dev).
 *
 * In the case where it looks like (dev) is a SCSI disk, we allow the number of
 * IDE disks to be specified in $num_ide_disks.  There should be a Better Way.
 */
int
bd_getdev(struct i386_devdesc *d)
{
	struct disk_devdesc *dev;
	int	biosdev;
	int	major;
	int	rootdev;
	char	*nip, *cp;
	int	i, unit;

	dev = (struct disk_devdesc *)d;
	biosdev = bd_unit2bios(dev->dd.d_unit);
	DEBUG("unit %d BIOS device %d", dev->dd.d_unit, biosdev);
	if (biosdev == -1)			/* not a BIOS device */
		return (-1);
	if (disk_open(dev, BD(dev).bd_sectors * BD(dev).bd_sectorsize,
	    BD(dev).bd_sectorsize) != 0)	/* oops, not a viable device */
		return (-1);
	else
		disk_close(dev);

	if (biosdev < 0x80) {
		/* floppy (or emulated floppy) or ATAPI device */
		if (bdinfo[dev->dd.d_unit].bd_type == DT_ATAPI) {
			/* is an ATAPI disk */
			major = WFDMAJOR;
		} else {
			/* is a floppy disk */
			major = FDMAJOR;
		}
	} else {
		/* assume an IDE disk */
		major = WDMAJOR;
	}
	/* default root disk unit number */
	unit = biosdev & 0x7f;

	/* XXX a better kludge to set the root disk unit number */
	if ((nip = getenv("root_disk_unit")) != NULL) {
		i = strtol(nip, &cp, 0);
		/* check for parse error */
		if ((cp != nip) && (*cp == 0))
			unit = i;
	}

	rootdev = MAKEBOOTDEV(major, dev->d_slice + 1, unit, dev->d_partition);
	DEBUG("dev is 0x%x\n", rootdev);
	return (rootdev);
}