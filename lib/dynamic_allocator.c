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
		cprintf("(size: %d, isFree: %d)\n", get_block_size(blk), is_free_block(blk)) ;
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
reverse_lsb(void *blk){
	uint32 cur_blk_size = get_block_size(blk);
	// reverses lsb
    set_block_data(blk , cur_blk_size , is_free_block(blk));
}

void*
handle_allocation(void *required_blk, uint32 required_size) {
    if (required_blk != NULL) {
        // Split the block if it's too large
        block_split(required_blk, required_size);
        reverse_lsb(required_blk);

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
        reverse_lsb(required_blk);
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
		if (blk_size >= required_size){
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
		if (!is_initialized){
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
	uint32 Best_size = -1;
	void *required_blk = NULL;
	struct BlockElement *blk = NULL;

	LIST_FOREACH(blk, &freeBlocksList)
	{
		uint32 blk_size= get_block_size(blk); 
		if (blk_size >= required_size){
			if(Best_size == -1 || blk_size < Best_size){
				// Store the address of the allocated block
			    required_blk = (void *)blk;
				Best_size = blk_size;
			}
		}
	}
	
	return handle_allocation(required_blk , required_size);
}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================

uint32*
get_header(void *va)
{
	// move 4 bytes back from first free space.
	uint32 *header = (uint32*) va - 1;
	return header;
}

uint32*
get_footer(void *va)
{
	uint32 *header = (uint32*) va - 1;
	// move to end of footer than go back 4 bytes.
	uint32 *footer = (uint32*)((uint8*)header + get_block_size(va) - sizeof(uint32));
	return footer;
}


void 
merge(struct BlockElement *va , struct BlockElement * v2)
{
	// remove right block from free list 
	LIST_REMOVE(&freeBlocksList , v2);

	// adjust first block with new sizes
	uint32 new_sz = get_block_size(va) + get_block_size(v2);

	set_block_data(va , new_sz , 0);

}

void 
insert_sorted(struct BlockElement *va) {
	// if list is empty , element needs to be inserted as head
	if(LIST_EMPTY(&freeBlocksList)){
		LIST_INSERT_HEAD(&freeBlocksList , va);
		return;
	}

    struct BlockElement *cur_block = LIST_FIRST(&freeBlocksList);


	LIST_FOREACH(cur_block , &freeBlocksList){
		if(cur_block > va){ // insert before first element that exceeds me in location
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
	if(va == NULL || is_free_block(va)){
		return;
	}

	uint32 *cur_header = get_header(va);
	uint32 *cur_footer = get_footer(va);

	reverse_lsb(va);

	// insert either way
	insert_sorted(va);

	// means there is an element behind me that is free
	if(LIST_FIRST(&freeBlocksList) < (struct BlockElement *)va){
		uint32 *prev_footer = (uint32 *) cur_header - 1;
		uint32 prev_va_size = ((*prev_footer & ~(0x1)) - (2 * sizeof(uint32)));
		uint32 *prev_va =  (uint32 *) ((uint8 *)prev_footer - prev_va_size);

		// check if the block before me is free
		if(is_free_block(prev_va)){
			merge((struct BlockElement *)prev_va , va);
			va = prev_va;
		}
	}

	// means there is an element after me that is free
	if(LIST_LAST(&freeBlocksList) > (struct BlockElement *)va){
		uint32 *prev_footer = (uint32 *) cur_header - 1;
		uint32 *nxt_header = (uint32 *) cur_footer + 1;
		uint32 *nxt_va = (uint32 *) nxt_header + 1;

		// check if the block after me is free
		if(is_free_block(nxt_va)){
			merge(va , (struct BlockElement *)nxt_va);
		}
	}
	
}

//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("realloc_block_FF is not implemented yet");
	//Your Code is Here...
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
