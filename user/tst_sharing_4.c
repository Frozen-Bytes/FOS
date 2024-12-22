// Test the free of shared variables (create_shared_memory)
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

	int eval = 0;
	bool is_correct = 1;

	uint32 pagealloc_start = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE; //UHS + 32MB + 4KB
	uint32 *x, *y, *z ;
	int freeFrames, diff, expected;

	cprintf("************************************************\n");
	cprintf("MAKE SURE to have a FRESH RUN for this test\n(i.e. don't run any program/test before it)\n");
	cprintf("************************************************\n\n\n");

	int Mega = 1024*1024;
	int kilo = 1024;
	int envID = sys_getenvid();
	cprintf("STEP A: checking free of a shared object ... [25%]\n");
	{
		freeFrames = sys_calculate_free_frames() ;
		x = smalloc("x", PAGE_SIZE, 1);
		if (x != (uint32*)pagealloc_start)
		{is_correct = 0; cprintf("Returned address is not correct. check the setting of it and/or the updating of the shared_mem_free_address");}
		expected = 1+1 ; /*1page +1table*/

		/*extra 1 page & 1 table for kernel sbrk (at max) due to sharedObject & frameStorage*/
		/*extra 1 page & 1 table for user sbrk (at max) if creating special DS to manage USER PAGE ALLOC */
		int upperLimit = expected +1+1 +1+1 ;
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff < expected || diff > upperLimit)
			{is_correct = 0; cprintf("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}

		sfree(x);

		int diff2 = (freeFrames - sys_calculate_free_frames());
		if (diff2 !=  (diff - expected))
		{is_correct = 0; cprintf("Wrong free: revise your freeSharedObject logic. Expected = %d, Actual = %d", expected, (freeFrames - sys_calculate_free_frames()));}
	}
	cprintf("Step A completed!!\n\n\n");

	if (is_correct)	eval+=25;
	is_correct = 1;

	cprintf("STEP B: checking free of 2 shared objects ... [25%]\n");
	{
		uint32 *x, *z ;
		freeFrames = sys_calculate_free_frames() ;
		z = smalloc("z", PAGE_SIZE, 1);
		x = smalloc("x", PAGE_SIZE, 1);

		if(x == NULL)
		{is_correct = 0; cprintf("Wrong free: make sure that you free the shared object by calling free_share_object()");}

		expected = 2+1 ; /*2pages +1table*/
		/*extra 1 page for kernel sbrk (at max) due to sharedObject & frameStorage of the 2nd object "x"*/
		/*if creating special DS to manage USER PAGE ALLOC, the prev. created page from STEP A is sufficient */
		int upperLimit = expected +1 ;
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff < expected || diff > upperLimit)
			{is_correct = 0; cprintf("Wrong previous free: make sure that you correctly free shared object before (Step A)");}

		sfree(z);

		int diff2 = (freeFrames - sys_calculate_free_frames());
		if (diff2 != (diff - 1 /*1 page*/))
		{is_correct = 0; cprintf("Wrong free: revise your freeSharedObject logic. Expected = %d, Actual = %d", expected, (freeFrames - sys_calculate_free_frames()));}

		sfree(x);

		expected = 0;
		int diff3 = (freeFrames - sys_calculate_free_frames());
		if (diff3 != (diff2 - (1+1) /*1 page + 1 table*/))
		{is_correct = 0; cprintf("Wrong free: revise your freeSharedObject logic. Expected = %d, Actual = %d", expected, (freeFrames - sys_calculate_free_frames()));}

	}
	cprintf("Step B completed!!\n\n\n");

	if (is_correct)	eval+=25;
	is_correct = 1;

	cprintf("STEP C: checking range of loop during free... [50%]\n");
	{
		uint32 *w, *u;
		int freeFrames = sys_calculate_free_frames() ;
		w = smalloc("w", 3 * PAGE_SIZE+1, 1);
		u = smalloc("u", PAGE_SIZE, 1);
		expected = 5+1 ; /*5pages +1table*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff != expected)
			{is_correct = 0; cprintf("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}

		sfree(w);

		expected = 1+1 ; /*1page +1table*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff != expected) {is_correct = 0; cprintf("Wrong free: revise your freeSharedObject logic. Expected = %d, Actual = %d", expected, (freeFrames - sys_calculate_free_frames()));}

		uint32 *o;

		o = smalloc("o", 2 * PAGE_SIZE-1,1);

		expected = 3+1 ; /*3pages +1table*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff != expected /*Exact! since it's not expected that to invloke sbrk due to the prev. sfree*/)
			{is_correct = 0; cprintf("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}

		sfree(o);

		expected = 1+1 ; /*1page +1table*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff != expected) {is_correct = 0; cprintf("Wrong free: revise your freeSharedObject logic. Expected = %d, Actual = %d", expected, (freeFrames - sys_calculate_free_frames()));}

		sfree(u);

		expected = 0;
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff != expected) {is_correct = 0; cprintf("Wrong free: revise your freeSharedObject logic. Expected = %d, Actual = %d", expected, (freeFrames - sys_calculate_free_frames()));}

		if (is_correct)	eval+=25;
		is_correct = 1;

		//Checking boundaries of page tables
		freeFrames = sys_calculate_free_frames() ;
		w = smalloc("w", 3 * Mega - 1*kilo, 1);
		u = smalloc("u", 7 * Mega - 1*kilo, 1);
		o = smalloc("o", 2 * Mega + 1*kilo, 1);

		expected = 3073+4+4 ; /*3073pages +4tables +4pages for framesStorage by Kernel Page Allocator since it exceed 2KB size*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff < expected || diff > expected +1+1 /*extra 1 page & 1 table for sbrk (at max)*/)
			{is_correct = 0; cprintf("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);}

		freeFrames = sys_calculate_free_frames() ;

		sfree(o);
//		if ((freeFrames - sys_calculate_free_frames()) !=  2560+3+5) {is_correct = 0; cprintf("Wrong free: check your logic");

		sfree(w);
//		if ((freeFrames - sys_calculate_free_frames()) !=  1792+3+3) {is_correct = 0; cprintf("Wrong free: check your logic");

		sfree(u);

		expected = 3073+4+4;
		diff = (sys_calculate_free_frames() - freeFrames);
		if (diff != expected) {is_correct = 0; cprintf("Wrong free: revise your freeSharedObject logic. Expected = %d, Actual = %d", expected, (freeFrames - sys_calculate_free_frames()));}
	}
	cprintf("Step C completed!!\n\n\n");
	if (is_correct)	eval+=25;
	is_correct = 1;

	cprintf("\n%~Test of freeSharedObjects [4] completed. Eval = %d%%\n\n", eval);

	return;
}
