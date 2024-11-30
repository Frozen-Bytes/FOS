#include <inc/lib.h>

static struct UheapPageInfo uheap_pages_info[NUM_OF_UHEAP_PAGE_ALLOCATOR_PAGES];

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/

void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #12] [3] USER HEAP [USER SIDE] - malloc()
	// Write your code here, remove the panic and write your code
	// panic("malloc() is not implemented yet...!!");
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy

	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
		return alloc_block_FF(size);
	}

	// Page Allocator will be used
	uint32 required_pages = ROUNDUP(size , PAGE_SIZE) / PAGE_SIZE;

	uint32 cnt = 0, first_page = -1, found = 0;
	for (int i = 0 ; i < NUM_OF_UHEAP_PAGE_ALLOCATOR_PAGES ;) {
		if (!uheap_pages_info[i].taken) {
			cnt++;
			if (first_page == -1) {
				first_page = i;
			}
			i++;
		} else {
			cnt = 0;
			first_page = -1;

			// skip allocated pages
			uint32 page_count = ROUNDUP(uheap_pages_info[i].size, PAGE_SIZE) / PAGE_SIZE;
			i += page_count;
		}

		if (cnt == required_pages) {
			found = 1;
			break;
		}
	}

	// no valid space
	if (!found) {
		return NULL;
	}

	uheap_pages_info[first_page].size = size;
	uheap_pages_info[first_page].taken = 1;

	// get the virtual size
	void* va = (void*)(UHEAP_PAGE_ALLOCATOR_START + (PAGE_SIZE * first_page));

	// invoke system call
	sys_allocate_user_mem((uint32)va, size);
	return va;
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #14] [3] USER HEAP [USER SIDE] - free()
	// Write your code here, remove the panic and write your code
	// panic("free() is not implemented yet...!!");
	if (!virtual_address) {
		return;
	}

	uint32 hard_limit = myEnv->uheap_limit;

	bool inside_block_allocator = ((uint32)virtual_address >= USER_HEAP_START) &&
								  ((uint32)virtual_address < hard_limit);

	bool insid_page_allocator = ((uint32)virtual_address >= hard_limit + PAGE_SIZE) &&
								((uint32)virtual_address < USER_HEAP_MAX);

	if (inside_block_allocator) {
		free_block(virtual_address);
		return;
	} else if(insid_page_allocator) {

		virtual_address = ROUNDDOWN(virtual_address, PAGE_SIZE);

		uint32 page_idx = (ROUNDUP((uint32)virtual_address - UHEAP_PAGE_ALLOCATOR_START, PAGE_SIZE)) / PAGE_SIZE;

		uint32 allocated_size = uheap_pages_info[page_idx].size;
		uint32 page_count = ROUNDUP(allocated_size, PAGE_SIZE) / PAGE_SIZE;

		uheap_pages_info[page_idx].size = 0;
		uheap_pages_info[page_idx].taken = 0;

		sys_free_user_mem((uint32)virtual_address, allocated_size);

	} else {
		panic("uheap.c::free(), attempt to free invalid address %p\n", virtual_address);
	}

}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #18] [4] SHARED MEMORY [USER SIDE] - smalloc()
	// Write your code here, remove the panic and write your code
    void * va = malloc(ROUNDUP(size,PAGE_SIZE));

    if(va == NULL) {
	    return NULL;
	}

	int32 id = sys_createSharedObject(sharedVarName, size, isWritable, va);
	if(id < 0) {
		free(va);
	    return NULL;
	}
	uint32 page_idx = (ROUNDUP((uint32)va - UHEAP_PAGE_ALLOCATOR_START, PAGE_SIZE)) / PAGE_SIZE;
	uheap_pages_info[page_idx].shared_obj_id = id;

	return va;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
/*


Get the size of the shared variable (use sys_getSizeOfSharedObject())
If not exists, return NULL
Apply FIRST FIT strategy to search the heap for suitable space (on 4 KB BOUNDARY)
if no suitable space found, return NULL
Call sys_getSharedObject(...) to invoke the Kernel for sharing this variable
RETURN:
If successful, return its virtual address
Else, return NULL
Testing:
FOS> run tshr2 3000 	 smalloc & sget
FOS> run tff3 3000 		 First Fit (smalloc, sget, malloc & free)



*/
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//TODO: [PROJECT'24.MS2 - #20] [4] SHARED MEMORY [USER SIDE] - sget()
	// Write your code here, remove the panic and write your code
	// panic("sget() is not implemented yet...!!");
	uint32 shared_obj_size = sys_getSizeOfSharedObject(ownerEnvID, sharedVarName);
	if(shared_obj_size == E_SHARED_MEM_NOT_EXISTS){
		return NULL;
	}
	void * va = malloc(ROUNDUP(shared_obj_size,PAGE_SIZE));
    if(!va) {
	    return NULL;
	}
	int id = sys_getSharedObject(ownerEnvID, sharedVarName, va);
	if(id == E_SHARED_MEM_NOT_EXISTS){
		free(va);
		return NULL;
	}
	uint32 page_idx = (ROUNDUP((uint32)va - UHEAP_PAGE_ALLOCATOR_START, PAGE_SIZE)) / PAGE_SIZE;
	uheap_pages_info[page_idx].shared_obj_id = id;

	return va;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [USER SIDE] - sfree()
	// Write your code here, remove the panic and write your code
	// panic("sfree() is not implemented yet...!!");

	if (!virtual_address) {
		return;
	}

	uint32 page_idx = (ROUNDUP((uint32) virtual_address - UHEAP_PAGE_ALLOCATOR_START, PAGE_SIZE)) / PAGE_SIZE;
	uint32 id = uheap_pages_info[page_idx].shared_obj_id;

	free(virtual_address);
	sys_freeSharedObject(id, virtual_address);
}


//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
