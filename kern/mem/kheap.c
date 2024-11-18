#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "inc/mmu.h"
#include "memory_manager.h"

#define PAGE_ALLOCATOR_START ((KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE))
#define PTE_KERN (PERM_PRESENT | PERM_USED | PERM_WRITEABLE)
#define NUM_OF_KHEAP_PAGE_ALLOCATOR_PAGES ((KERNEL_HEAP_MAX - PAGE_ALLOCATOR_START) / PAGE_SIZE)

static struct HeapBlock heap_blocks[NUM_OF_KHEAP_PAGE_ALLOCATOR_PAGES];

struct HeapBlock* to_heap_block(uint32 va);

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

	// Reallocating a page is illegal (i.e. very bad)
	if (frame_info != NULL) {
		panic("allocate_page(): trying to allocate an already allocated page (va: %x)", va);
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

    struct HeapBlock *first_blk =  heap_blocks;
	first_blk->page_count = NUM_OF_KHEAP_PAGE_ALLOCATOR_PAGES;
	first_blk->start_va = PAGE_ALLOCATOR_START;

	LIST_INIT(&free_blocks_list);
	LIST_INSERT_HEAD(&free_blocks_list, first_blk);

	// allocate all pages in the given range
	for (uint32 va = kheap_start; va < kheap_break; va += PAGE_SIZE) {
		int status = allocate_page(va, PTE_KERN);
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
		int perm = (PERM_PRESENT | PERM_WRITEABLE);
		map_frame(ptr_page_directory, frame, va, perm);
	}

	uint32 old_break = kheap_break;
	kheap_break = new_break;
	// update the END BLOCK
	uint32 *end_block = (uint32*)(kheap_break - sizeof(uint32));
	*end_block = 1;

	return (void*)(old_break);
}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void split_page(struct HeapBlock * blk, uint32 size)
{

	// |..........|..........|
	uint32 rem_sz = blk->page_count - size;

	if (rem_sz == 0) {
		LIST_REMOVE(&free_blocks_list , blk);

		unmap_frame(ptr_page_directory , blk ->start_va);

		return;
	}

	uint32 new_address = blk->start_va + (size * PAGE_SIZE);

	allocate_page(new_address , PTE_KERN);

	struct HeapBlock *new_page = (struct HeapBlock *)new_address;

	new_page->page_count = rem_sz;

	new_page->start_va = new_address;

	LIST_INSERT_AFTER(&free_blocks_list , blk , new_page);

    LIST_REMOVE(&free_blocks_list , blk);

	unmap_frame(ptr_page_directory , blk ->start_va);
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

	struct HeapBlock * blk_start_ptr = NULL;

	// convert given size from bytes to pages
	uint32 blk_size_needed = ROUNDUP(size , PAGE_SIZE) / PAGE_SIZE;

	struct HeapBlock *cur_blk = NULL;

	if (LIST_SIZE(&MemFrameLists.free_frame_list) < blk_size_needed) {
		release_spinlock(&MemFrameLists.mfllock);

      	return NULL;
	}

	// cprintf("PLEASE \n");

	// struct free_page_info *tmp = LIST_HEAD(&free_page_list);
	// cprintf()

	LIST_FOREACH(cur_blk , &free_blocks_list) {
		if (cur_blk->page_count >= blk_size_needed ) {
          blk_start_ptr = cur_blk;

		  break;
		}
	}

	if (blk_start_ptr == NULL) {
		release_spinlock(&MemFrameLists.mfllock);

		return NULL;
	}

    uint32 cur_page = blk_start_ptr->start_va;

    uint32 start_page = blk_start_ptr->start_va;

	split_page(blk_start_ptr , blk_size_needed);

	// allocate pages from segment start.
	allocate_page(cur_page, PTE_KERN);

	cur_page += PAGE_SIZE;

	blk_size_needed--;

	while (blk_size_needed--) {
		allocate_page(cur_page, PTE_KERN);

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
	//return the physical address corresponding to given virtual_address
	//physical address including offset (not only the frame physical address)
	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	struct FrameInfo *corresponding_frame;
	// Not needed but get_frame_info function requires an output parameter
	uint32 *ptr_page_table;

	corresponding_frame = get_frame_info(ptr_page_directory, virtual_address, &ptr_page_table);

	// if virtual address doesn't map to anything
	if(corresponding_frame == NULL){
		return 0;
	}

	uint32 frame_physical_address = to_physical_address(corresponding_frame);
	// Frame physical address + offset
	uint32 kheap_physical_address = frame_physical_address + PGOFF(virtual_address);

	return kheap_physical_address;
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	//return the virtual address corresponding to given physical_address
	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	struct FrameInfo *corresponding_frame = to_frame_info(physical_address);
	uint32 mapped_page_virtual_address = corresponding_frame->mapped_page_virtual_address;

	// In the initialize_frame_info function used while initializing paging,
	// all values in the FrameInfo struct are initialized by 0
	// so if the virtual address is still 0, then this frame hasn't been mapped
	// and a kernel heap virtual address can never equal 0,
	// since kernel heap occupies part of the top 256 MBs of the virtual memory.
	if(mapped_page_virtual_address == 0){
		return 0;
	} else {
		uint32 offset = physical_address % PAGE_SIZE;
		return (mapped_page_virtual_address << 12) | offset;
	}
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

struct HeapBlock*
to_heap_block(uint32 va) {
	assert(va && (va >= PAGE_ALLOCATOR_START));
	uint32 offset = (va - PAGE_ALLOCATOR_START) / PAGE_SIZE;
	return heap_blocks + offset;
}
