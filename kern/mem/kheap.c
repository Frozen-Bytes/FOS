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
struct HeapBlock* split_heap_block(struct HeapBlock* blk, uint32 required_pages);
void insert_heap_block_sorted(struct HeapBlock* b);
void coalescing_heap_block(struct HeapBlock* b);

//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int allocate_page(uint32 va , uint32 perm, bool force)
{
	uint32 *page_table = NULL;
	struct FrameInfo *frame_info = get_frame_info(ptr_page_directory, va, &page_table);

	// Reallocating a page is illegal (i.e. very bad)
	if (frame_info != NULL) {
		if (!force) {
			panic("allocate_page(): trying to allocate an already allocated page (va: %x)", va);
		}

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

    struct HeapBlock *first_blk =  heap_blocks;
	first_blk->page_count = NUM_OF_KHEAP_PAGE_ALLOCATOR_PAGES;
	first_blk->start_va = PAGE_ALLOCATOR_START;

	LIST_INIT(&free_blocks_list);
	LIST_INSERT_HEAD(&free_blocks_list, first_blk);

	// allocate all pages in the given range
	for (uint32 va = kheap_start; va < kheap_break; va += PAGE_SIZE) {
		int status = allocate_page(va, PTE_KERN, 1);
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

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'24.MS2 - #03] [1] KERNEL HEAP - kmalloc
	// Write your code here, remove the panic and write your code
	// kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

    if (size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
		return alloc_block_FF(size);
	}

	bool is_holding_lock = holding_spinlock(&MemFrameLists.mfllock);

	if (!is_holding_lock) {
		acquire_spinlock(&MemFrameLists.mfllock);
	}

	// Convert given size from bytes to pages
	uint32 required_pages = ROUNDUP(size , PAGE_SIZE) / PAGE_SIZE;
	struct HeapBlock* blk = NULL;

	LIST_FOREACH(blk , &free_blocks_list) {
		if (blk->page_count >= required_pages) { break; }
	}

	if (!blk) {
		goto error_return;
	}

	int status = 0;
	uint32 va = blk->start_va;
	for (uint32 i = 0; i < required_pages; i++, va += PAGE_SIZE) {
		status = allocate_page(va, PTE_KERN, 0);
		if (status != 0) {
			break;
		}
	}

	// Allocation failed
	if (status != 0) {
		// Release allocated frames
		for (uint32 allocated_va = blk->start_va; allocated_va < va; allocated_va += PAGE_SIZE) {
			unmap_frame(ptr_page_directory, allocated_va);
		}

		goto error_return;
	}

	struct HeapBlock* new_blk = split_heap_block(blk , required_pages);
	if (new_blk) {
		LIST_INSERT_AFTER(&free_blocks_list, blk, new_blk);
	}
	LIST_REMOVE(&free_blocks_list, blk);

	if (!is_holding_lock) {
		release_spinlock(&MemFrameLists.mfllock);
	}

	return (void*)(blk->start_va);

error_return:
	if (!is_holding_lock) {
		release_spinlock(&MemFrameLists.mfllock);
	}
	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	// panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

	if (!virtual_address) {
		return;
	}

	void* blk_allocator_brk = sbrk(0);
	bool is_blk_addr = ((uint32) virtual_address >= kheap_start) &&
	                   (virtual_address < blk_allocator_brk);
	bool is_page_addr = ((uint32) virtual_address >= kheap_limit + PAGE_SIZE) &&
				        ((uint32) virtual_address < KERNEL_HEAP_MAX);

	if (is_blk_addr) {
		free_block(virtual_address);
	} else if (is_page_addr) {
		acquire_spinlock(&MemFrameLists.mfllock);

		// Not necessary, but makes sure that VA always starts on a page boundary
		virtual_address = ROUNDDOWN(virtual_address, PAGE_SIZE);
		struct HeapBlock* blk = to_heap_block((uint32) virtual_address);
		uint32 alloc_sz = blk->page_count * PAGE_SIZE;

		for (void* va = virtual_address; va < virtual_address + alloc_sz; va += PAGE_SIZE) {
			uint32 pa = kheap_physical_address((uint32) va);
			struct FrameInfo* frame_info = to_frame_info(pa);

			if (!frame_info) {
				panic("kfree(): trying to free an unallocated frame '%x' (frame #%d)",
				      frame_info,
				      to_frame_number(frame_info));
			}

			// Should invalidate cache
			unmap_frame(ptr_page_directory, (uint32) va);
		}

		insert_heap_block_sorted(blk);
		coalescing_heap_block(blk);

		release_spinlock(&MemFrameLists.mfllock);
	} else {
		panic("kfree(): out of bounds VA '%x'", virtual_address);
	}
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

/*
	kmalloc_with_krealloc description:
	this function use kalloc technique to realloc new place but with transfer
    old data and handle case that there is enough frames to realloc but not
    to allocate first then realloc the new place.
*/
void *
kmalloc_with_krealloc(uint32 virtual_address, uint32 size){
	uint32 old_size = 0;
	bool is_page_addr = ((uint32) virtual_address >= kheap_limit + PAGE_SIZE) &&
				        ((uint32) virtual_address < KERNEL_HEAP_MAX);
						
   // get the size of block
	if(is_page_addr) {
		struct HeapBlock* blk = to_heap_block((uint32) virtual_address);
		old_size = blk->page_count;
	} else {
		panic("kmalloc_with_krealloc(): out of bounds VA '%x'", virtual_address);
		return NULL;
	}

	bool is_holding_lock = holding_spinlock(&MemFrameLists.mfllock);

	if (!is_holding_lock) {
		acquire_spinlock(&MemFrameLists.mfllock);
	}

	// Convert given size from bytes to pages
	uint32 required_pages = ROUNDUP(size , PAGE_SIZE) / PAGE_SIZE;
	struct HeapBlock* blk = NULL;

	LIST_FOREACH(blk , &free_blocks_list) {
		if (blk->page_count >= required_pages) { break; }
	}

	if (!blk) {
		goto error_return;
	}

	int status = 0;
	uint32 va = blk->start_va;
	for (uint32 i = 0; i < required_pages; i++, va += PAGE_SIZE,virtual_address += PAGE_SIZE) {
		if(i < old_size){
			uint32 *page_table = NULL;
			uint32 perm = pt_get_page_permissions(ptr_page_directory, virtual_address);
	        struct FrameInfo *frame_info = get_frame_info(ptr_page_directory, va, &page_table);
			if(frame_info == NULL){
				panic("kmalloc_with_krealloc(): not allocated VA '%x'", virtual_address);
			}
			map_frame(ptr_page_directory, frame_info, va, perm);
			continue;
		}
		status = allocate_page(va, PTE_KERN, 0);
		if (status != 0) {
			break;
		}
	}

	// Allocation failed
	if (status != 0) {
		// Release allocated frames
		for (uint32 allocated_va = blk->start_va; allocated_va < va; allocated_va += PAGE_SIZE) {
			unmap_frame(ptr_page_directory, allocated_va);
		}

		goto error_return;
	}

	struct HeapBlock* new_blk = split_heap_block(blk , required_pages);
	if (new_blk) {
		LIST_INSERT_AFTER(&free_blocks_list, blk, new_blk);
	}
	LIST_REMOVE(&free_blocks_list, blk);

	if (!is_holding_lock) {
		release_spinlock(&MemFrameLists.mfllock);
	}

	return (void*)(blk->start_va);

error_return:
	if (!is_holding_lock) {
		release_spinlock(&MemFrameLists.mfllock);
	}
	return NULL;
}

void*
kexpand_block(uint32 va, uint32 new_size)
{
	uint32 old_size = 0;
    bool is_page_addr = ((uint32) va >= kheap_limit + PAGE_SIZE) &&
				        ((uint32) va < KERNEL_HEAP_MAX);
						
   // get the size of block
	if(is_page_addr) {
		struct HeapBlock* blk = to_heap_block((uint32) va);
		old_size = blk->page_count * PAGE_SIZE;
	} else {
		panic("kexpand_block(): out of bounds VA '%x'", va);
		return NULL;
	}
	uint32 next_block_va = va + old_size;

	// if the next block is not free
	uint32 *page_table = NULL;
	struct FrameInfo *frame_info = get_frame_info(ptr_page_directory, va, &page_table);
	if (frame_info != NULL) {
		return NULL;
	}
    
	old_size /= PAGE_SIZE;
	new_size /= PAGE_SIZE;
	struct HeapBlock* cur_blk = to_heap_block(va);
    struct HeapBlock* next_blk = to_heap_block(next_block_va);
	uint32 next_block_size = next_blk->page_count;
	uint32 total_size = old_size + next_block_size;

	if (total_size < new_size) {
		return NULL;
	}

	struct HeapBlock* rm_blk = split_heap_block(next_blk, new_size - old_size);
	if(rm_blk != NULL) {
		LIST_INSERT_BEFORE(&free_blocks_list, next_blk, rm_blk);
	}
	LIST_REMOVE(&free_blocks_list, next_blk);

	cur_blk->page_count = new_size;

	return (void *)va;
}

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code
	//return NULL;
	//panic("krealloc() is not implemented yet...!!");
	if (virtual_address == NULL) {
		if (new_size == 0) {
			return NULL;
		} else {
			return kmalloc(new_size);
		}
	}

	if (new_size == 0) {
		kfree(virtual_address);
		return NULL;
	}

	void * new_allocated_va = (void *)virtual_address;
    uint32 old_size = 0;

    bool is_blk_addr = ((uint32) virtual_address >= kheap_start) &&
	                   ((uint32)virtual_address < kheap_break);
	bool is_page_addr = ((uint32) virtual_address >= kheap_limit + PAGE_SIZE) &&
				        ((uint32) virtual_address < KERNEL_HEAP_MAX);
						
   // get the size of block
	if(is_page_addr) {
		struct HeapBlock* blk = to_heap_block((uint32) virtual_address);
		old_size = blk->page_count * PAGE_SIZE;
	} else if(is_blk_addr) {
        old_size = get_block_size(virtual_address) - (2 * sizeof(uint32));
	} else {
		panic("krealloc(): out of bounds VA '%x'", virtual_address);
		return NULL;
	}
    
    // expand the block
	if(new_size >= old_size) {
		// the new and old bolcks in Block Allocator Area
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE){
			new_allocated_va = realloc_block_FF(virtual_address, new_size);
		}

        // the old bolck in Block Allocator Area but new block in Page Allocator Area
		else if(is_blk_addr){
             new_allocated_va = kmalloc(new_size);
			 // relocating happened
			 if(new_allocated_va != NULL) {
			    memmove(new_allocated_va, virtual_address, old_size);
			    kfree(virtual_address);
			 }
		}
        
		// the new and old bolcks in Page Allocator Area
		else {
		    new_size = ROUNDUP(new_size, PAGE_SIZE);
		    if(new_size == old_size) {
		        new_allocated_va = virtual_address ;
		    }
		    new_allocated_va = kexpand_block((uint32)virtual_address, new_size);

		    // expand failed. try relocating
		    if(new_allocated_va == NULL) {
			    new_allocated_va = kmalloc_with_krealloc((uint32)virtual_address, new_size);
			    // relocating happened
			    if(new_allocated_va != NULL){
		    	    kfree(virtual_address);
			    }
		    }
		}

		return new_allocated_va;
	}
    
	// shrink the block
	{
		// the new and old bolcks in Block Allocator Area
		if(is_blk_addr) {
			new_allocated_va = realloc_block_FF(virtual_address, new_size);
		}    
        // the new bolck in Block Allocator Area but old block in Page Allocator Area
		else if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE) {
             new_allocated_va = kmalloc(new_size);
			 // relocating happened
			 if(new_allocated_va != NULL) {
			    memmove(new_allocated_va, virtual_address, new_size);
			    kfree(virtual_address);
			 }
		}  
		// the new and old bolcks in Page Allocator Area
		else {
			new_allocated_va = virtual_address;
	        old_size /= PAGE_SIZE;
	        new_size = ROUNDUP(new_size, PAGE_SIZE) / PAGE_SIZE;
	        struct HeapBlock* cur_blk = to_heap_block((uint32)virtual_address);
	        struct HeapBlock* rm_blk = split_heap_block(cur_blk, new_size);
	        if(rm_blk != NULL) {
		        rm_blk->page_count = old_size - new_size ;
			    kfree(rm_blk);
	        }
            cur_blk->page_count = new_size;
		}
		return new_allocated_va;
	}
}

