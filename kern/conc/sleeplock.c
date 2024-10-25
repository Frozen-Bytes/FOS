// Sleeping locks

#include "inc/types.h"
#include "inc/x86.h"
#include "inc/memlayout.h"
#include "inc/mmu.h"
#include "inc/environment_definitions.h"
#include "inc/assert.h"
#include "inc/string.h"
#include "sleeplock.h"
#include "channel.h"
#include "../cpu/cpu.h"
#include "../proc/user_environment.h"

void init_sleeplock(struct sleeplock *lk, char *name)
{
	init_channel(&(lk->chan), "sleep lock channel");
	init_spinlock(&(lk->lk), "lock of sleep lock");
	strcpy(lk->name, name);
	lk->locked = 0;
	lk->pid = 0;
}
int holding_sleeplock(struct sleeplock *lk)
{
	int r;
	acquire_spinlock(&(lk->lk));
	r = lk->locked && (lk->pid == get_cpu_proc()->env_id);
	release_spinlock(&(lk->lk));
	return r;
}
//==========================================================================

void acquire_sleeplock(struct sleeplock *lk)
{
	//TODO: [PROJECT'24.MS1 - #13] [4] LOCKS - acquire_sleeplock
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	// panic("acquire_sleeplock is not implemented yet");
	//Your Code is Here...

	assert(lk);

	if (holding_sleeplock(lk)) {
		panic("acquire_sleeplock: lock \"%s\" is already held by the same CPU.", lk->name);
	}

	acquire_spinlock(&lk->lk);

	while (lk->locked) {
		sleep(&lk->chan, &lk->lk);
	}
	lk->locked = 1;

	// set debug information
	struct Env *env = get_cpu_proc();
	lk->pid = env->env_id;
	strncpy(lk->name, env->prog_name, NAMELEN);

	release_spinlock(&lk->lk);
}

void release_sleeplock(struct sleeplock *lk)
{
	//TODO: [PROJECT'24.MS1 - #14] [4] LOCKS - release_sleeplock
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	// panic("release_sleeplock is not implemented yet");
	//Your Code is Here...

	assert(lk);

	if (!holding_sleeplock(lk)) {
		printcallstack_sleeplock(lk);
		panic("release_sleeplock: lock \"%s\" is either not held or held by another CPU!", lk->name);
	}

	acquire_spinlock(&lk->lk);

	// clear debug information
	lk->pid = 0;
	memset(lk->name, '\0', NAMELEN);

	lk->locked = 0;
	if (queue_size(&lk->chan.queue) > 0) {
		wakeup_all(&lk->chan);
	}

	release_spinlock(&lk->lk);
}

void
printcallstack_sleeplock(const struct sleeplock *lk)
{
	cprintf("\nCaller Stack:\n");
	uint32 pcs[10] = {};
	int stacklen = 	getcallerpcs(&lk,  pcs);
	for (int i = 0; i < stacklen; ++i) {
		cprintf("  PC[%d] = %x\n", i, pcs[i]);
	}
}
