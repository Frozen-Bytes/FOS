// Test the free of shared variables
#include <inc/lib.h>

extern volatile bool printStats;
void
_main(void)
{
	printStats = 0;
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN KERNEL DYNAMIC ALLOCATOR DUE
	 * TO sbrk()
	 *********************************************************/

	/*=================================================*/
	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
#if USE_KHEAP
	{
		if (LIST_SIZE(&(myEnv->page_WS_list)) >= myEnv->page_WS_max_size)
			panic("Please increase the WS size");
	}
#else
	panic("make sure to enable the kernel heap: USE_KHEAP=1");
#endif
	/*=================================================*/

	uint32 pagealloc_start = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE; //UHS + 32MB + 4KB
	uint32 *x, *y, *z ;
	int freeFrames, diff, expected;

	cprintf("************************************************\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("************************************************\n\n\n");

	int envID = sys_getenvid();

	cprintf("STEP A: checking free of shared object using 2 environments... \n");
	{
		uint32 *x;
		int32 envIdSlave1 = sys_create_env("tshr5slave", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
		int32 envIdSlave2 = sys_create_env("tshr5slave", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));

		int freeFrames = sys_calculate_free_frames() ;

		x = smalloc("x", PAGE_SIZE, 1);

		cprintf("Master env created x (1 page) \n");
		if (x != (uint32*)pagealloc_start) panic("Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");
		expected = 1+1 ; /*1page +1table*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff < expected || diff > expected +1+1 /*extra 1 page & 1 table for sbrk (at max)*/)
			panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);

		//to check that the slave environments completed successfully
		rsttst();

		sys_run_env(envIdSlave1);
		sys_run_env(envIdSlave2);

		cprintf("please be patient ...\n");
		env_sleep(3000);

		//to ensure that the slave environments completed successfully
		while (gettst()!=2) ;// panic("test failed");

		freeFrames = sys_calculate_free_frames() ;

		sfree(x);

		cprintf("Master env removed x (1 page) \n");
		int diff2 = (sys_calculate_free_frames() - freeFrames);
		expected = 1+1; /*1page+1table*/
		if (diff != expected) panic("Wrong free (diff=%d, expected=%d): revise your freeSharedObject logic\n", diff, expected);
	}
	cprintf("Step A completed successfully!!\n\n\n");

	cprintf("STEP B: checking free of 2 shared objects ... \n");
	{
		uint32 *x, *z ;
		int32 envIdSlaveB1 = sys_create_env("tshr5slaveB1", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
		int32 envIdSlaveB2 = sys_create_env("tshr5slaveB2", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));

		z = smalloc("z", PAGE_SIZE+1, 1);
		cprintf("Master env created z (2 pages) \n");

		x = smalloc("x", PAGE_SIZE+1024, 1);
		cprintf("Master env created x (2 pages) \n");

		rsttst();

		sys_run_env(envIdSlaveB1);
		sys_run_env(envIdSlaveB2);

		//give slaves time to catch the shared object before removal
		{
//			env_sleep(4000);
			while (gettst()!=2) ;
		}

//		rsttst();

		int freeFrames = sys_calculate_free_frames() ;

		sfree(z);
		cprintf("Master env removed z\n");

		sfree(x);
		cprintf("Master env removed x\n");

		int diff = (sys_calculate_free_frames() - freeFrames);
		expected = 1 /*table*/;
		if (diff !=  expected) panic("Wrong free: frames removed not equal 1 !, correct frames to be removed are 1:\nfrom the env: 1 table\nframes_storage of z & x: should NOT cleared yet (still in use!)\n");

		//To indicate that it's completed successfully
		cprintf("Master is completed.\n");
		inctst();
	}


	return;
}
