// Test the free of shared variables
#include <inc/lib.h>

void
_main(void)
{
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

	x = sget(sys_getparentenvid(),"x");
	cprintf("Slave B1 env used x (getSharedObject)\n");
	//To indicate that it's successfully got x
	inctst();
	cprintf("Slave B1 please be patient ...\n");

	//sleep a while to allow the master to remove x & z
	env_sleep(6000);
	while (gettst()!=2) ;// panic("test failed");

	freeFrames = sys_calculate_free_frames() ;

	sfree(x);
	cprintf("Slave B1 env removed x\n");
	expected = 1+1; /*1page+1table*/
	if ((sys_calculate_free_frames() - freeFrames) !=  expected) panic("B1 wrong free: frames removed not equal 4 !, correct frames to be removed are 4:\nfrom the env: 1 table and 1 for frame of x\nframes_storage of x: should be cleared now\n");

	//To indicate that it's completed successfully
	inctst();
	return;
}

