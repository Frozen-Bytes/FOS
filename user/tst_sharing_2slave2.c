// Test the creation of shared variables and using them
// Slave program2: Get 2 shared variables, edit the writable one, and attempt to edit the readOnly one
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


	int32 parentenvID = sys_getparentenvid();
	uint32 *x, *z, *expectedVA;
	int freeFrames, diff, expected;

	//GET: z then x, opposite to creation order (x then y then z)
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
		x = sget(parentenvID,"x");
		expectedVA = (uint32*)(pagealloc_start + 1 * PAGE_SIZE);
		if (x != expectedVA) panic("Get(): Returned address is not correct. Expected = %x, Actual = %x\nMake sure that you align the allocation on 4KB boundary", expectedVA, x);
		expected = 0;
		diff = (freeFrames - sys_calculate_free_frames());
		if (diff != expected) panic("Wrong allocation (current=%d, expected=%d): make sure that you allocate the required space in the user environment and add its frames to frames_storage", freeFrames - sys_calculate_free_frames(), expected);
	}
	sys_unlock_cons();
	//sys_unlock_cons();

	if (*x != 10) panic("Get(): Shared Variable is not created or got correctly") ;

	//Edit the writable object
	*z = 50;
	if (*z != 50) panic("Get(): Shared Variable is not created or got correctly") ;

	inctst();

	//sync with master
	while (gettst() != 5) ;

	//Attempt to edit the ReadOnly object, it should panic
	sys_bypassPageFault(6);
	cprintf("Attempt to edit the ReadOnly object @ va = %x\n", x);
	*x = 100;

	sys_bypassPageFault(0);

	inctst();
	if (*x == 100)
		panic("Test FAILED! it should not edit the variable x since it's a read-only!") ;
	return;
}
