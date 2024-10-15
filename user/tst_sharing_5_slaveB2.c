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

	z = sget(sys_getparentenvid(),"z");
	cprintf("Slave B2 env used z (getSharedObject)\n");
	//To indicate that it's successfully got z
	inctst();

	cprintf("Slave B2 please be patient ...\n");

	//sleep a while to allow the master to remove x & z
	env_sleep(9000);
	//to ensure that the other environments completed successfully
	while (gettst()!=3) ;// panic("test failed");

	freeFrames = sys_calculate_free_frames() ;

	sfree(z);
	cprintf("Slave B2 env removed z\n");
	expected = 1+1; /*1page+1table*/
	if ((sys_calculate_free_frames() - freeFrames) !=  expected) panic("wrong free: frames removed not equal 4 !, correct frames to be removed are 4:\nfrom the env: 1 table + 1 frame for z\nframes_storage of z: should be cleared now\n");


	cprintf("Step B completed successfully!!\n\n\n");
	cprintf("\n%~Congratulations!! Test of freeSharedObjects [5] completed successfully!!\n\n\n");

	return;
}
