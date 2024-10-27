/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (~(*curBlkMetaData) & 0x1) ;
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d, address: %p)\n", get_block_size(blk), is_free_block(blk), blk) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; //ensure it's multiple of 2
		if (initSizeOfAllocatedSpace == 0)
			return ;
		is_initialized = 1;
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator

	LIST_INIT(&freeBlocksList);
	uint32 *beg_block = (uint32*)daStart;
	*beg_block = 1;
	struct BlockElement *first_free_block = (struct BlockElement*)(beg_block + 2); // skip the BEG block (1 word) and the block's header (1 word) to initialize the first free block 
	uint32 first_free_block_size = initSizeOfAllocatedSpace - (2 * sizeof(uint32)); // size of the free block subtracting BEG & END
	set_block_data(first_free_block, first_free_block_size, 0);
	LIST_INSERT_HEAD(&freeBlocksList, first_free_block);
	uint32 *end_block = (uint32*)(daStart + initSizeOfAllocatedSpace - sizeof(uint32));
	*end_block = 1;
}
//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================
void set_block_data(void* va, uint32 totalSize, bool isAllocated)
{
	//TODO: [PROJECT'24.MS1 - #05] [3] DYNAMIC ALLOCATOR - set_block_data
	
	if (totalSize % 2 != 0) totalSize++;	//ensure that the size is even (to use LSB as allocation flag)
	if (totalSize < 16)
		panic("Block size must be at least 16 bits.\n");
	if (!is_initialized)
	{
		uint32 required_size = totalSize + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
		uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}

	uint32 *header = (uint32*) va - 1;
	uint32 *footer = (uint32*)((uint8*)header + totalSize - sizeof(uint32));
	*header = (totalSize | isAllocated);
    *footer = *header;
}


//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void 
block_split(void *blk, uint32 size)
{
    uint32 cur_blk_size = get_block_size(blk);

   // Min size for block (8 bytes for prev and nxt pointer + 8 bytes for header and footer)
	uint32 min_blk_size = DYN_ALLOC_MIN_BLOCK_SIZE + 2 * sizeof(int);

    // Ensure the remaining block size after splitting can carry a block
    if (cur_blk_size - size < min_blk_size ){
        return;
	}

    set_block_data(blk, size, 0);

	// new block data
    uint32 new_blk_size = cur_blk_size - size;
    void *new_blk = (uint8 *)blk + size;

    // Set data for the new block
    set_block_data(new_blk, new_blk_size, 0);

    // Insert the new block into the free block list after the cur block
    LIST_INSERT_AFTER(&freeBlocksList, (struct BlockElement *)blk, (struct BlockElement *)new_blk);
}

void 
mark_blk_allocated(void *blk){
	uint32 cur_blk_size = get_block_size(blk);
    set_block_data(blk , cur_blk_size , 1);
}

void 
mark_blk_freed(void *blk){
	uint32 cur_blk_size = get_block_size(blk);
    set_block_data(blk , cur_blk_size , 0);
}


void*
handle_allocation(void *required_blk, uint32 required_size) {
    if (required_blk != NULL) {
        // Split the block if it's too large
        block_split(required_blk, required_size);
        mark_blk_allocated(required_blk);

        // Remove the block from the free list
        LIST_REMOVE(&freeBlocksList, (struct BlockElement*)required_blk);
        return required_blk;
    }

    // If no fitting block is found, request more memory from sbrk
    required_blk = sbrk(required_size);
    if ((int32)required_blk == -1) {
        return NULL;
    } else {
        // Split the block if it's too large
        block_split(required_blk, required_size);
        mark_blk_allocated(required_blk);
        return required_blk;
    }
}

