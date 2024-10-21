/*
 * channel.c
 *
 *  Created on: Sep 22, 2024
 *      Author: HP
 */
#include "channel.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <inc/string.h>
#include <inc/disk.h>

//===============================
// 1) INITIALIZE THE CHANNEL:
//===============================
// initialize its lock & queue
void init_channel(struct Channel *chan, char *name)
{
	strcpy(chan->name, name);
	init_queue(&(chan->queue));
}

//===============================
// 2) SLEEP ON A GIVEN CHANNEL:
//===============================
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// Ref: xv6-x86 OS code
void sleep(struct Channel *chan, struct spinlock *lk)
{
	//TODO: [PROJECT'24.MS1 - #10] [4] LOCKS - sleep

	// To be able to release the sleep lock guard without missing wake-up
	acquire_spinlock(ProcessQueues.qlock);

	// Release the given sleep lock guard before being blocked
	release_spinlock(lk);

	// Enqueue the current process into the given waiting queue and block it
	struct Env *current_running_process = get_cpu_proc();
	enqueue(chan->queue, current_running_process);
	current_running_process->env_status = ENV_BLOCKED;

	// Release process queues lock to enable other processes to wake me
	release_spinlock(ProcessQueues.qlock);

	// Let the scheduler go back to scheduling ready processes
	sched();

	// When the 'current_running_process' that was blocked wakes up
	// It will wake up here, as soon as it wakes up it should reacquire
	// the guard so that when it goes back to the 'acquire_sleeplock'
	// function's while loop, if it finds the sleep lock closed,
	// it is still able to go to sleep again without missing wake-up
	acquire_spinlock(lk);
}

//==================================================
// 3) WAKEUP ONE BLOCKED PROCESS ON A GIVEN CHANNEL:
//==================================================
// Wake up ONE process sleeping on chan.
// The qlock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes
void wakeup_one(struct Channel *chan)
{
	//TODO: [PROJECT'24.MS1 - #11] [4] LOCKS - wakeup_one
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("wakeup_one is not implemented yet");
	//Your Code is Here...
}

//====================================================
// 4) WAKEUP ALL BLOCKED PROCESSES ON A GIVEN CHANNEL:
//====================================================
// Wake up all processes sleeping on chan.
// The queues lock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes

void wakeup_all(struct Channel *chan)
{
	//TODO: [PROJECT'24.MS1 - #12] [4] LOCKS - wakeup_all
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("wakeup_all is not implemented yet");
	//Your Code is Here...

}

