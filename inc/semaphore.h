// user-level semaphore
#ifndef FOS_INC_SEMAPHORE_H
#define FOS_INC_SEMAPHORE_H

#include <inc/environment_definitions.h>

struct __semdata
{
	//queue of all blocked envs on this Semaphore
	struct Env_Queue queue;

	//semaphore value
	int count;

	//lock variable protecting this count
	uint32 lock;

	// For debugging: Name of semaphore.
	char name[64];
};
struct semaphore
{
	struct __semdata* semdata ;
};
struct semaphore create_semaphore(char *semaphoreName, uint32 value);
struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName);

void wait_semaphore(struct semaphore sem);
void signal_semaphore(struct semaphore sem);
int semaphore_count(struct semaphore sem);

#endif /*FOS_INC_SEMAPHORE_H*/
