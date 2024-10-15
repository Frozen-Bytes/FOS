// Test the creation of shared variables and using them
// Slave program1: Read the 2 shared variables, edit the 3rd one, and exit
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

	uint32 *x,*y,*z, *expectedVA;
	int freeFrames, diff, expected;
	int32 parentenvID = sys_getparentenvid();
	//GET: z then y then x, opposite to creation order (x then y then z)
	//So, addresses here will be different from the OWNER addresses
	//sys_lock_cons();
	sys_lock_cons();
	{
		freeFrames = sys_calculate_free_frames() ;
		z = sget(parentenvID,"z");
		expectedVA = (uint32*)(pagealloc_start + 0 * PAGE_SIZE);
		if (z != expectedVA) panic("Get(): Returned address is not correct. Expected = %x, Actual = %x\nMake sure that you align the allocation on 4KB boundary", expectedVA, z);
		expected = 1 ; /*1table*/
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff != expected) panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);
	}
	sys_unlock_cons();
	//sys_unlock_cons();

	//sys_lock_cons();
	sys_lock_cons();
	{
		freeFrames = sys_calculate_free_frames() ;
		y = sget(parentenvID,"y");
		expectedVA = (uint32*)(pagealloc_start + 1 * PAGE_SIZE);
		if (y != expectedVA) panic("Get(): Returned address is not correct. Expected = %x, Actual = %x\nMake sure that you align the allocation on 4KB boundary", expectedVA, y);
		expected = 0 ;
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff != expected) panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);
	}
	sys_unlock_cons();
	//sys_unlock_cons();

	if (*y != 20) panic("Get(): Shared Variable is not created or got correctly") ;

	//sys_lock_cons();
	sys_lock_cons();
	{
		freeFrames = sys_calculate_free_frames() ;
		x = sget(parentenvID,"x");
		expectedVA = (uint32*)(pagealloc_start + 2 * PAGE_SIZE);
		if (x != expectedVA) panic("Get(): Returned address is not correct. Expected = %x, Actual = %x\nMake sure that you align the allocation on 4KB boundary", expectedVA, x);
		expected = 0 ;
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff != expected) panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);
	}
	sys_unlock_cons();
	//sys_unlock_cons();

	if (*x != 10) panic("Get(): Shared Variable is not created or got correctly") ;

	*z = *x + *y ;
	if (*z != 30) panic("Get(): Shared Variable is not created or got correctly") ;

	//To indicate that it's completed successfully
	inctst();

	return;
}
