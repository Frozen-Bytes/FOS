// Test the use of semaphores to allow multiprograms to enter the CS at same time
// Master program: take user input, create the semaphores, run slaves and wait them to finish
#include <inc/lib.h>

void
_main(void)
{
	int envID = sys_getenvid();
	char line[256] ;
	readline("Enter total number of customers: ", line) ;
	int totalNumOfCusts = strtol(line, NULL, 10);
	readline("Enter shop capacity: ", line) ;
	int shopCapacity = strtol(line, NULL, 10);

	struct semaphore shopCapacitySem = create_semaphore("shopCapacity", shopCapacity);
	struct semaphore dependSem = create_semaphore("depend", 0);

	int i = 0 ;
	int id ;
	for (; i<totalNumOfCusts; i++)
	{
		id = sys_create_env("sem2Slave", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
		if (id == E_ENV_CREATION_ERROR)
			panic("NO AVAILABLE ENVs... Please reduce the number of customers and try again...");
		sys_run_env(id) ;
	}

	for (i = 0 ; i<totalNumOfCusts; i++)
	{
		wait_semaphore(dependSem);
	}
	int sem1val = semaphore_count(shopCapacitySem);
	int sem2val = semaphore_count(dependSem);

	//wait a while to allow the slaves to finish printing their closing messages
	env_sleep(10000);
	if (sem2val == 0 && sem1val == shopCapacity)
		cprintf("\nCongratulations!! Test of Semaphores [2] completed successfully!!\n\n\n");
	else
		cprintf("\nError: wrong semaphore value... please review your semaphore code again...\n");

	return;
}
