// Test the free of shared variables
#include <inc/lib.h>

extern volatile bool printStats;
void
_main(void)
{
	printStats = 0;
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
sys_lock_cons();
	x = sget(sys_getparentenvid(),"x");

	freeFrames = sys_calculate_free_frames() ;

	cprintf("Slave env used x (getSharedObject)\n");

	sfree(x);

	cprintf("Slave env removed x\n");

	diff = (sys_calculate_free_frames() - freeFrames);
sys_unlock_cons();
	expected = 1;
	if (diff != expected) panic("wrong free: frames removed not equal 1 !, correct frames to be removed is 1:\nfrom the env: 1 table for x\nframes_storage: not cleared yet\n");

	//to ensure that this environment is completed successfully
	inctst();

	return;
}
