#ifndef DISK_H
#define DISK_H

//#include <inc/lib.h>
#include <inc/types.h>
#include <inc/assert.h>
#include <kern/conc/channel.h>
#include <kern/conc/ksemaphore.h>

#define SECTSIZE	512			// bytes per disk sector
#define BLKSECTS	(BLKSIZE / SECTSIZE)	// sectors per block

/* Disk block n, when in memory, is mapped into the file system
 * server's address space at DISKMAP + (n*BLKSIZE). */
#define DISKMAP		0x10000000

/* Maximum disk size we can handle (3GB) */
#define DISKSIZE	0xC0000000

/* ide.c */
//bool	ide_probe_disk1(void);
//void	ide_set_disk(int diskno);
void ide_init();
int	ide_read(uint32 secno, void *dst, uint32 nsecs);
int	ide_write(uint32 secno, const void *src, uint32 nsecs);

#define DISK_INT_BLK_METHOD LCK_SLEEP 	//Specify the method of handling the block/release on DISK
struct Channel DISKchannel;				//channel of waiting for DISK
struct spinlock DISKlock;				//spinlock to protect the DISKchannel
struct ksemaphore DISKsem;				//semaphore to manage DISK interrupts

#endif	// !DISK_H