struct HeapBlock*
to_heap_block(uint32 va) {
	assert(va && (va >= PAGE_ALLOCATOR_START));
	uint32 offset = (va - PAGE_ALLOCATOR_START) / PAGE_SIZE;
	return heap_blocks + offset;
}

struct HeapBlock*
split_heap_block(struct HeapBlock* blk, uint32 required_pages)
{
	assert(blk);

	uint32 page_surplus = blk->page_count - required_pages;
	if (page_surplus == 0) {
		return NULL;
	}

	uint32 next_block_va = blk->start_va + (required_pages * PAGE_SIZE);
	struct HeapBlock* next_block = to_heap_block(next_block_va);
	next_block->page_count = page_surplus;
	next_block->start_va = next_block_va;

	blk->page_count = required_pages;

	return next_block;
}

void
insert_heap_block_sorted(struct HeapBlock* b) {
	assert(b);

	if (LIST_EMPTY(&free_blocks_list)) {
		LIST_INSERT_HEAD(&free_blocks_list, b);
		return;
	}

	struct HeapBlock* blk = NULL;
	LIST_FOREACH(blk, &free_blocks_list) {
		if (blk->start_va > b->start_va) {
			LIST_INSERT_BEFORE(&free_blocks_list, blk, b);
			return;
		}
	}

	LIST_INSERT_TAIL(&free_blocks_list, b);
}

void
coalescing_heap_block(struct HeapBlock* b) {
	assert(b);

	struct HeapBlock* prev = LIST_PREV(b);
	struct HeapBlock* next = LIST_NEXT(b);
	bool is_prev_free = (prev && (prev->start_va + prev->page_count * PAGE_SIZE == b->start_va));
	bool is_next_free = (next && (b->start_va + b->page_count * PAGE_SIZE == next->start_va));

	if (is_prev_free && is_next_free) {
		prev->page_count += b->page_count + next->page_count;
		LIST_REMOVE(&free_blocks_list, b);
		LIST_REMOVE(&free_blocks_list, next);
	} else if (is_prev_free) {
		prev->page_count += b->page_count;
		LIST_REMOVE(&free_blocks_list, b);
	} else if (is_next_free) {
		b->page_count += next->page_count;
		LIST_REMOVE(&free_blocks_list, next);
	}
}
