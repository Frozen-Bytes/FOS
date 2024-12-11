// User-level Semaphore

#include "inc/lib.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	//TODO: [PROJECT'24.MS3 - #02] [2] USER-LEVEL SEMAPHORE - create_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	// panic("create_semaphore is not implemented yet");
	//Your Code is Here...

	struct semaphore sem = { 0 };

	// Wrap me up, baby!
	sem.semdata = smalloc(semaphoreName, sizeof(struct __semdata), 1);
	if (sem.semdata) {
		memset(sem.semdata, 0, sizeof(struct __semdata));

		// Should `init_queue` be a system call???
		// Why limit queue utilities to the kernel only?
		// Bro, this does not make any sense
		LIST_INIT(&sem.semdata->queue);
		sem.semdata->count = value;
		sem.semdata->lock = 0;

		uint32 name_buffer_len = sizeof(sem.semdata->name) / sizeof(*sem.semdata->name);
		strncpy(sem.semdata->name, semaphoreName, name_buffer_len);
		sem.semdata->name[name_buffer_len - 1] = '\0';
	}

	return sem;
}

struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	//TODO: [PROJECT'24.MS3 - #03] [2] USER-LEVEL SEMAPHORE - get_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	// panic("get_semaphore is not implemented yet");
	//Your Code is Here...

	// And that's a wrap, everyone!
	struct semaphore sem = { sget(ownerEnvID, semaphoreName) };

	return sem;
}

void wait_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #04] [2] USER-LEVEL SEMAPHORE - wait_semaphore
	
	uint32 keyw = 1;
	
	while(xchg(&(sem.semdata->lock), keyw) != 0);

	sem.semdata->count--;
	
	if (sem.semdata->count < 0){
		block_and_schedule_next(sem.semdata);
	}
	
	sem.semdata->lock = 0;
}

void signal_semaphore(struct semaphore sem)
{
	//TODO: [PROJECT'24.MS3 - #05] [2] USER-LEVEL SEMAPHORE - signal_semaphore

	uint32 keys = 1;

	while(xchg(&(sem.semdata->lock), keys) != 0);
	
	sem.semdata->count++;
	
	if (sem.semdata->count <= 0) {
		unblock_and_enqueue_ready(sem.semdata);
	}

	sem.semdata->lock = 0;
}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
