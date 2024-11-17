#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

#define ACTUAL_START ((KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE))

//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int allocate_page(uint32 va , uint32 perm)
{
	uint32 *page_table = NULL;
	struct FrameInfo *frame_info = get_frame_info(ptr_page_directory, va, &page_table);

	// already allocated
	if (frame_info != NULL) {
		free_frame(frame_info);
	}

	int status = allocate_frame(&frame_info);
	if (status == E_NO_MEM) {
		return -1;
	}

	status = map_frame(ptr_page_directory, frame_info, va, perm);

	if (status == E_NO_MEM) {
		free_frame(frame_info);
		return -1;
	}
	return 0;
}

int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'24.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator
	if (daStart + initSizeToAllocate >= daLimit) {
		panic("kheap.c::initialize_kheap_dynamic_allocator(), initial size exceed the given limit");
	}

	kheap_start = daStart;
	kheap_break = daStart + initSizeToAllocate;
	kheap_limit = daLimit;

	LIST_INIT(&free_page_list);

    allocate_page(ACTUAL_START,PERM_PRESENT | PERM_USED | PERM_WRITEABLE);

    struct free_page_info *first_blk =  (struct free_page_info *) ACTUAL_START;

	first_blk->block_sz = (KERNEL_HEAP_MAX - ACTUAL_START) / PAGE_SIZE;

	first_blk->starting_address = ACTUAL_START;

	LIST_INSERT_HEAD(&free_page_list,first_blk);

	// allocate all pages in the given range
	for (uint32 va = kheap_start; va < kheap_break; va += PAGE_SIZE) {
		int status = allocate_page(va,PERM_PRESENT | PERM_USED | PERM_WRITEABLE);
		if (status == -1) {
			panic("kheap.c::initialize_kheap_dynamic_allocator(), no enough memory for a page");
		}
	}

	initialize_dynamic_allocator(daStart, initSizeToAllocate);
	return 0;
}

/*
	Description:
	Since virtual address space is mapped in quanta of pages (multiple of 4KB).
	sbrk always increase the size by multiple of pages
	If increment > 0: if within the hard limit
	move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	allocate pages and map them into the kernel virtual address space as necessary,
	returns the address of the previous break (i.e. the beginning of newly mapped memory).
	If increment = 0: just return the current position of the segment break
	if no memory OR break exceed the hard limit: it should return -1
*/
void* sbrk(int numOfPages)
{
	/* numOfPages > 0: move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	 * 				you should allocate pages and map them into the kernel virtual address space,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, return -1
	 */

	//MS2: COMMENT THIS LINE BEFORE START CODING==========
	// return (void*)-1 ;
	//====================================================

	//TODO: [PROJECT'24.MS2 - #02] [1] KERNEL HEAP - sbrk
	// Write your code here, remove the panic and write your code
	// panic("sbrk() is not implemented yet...!!");
	if(numOfPages == 0){
		return (void*)kheap_break;
	}
	uint32 new_added_size = numOfPages * PAGE_SIZE;
	uint32 new_break = kheap_break + new_added_size;
	if(new_break > kheap_limit || new_break < kheap_break){
		return (void*)-1;
	}
	uint32 start_page = kheap_break;
	uint32 end_page = ROUNDUP(new_break, PAGE_SIZE);
	for (uint32 va = start_page; va < end_page; va += PAGE_SIZE){
		struct FrameInfo *frame;
		allocate_frame(&frame);
		// allocate_page(va);
		int perm = (PERM_PRESENT | PERM_WRITEABLE);
		map_frame(ptr_page_directory, frame, va, perm);
		// free_frame(frame);
	}

	uint32 old_break = kheap_break;
	kheap_break = new_break;
	void *PE = (void*)(old_break - sizeof(uint32));
	// uint32 *end_block = (uint32*)(daStart + initSizeOfAllocatedSpace - sizeof(uint32));
	uint32 *end_block = (uint32*)(kheap_break - sizeof(uint32));
	*end_block = 1;

	return (void*)(old_break);
}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void split_page(struct free_page_info * blk, uint32 size)
{

	// |..........|..........|
	uint32 rem_sz = blk->block_sz - size;

	if (rem_sz == 0) {
		LIST_REMOVE(&free_page_list , blk);

		unmap_frame(ptr_page_directory , blk ->starting_address);

		return;
	}

	uint32 new_address = blk->starting_address + (size * PAGE_SIZE);

	allocate_page(new_address , PERM_PRESENT | PERM_USED | PERM_WRITEABLE);

	struct free_page_info *new_page = (struct free_page_info *)new_address;

	new_page->block_sz = rem_sz;

	new_page->starting_address = new_address;

	LIST_INSERT_AFTER(&free_page_list , blk , new_page);

    LIST_REMOVE(&free_page_list , blk);

	unmap_frame(ptr_page_directory , blk ->starting_address);
}

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'24.MS2 - #03] [1] KERNEL HEAP - kmalloc
	// Write your code here, remove the panic and write your code
	// kpanic_into_prompt("kmalloc() is not implemented yet...!!");
    acquire_spinlock(&MemFrameLists.mfllock);
	// allocate by first-fit case
    if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
		void *ptr = alloc_block_FF(size);

		release_spinlock(&MemFrameLists.mfllock);

		return ptr;
	}

	struct free_page_info * blk_start_ptr = NULL;

	// convert given size from bytes to pages
	uint32 blk_size_needed = ROUNDUP(size , PAGE_SIZE) / PAGE_SIZE;

	struct free_page_info *cur_blk = NULL;

	if (LIST_SIZE(&MemFrameLists.free_frame_list) < blk_size_needed) {
		release_spinlock(&MemFrameLists.mfllock);

      	return NULL;
	}

	// cprintf("PLEASE \n");

	// struct free_page_info *tmp = LIST_HEAD(&free_page_list);
	// cprintf()

	LIST_FOREACH(cur_blk , &free_page_list) {
		if (cur_blk->block_sz >= blk_size_needed ) {
          blk_start_ptr = cur_blk;

		  break;
		}
	}

	if (blk_start_ptr == NULL) {
		release_spinlock(&MemFrameLists.mfllock);

		return NULL;
	}

    uint32 cur_page = blk_start_ptr->starting_address;

    uint32 start_page = blk_start_ptr->starting_address;

	split_page(blk_start_ptr , blk_size_needed);

	// allocate pages from segment start.
	allocate_page(cur_page,PERM_PRESENT | PERM_USED | PERM_WRITEABLE);

	cur_page += PAGE_SIZE;

	blk_size_needed--;

	while (blk_size_needed--) {
		allocate_page(cur_page,PERM_PRESENT | PERM_USED | PERM_WRITEABLE);

		cur_page += PAGE_SIZE;
	}

	release_spinlock(&MemFrameLists.mfllock);

	return (void *)(start_page);

	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}
//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, if moved to another loc: the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}
