// Test the SPECIAL CASES during the creation & get of shared variables
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

	cprintf("************************************************\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("************************************************\n\n\n");

	uint32 pagealloc_start = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE; //UHS + 32MB + 4KB

	uint32 *x, *y, *z ;
	cprintf("STEP A: checking creation of shared object that is already exists... \n\n");
	{
		int ret ;
		//int ret = sys_createSharedObject("x", PAGE_SIZE, 1, (void*)&x);
		x = smalloc("x", PAGE_SIZE, 1);
		int freeFrames = sys_calculate_free_frames() ;
		x = smalloc("x", PAGE_SIZE, 1);
		if (x != NULL) panic("Trying to create an already exists object and corresponding error is not returned!!");
		if ((freeFrames - sys_calculate_free_frames()) !=  0) panic("Wrong allocation: make sure that you don't allocate any memory if the shared object exists");
	}

	cprintf("STEP B: checking getting shared object that is NOT exists... \n\n");
	{
		int ret ;
		x = sget(myEnv->env_id, "xx");
		int freeFrames = sys_calculate_free_frames() ;
		if (x != NULL) panic("Trying to get a NON existing object and corresponding error is not returned!!");
		if ((freeFrames - sys_calculate_free_frames()) !=  0) panic("Wrong get: make sure that you don't allocate any memory if the shared object not exists");
	}

	cprintf("STEP C: checking the creation of shared object that exceeds the SHARED area limit... \n\n");
	{
		int freeFrames = sys_calculate_free_frames() ;
		uint32 size = USER_HEAP_MAX - pagealloc_start - PAGE_SIZE + 1;
		y = smalloc("y", size, 1);
		if (y != NULL) panic("Trying to create a shared object that exceed the SHARED area limit and the corresponding error is not returned!!");
		if ((freeFrames - sys_calculate_free_frames()) !=  0) panic("Wrong allocation: make sure that you don't allocate any memory if the shared object exceed the SHARED area limit");
	}

	cprintf("\n%~Congratulations!! Test of Shared Variables [Create & Get: Special Cases] completed successfully!!\n\n\n");

}