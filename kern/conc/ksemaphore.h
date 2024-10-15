// Long-term locks to be used in kernel
#ifndef KERN_CONC_KSEMAPHORE_H_
#define KERN_CONC_KSEMAPHORE_H_

#include <kern/conc/spinlock.h>
#include <kern/conc/channel.h>
#include <kern/cpu/sched_helpers.h>

struct ksemaphore
{
	int count;       			// Semaphore value
	struct spinlock lk; 		// spinlock protecting this count
	struct Channel chan;		// channel to hold all blocked processes on this semaphore

	// For debugging:
	char name[NAMELEN];        	// Name of semaphore.
};

void init_ksemaphore(struct ksemaphore *ksem, int value, char *name);
void wait_ksemaphore(struct ksemaphore *ksem);
void signal_ksemaphore(struct ksemaphore *ksem);

#endif /*KERN_CONC_KSEMAPHORE_H_*/
