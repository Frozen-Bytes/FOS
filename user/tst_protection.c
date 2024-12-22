#include <inc/lib.h>

void
_main(void)
{
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

	int numOfSlaves = 3;
	rsttst();
	//[1] Run programs that allocate many shared variables
	for (int i = 0; i < numOfSlaves; ++i) {
		int32 envId = sys_create_env("protection_slave1", (myEnv->page_WS_max_size),(myEnv->SecondListSize), (myEnv->percentage_of_WS_pages_to_be_removed));
		sys_run_env(envId);
	}

	while (gettst() != numOfSlaves) ;

	cprintf("%~\nCongratulations... test protection is run successfully\n");

}