void*
alloc_block_FF(uint32 size)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #06] [3] DYNAMIC ALLOCATOR - alloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_FF is not implemented yet");
	//Your Code is Here...
	if (size == 0)
		return NULL;

    // Calculate required size for the block (size of the block + 8 bytes for header and footer)
	uint32 required_size = size + 2 * sizeof(int);
	void *required_blk = NULL;
	struct BlockElement *blk = NULL;

	LIST_FOREACH(blk, &freeBlocksList)
	{
		uint32 blk_size = get_block_size(blk);
		if (blk_size >= required_size) {
			// Store the address of the allocated block
			required_blk = (void *)blk;
			break;
		}
	}
    
	return handle_allocation(required_blk , required_size);
}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void*
alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_BF is not implemented yet");
	//Your Code is Here...
    //==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized) {
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
	//==================================================================================
    if (size == 0)
		return NULL;

	// Calculate required size for the block (size of the block + 8 bytes for header and footer)
	uint32 required_size = size + 2 * sizeof(int) /*header & footer*/;
	uint32 best_size = -1;
	void *required_blk = NULL;
	struct BlockElement *blk = NULL;

	LIST_FOREACH(blk, &freeBlocksList)
	{
		uint32 blk_size = get_block_size(blk); 
		if (blk_size >= required_size) {
			if (best_size == -1 || blk_size < best_size) {
				// Store the address of the allocated block
			    required_blk = (void *)blk;
				best_size = blk_size;
			}
		}
	}
	
	return handle_allocation(required_blk , required_size);
}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================

uint32*
get_header_of_block(void *va)
{
	// move 4 bytes back from first free space.
	uint32 *header = (uint32*) va - 1;
	return header;
}

uint32*
get_footer_of_block(void *va)
{
	uint32 *header = (uint32*) va - 1;
	// move to end of footer than go back 4 bytes.
	uint32 *footer = (uint32*)((uint8*)header + get_block_size(va) - sizeof(uint32));
	return footer;
}


void 
merge_blocks(struct BlockElement *va , struct BlockElement * v2)
{
	// remove right block from free list 
	LIST_REMOVE(&freeBlocksList , v2);

	// adjust first block with new sizes
	uint32 new_sz = get_block_size(va) + get_block_size(v2);

	set_block_data(va , new_sz , 0);

}

void 
insert_sorted_into_free_list(struct BlockElement *va) {
	// if list is empty , element needs to be inserted as head
	if (LIST_EMPTY(&freeBlocksList)) {
		LIST_INSERT_HEAD(&freeBlocksList , va);
		return;
	}

    struct BlockElement *cur_block = LIST_FIRST(&freeBlocksList);


	LIST_FOREACH(cur_block , &freeBlocksList) {
		if (cur_block > va) { // insert before first element that exceeds me in location
			LIST_INSERT_BEFORE(&freeBlocksList , cur_block , va);
			return;
		}
	}// needs to be inserted in the end
	LIST_INSERT_TAIL(&freeBlocksList , va);

}


void 
free_block(void *va)
{
	// if null or already free just return.
	if (va == NULL || is_free_block(va)) {
		return;
	}

	uint32 *cur_header = get_header_of_block(va);
	uint32 *cur_footer = get_footer_of_block(va);

	mark_blk_freed(va);

	// insert either way
	insert_sorted_into_free_list(va);

	// means there is an element behind me that is free
	if (LIST_FIRST(&freeBlocksList) < (struct BlockElement *)va) {
		uint32 *prev_footer = (uint32 *) cur_header - 1;
		uint32 prev_va_size = ((*prev_footer & ~(0x1)) - (2 * sizeof(uint32)));
		uint32 *prev_va =  (uint32 *) ((uint8 *)prev_footer - prev_va_size);

		// check if the block before me is free
		if (is_free_block(prev_va)) {
			merge_blocks((struct BlockElement *)prev_va , va);
			va = prev_va;
		}
	}

	// means there is an element after me that is free
	if (LIST_LAST(&freeBlocksList) > (struct BlockElement *)va) {
		uint32 *prev_footer = (uint32 *) cur_header - 1;
		uint32 *nxt_header = (uint32 *) cur_footer + 1;
		uint32 *nxt_va = (uint32 *) nxt_header + 1;

		// check if the block after me is free
		if (is_free_block(nxt_va)) {
			merge_blocks(va , (struct BlockElement *)nxt_va);
		}
	}
	
}

//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================

int
is_valid_block(void* va)
{
	// BEG/END are special blocks, their size=0 and marked allocated
	// so if the block doesn't have these specs, it's valid to use
	return !(get_block_size(va) == 0 && !is_free_block(va));
}

void
set_data_and_split_block(void* va, uint32 new_required_size, uint32 remaining_block_size)
{
	// take only required size
	set_block_data(va, new_required_size, 1);

	// split the remaining block to be free one
	uint32 *free_block_va = va + new_required_size;
	set_block_data(free_block_va, remaining_block_size, 1);
	free_block(free_block_va);
}

