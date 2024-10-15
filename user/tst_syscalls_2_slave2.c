// Test the correct validation of system call params
#include <inc/lib.h>

void
_main(void)
{
	//[2] Invalid Range (outside User Heap)
	sys_allocate_user_mem(USER_HEAP_MAX, 10);
	inctst();
	panic("tst system calls #2 failed: sys_allocate_user_mem is called with invalid params\nThe env must be killed and shouldn't return here.");

	sys_free_user_mem(USER_HEAP_MAX, 10);
	inctst();
	panic("tst system calls #2 failed: sys_free_user_mem is called with invalid params\nThe env must be killed and shouldn't return here.");
}
