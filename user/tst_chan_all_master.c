// Test the Sleep and wakeup on a channel
// Master program: create and run slaves, wait them to finish
#include <inc/lib.h>

void
_main(void)
{
	int envID = sys_getenvid();
	char slavesCnt[10];
	readline("Enter the number of Slave Programs: ", slavesCnt);
	int numOfSlaves = strtol(slavesCnt, NULL, 10);

	//Create and run slave programs that should sleep
	int id;
	for (int i = 0; i < numOfSlaves; ++i)
	{
		id = sys_create_env("tstChanAllSlave", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
		if (id== E_ENV_CREATION_ERROR)
		{
			cprintf("\n%~insufficient number of processes in the system! only %d slave processes are created\n", i);
			numOfSlaves = i;
			break;
		}
		sys_run_env(id);
	}

	//Wait until all slaves are blocked
	int numOfBlockedProcesses = 0;
	sys_utilities("__GetChanQueueSize__", (uint32)(&numOfBlockedProcesses));
	int cnt = 0;
	while (numOfBlockedProcesses != numOfSlaves)
	{
		env_sleep(1000);
		if (cnt == numOfSlaves)
		{
			panic("%~test channels failed! unexpected number of blocked slaves. Expected = %d, Current = %d", numOfSlaves, numOfBlockedProcesses);
		}
		sys_utilities("__GetChanQueueSize__", (uint32)(&numOfBlockedProcesses));
		cnt++ ;
	}

	rsttst();

	//Wakeup all
	sys_utilities("__WakeupAll__", 0);

	//Wait until all slave finished
	cnt = 0;
	while (gettst() != numOfSlaves)
	{
		env_sleep(1000);
		if (cnt == numOfSlaves)
		{
			panic("%~test channels failed! not all slaves finished");
		}
		cnt++ ;
	}

	cprintf("%~\n\nCongratulations!! Test of Channel (sleep & wakeup ALL) completed successfully!!\n\n");

	return;
}
