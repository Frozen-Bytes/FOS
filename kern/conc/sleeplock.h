// Long-term locks for processes
/*originally taken from xv6-x86 OS
 * */
#ifndef KERN_CONC_SLEEPLOCK_H_
#define KERN_CONC_SLEEPLOCK_H_

#include <kern/conc/spinlock.h>
#include <kern/conc/channel.h>
#include <kern/cpu/sched_helpers.h>

//===================================================================================
//TODO: [PROJECT'24.MS1 - #00 GIVENS] [4] LOCKS - SleepLock struct & helper functions
struct sleeplock
{
	bool locked;       		// Is the lock held?
	struct spinlock lk; 	// spinlock protecting this sleep lock
	struct Channel chan;	// channel to hold all blocked processes on this lock
	// For debugging:
	char name[NAMELEN];    	// Name of lock.
	int pid;           		// Process holding lock
};

void init_sleeplock(struct sleeplock *lk, char *name);
int  holding_sleeplock(struct sleeplock *lk);
//===================================================================================

void acquire_sleeplock(struct sleeplock *lk);
void release_sleeplock(struct sleeplock *lk);

#endif /*KERN_CONC_SLEEPLOCK_H_*/
