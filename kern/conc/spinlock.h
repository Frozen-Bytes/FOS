// Mutual exclusion lock.
/*originally taken from xv6-x86 OS
 * USED ONLY FOR PROTECTION IN MULTI-CORE
 * Not designed for protection in a single core
 * */
#ifndef KERN_CONC_SPINLOCK_H_
#define KERN_CONC_SPINLOCK_H_

//=======================================================================
//TODO: [PROJECT'24.MS1 - #00 GIVENS] [4] LOCKS - SpinLock Implementation
struct spinlock {
  uint32 locked;       	// Is the lock held?

  // For debugging:
  char name[NAMELEN];	// Name of lock.
  struct cpu *cpu;   	// The cpu holding the lock.
  uint32 pcs[10];      	// The call stack (an array of program counters)
                     	// that locked the lock.
};
void init_spinlock(struct spinlock *lk, char *name);
void acquire_spinlock(struct spinlock *lk);
void release_spinlock(struct spinlock *lk);
int getcallerpcs(void *v, uint32 pcs[]) ;
void printcallstack(struct spinlock *lk);
int holding_spinlock(struct spinlock *lock);
//=======================================================================

#endif /*KERN_CONC_SPINLOCK_H_*/