void*
shrink(void* va, uint32 new_required_size)
{
	// avoid accessing nullptr
	assert(va != NULL);

	uint32 old_size = get_block_size(va);
	uint32 remaining_block_size = old_size - new_required_size;

	// avoid unsigned int wrapping
	assert(old_size >= new_required_size);

	// if remaining_block_size >= 16: split
	// else: check if can be merged with a free block infront of it and resulting_size >= 16
	if (remaining_block_size >= 16) {
		set_data_and_split_block(va, new_required_size, remaining_block_size);
	} else {
		uint32 *next_block_va = va + old_size;

		// if next block isn't END, and is free: can be merged
		// else: size stays the same
		if (is_valid_block(next_block_va) && is_free_block(next_block_va)) {
			// a valid block can be made, as next_block_va is at least 16
			uint32 merged_block_size = remaining_block_size + get_block_size(next_block_va);

			// va only takes required size
			set_block_data(va, new_required_size, 1);

			// making the new free block
			// this will use free_part_va, have space for a header behind it
			// even it entered the next_block_va area
			uint32 *free_part_va = va + new_required_size;

			// this will use the header of the free part right after the shrinked block
			// the footer of the next free block, resulting in a new merged bigger free block
			LIST_REMOVE(&freeBlocksList, (struct BlockElement*)next_block_va);
			set_block_data(free_part_va, merged_block_size, 1);
			free_block(free_part_va);
		}
	}
	return va;
}

void*
expand(void* va, uint32 new_required_size)
{
	// avoid accessing nullptr
	assert(va != NULL);

	uint32 old_size = get_block_size(va);
	uint32 *next_block_va = va + old_size;

	// if the next block is the end, cannot expand
	if (!is_valid_block(next_block_va)) {
		return NULL;
	}

	uint32 next_block_size = get_block_size(next_block_va);
	uint32 total_size = old_size + next_block_size;
	if (!is_free_block(next_block_va) || total_size < new_required_size) {
		return NULL;
	}

	LIST_REMOVE(&freeBlocksList, (struct BlockElement*)next_block_va);

	uint32 remaining_block_size = total_size - new_required_size;

	// can split, and get a new freeBlock
	if (remaining_block_size >= 16) {
		set_data_and_split_block(va, new_required_size, remaining_block_size);
	} else {
		// cannot split, leading to internal fragmentation
		set_block_data(va, total_size, 1);
	}

	return va;
}

void
move_block_data(void* old_va, void* new_va, int old_total_size)
{
	// to ignore size of the metadata
	old_total_size -= 8;

	// given size is in bytes
	int size = old_total_size / sizeof(int32);
	for (int i = 0 ; i < size ; i++)
	{
		int32 *cur = (int32*)old_va + i;
		int32 *new = (int32*)new_va + i;
		*new = *cur;
	}
}

void*
realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//Your Code is Here...

	// handle nulls cases:
	if (va == NULL) {
		if (new_size == 0) {
			return NULL;
		} else {
			return alloc_block_FF(new_size);
		}
	}

	if (new_size == 0) {
		free_block(va);
		return NULL;
	}

	{
		if (new_size % 2 != 0) {
			// ensure that the size is even (to use LSB as allocation flag)
			new_size++;
		}
		if (new_size < DYN_ALLOC_MIN_BLOCK_SIZE) {
			new_size = DYN_ALLOC_MIN_BLOCK_SIZE;
		}
	}

	uint32 old_size = get_block_size(va);
	uint32 new_required_size = new_size + 2 * sizeof(uint32);

	if (is_free_block(va)) {
		LIST_REMOVE(&freeBlocksList, (struct BlockElement*)va);
		set_block_data(va, old_size, 1);
	}

	if (new_required_size == old_size) {
		return va;
	}

	if (new_required_size < old_size) {
		return shrink(va, new_required_size);
	} else {
		void *new_va = expand(va, new_required_size);
		
		// expand happened
		if (new_va == va) {
			return new_va;
		// expand failed. try relocating
		} else {
			void *new_allocated_va = alloc_block_FF(new_size);
			if (new_allocated_va == NULL) {
				return NULL;
			}

			move_block_data(va, new_allocated_va, old_size);
			free_block(va);
			return new_allocated_va;
		}
	}
}

/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}