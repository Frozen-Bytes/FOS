/*
 * channel.h
 *
 *  Created on: Sep 22, 2024
 *      Author: HP
 */

#ifndef KERN_CONC_CHANNEL_H_
#define KERN_CONC_CHANNEL_H_

#include <kern/cpu/sched_helpers.h>
#include <kern/conc/spinlock.h>

//===================================================================================
//TODO: [PROJECT'24.MS1 - #00 GIVENS] [4] LOCKS - Channel struct & initialization
struct Channel
{
	struct Env_Queue queue;	//queue of blocked processes waiting on this channel
	char name[NAMELEN];     //channel name
};
void init_channel(struct Channel *chan, char *name);
//===================================================================================

void sleep(struct Channel *chan, struct spinlock* lk); 	//block the running process on the given channel (queue) using the given lk
void wakeup_one(struct Channel *chan);					//wakeup ONE blocked process on the given channel (queue)
void wakeup_all(struct Channel *chan);					//wakeup ALL blocked processes on the given channel (queue)


#endif /* KERN_CONC_CHANNEL_H_ */
