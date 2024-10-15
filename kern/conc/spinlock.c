// Mutual exclusion spin locks.
/*originally taken from xv6-x86 OS
 * USED ONLY FOR PROTECTION IN MULTI-CORE
 * Not designed for protection in a single core
 * */
#include "inc/types.h"
#include "inc/x86.h"
#include "inc/memlayout.h"
#include "inc/mmu.h"
#include "inc/string.h"
#include "inc/environment_definitions.h"
#include "inc/assert.h"
#include "spinlock.h"
#include "../cpu/cpu.h"
#include "../proc/user_environment.h"

void init_spinlock(struct spinlock *lk, char *name)
{
	strcpy(lk->name, name);
	lk->locked = 0;
	lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void acquire_spinlock(struct spinlock *lk)
{
	if(holding_spinlock(lk))
		panic("acquire_spinlock: lock \"%s\" is already held by the same CPU.", lk->name);

	pushcli(); // disable interrupts to avoid deadlock (in case if interrupted from a higher priority (or event handler) just after holding the lock => the handler will stuck in busy-waiting and prevent the other from resuming)

	//cprintf("\nAttempt to acquire SPIN lock [%s] by [%d]\n", lk->name, myproc() != NULL? myproc()->env_id : 0);

	// The xchg is atomic.
	while(xchg(&lk->locked, 1) != 0) ;

	//cprintf("SPIN lock [%s] is ACQUIRED  by [%d]\n", lk->name, myproc() != NULL? myproc()->env_id : 0);

	// Tell the C compiler and the processor to not move loads or stores
	// past this point, to ensure that the critical section's memory
	// references happen after the lock is acquired.
	__sync_synchronize();

	// Record info about lock acquisition for debugging.
	lk->cpu = mycpu();
	getcallerpcs(&lk, lk->pcs);

}

// Release the lock.
void release_spinlock(struct spinlock *lk)
{
	if(!holding_spinlock(lk))
	{
		printcallstack(lk);
		panic("release: lock \"%s\" is either not held or held by another CPU!", lk->name);
	}
	lk->pcs[0] = 0;
	lk->cpu = 0;

	// Tell the C compiler and the processor to not move loads or stores
	// past this point, to ensure that all the stores in the critical
	// section are visible to other cores before the lock is released.
	// Both the C compiler and the hardware may re-order loads and
	// stores; __sync_synchronize() tells them both not to.
	__sync_synchronize();

	// Release the lock, equivalent to lk->locked = 0.
	// This code can't use a C assignment, since it might
	// not be atomic. A real OS would use C atomics here.
	asm volatile("movl $0, %0" : "+m" (lk->locked) : );

	popcli();
}

// Record the current call stack in pcs[] by following the %ebp chain.
int getcallerpcs(void *v, uint32 pcs[])
{
	uint32 *ebp;
	int i;
	struct Env* p = get_cpu_proc();
	struct cpu* c = mycpu();
	ebp = (uint32*)v - 2;
	for(i = 0; i < 10; i++)
	{
		//cprintf("old ebp = %x\n", ebp);
		if	(	ebp == 0 || (ebp < (uint32*) USER_LIMIT) ||
				(ebp >= (uint32*)(c->stack + KERNEL_STACK_SIZE) && ebp < (uint32*)(c->stack + KERNEL_STACK_SIZE + PAGE_SIZE)) ||
				(p && ebp >= (uint32*) (p->kstack + KERNEL_STACK_SIZE)))
			break;
		pcs[i] = ebp[1];     // saved %eip
		ebp = (uint32*)ebp[0]; // saved %ebp
		//		cprintf("new ebp = %x\n", ebp);
		//		cprintf("pc[%d] = %x\n", i, pcs[i]);
	}
	int length = i ;
	for(; i < 10; i++)
		pcs[i] = 0;
	return length ;
}

void printcallstack(struct spinlock *lk)
{
	cprintf("\nCaller Stack:\n");
	int stacklen = 	getcallerpcs(&lk, lk->pcs);
	for (int i = 0; i < stacklen; ++i) {
		cprintf("  PC[%d] = %x\n", i, lk->pcs[i]);
	}
}
// Check whether this cpu is holding the lock.
int holding_spinlock(struct spinlock *lock)
{
	int r;
	pushcli();
	r = lock->locked && lock->cpu == mycpu();
	popcli();
	return r;
}

