/*
 * test_lists_managment.c

 *
 *  Created on: Oct 6, 2022
 *  Updated on: Sept 20, 2023
 *      Author: HP
 */
#include <inc/queue.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/dynamic_allocator.h>
#include <inc/memlayout.h>

//NOTE: ALL tests in this file shall work with USE_KHEAP = 0

#define Mega  (1024*1024)
#define kilo (1024)

void test_initialize_dynamic_allocator()
{
#if USE_KHEAP
	panic("test_initialize_dynamic_allocator: the kernel heap should be diabled. make sure USE_KHEAP = 0");
	return;
#endif

	//write initial data at the start (for checking)
	int* tmp_ptr = (int*)KERNEL_HEAP_START;
	*tmp_ptr = -1 ;
	*(tmp_ptr+1) = 1 ;

	uint32 initAllocatedSpace = 2*Mega;
	initialize_dynamic_allocator(KERNEL_HEAP_START, initAllocatedSpace);


	//Check#1: Metadata
	uint32* daBeg = (uint32*) KERNEL_HEAP_START ;
	uint32* daEnd = (uint32*) (KERNEL_HEAP_START +  initAllocatedSpace - sizeof(int));
	uint32* blkHeader = (uint32*) (KERNEL_HEAP_START + sizeof(int));
	uint32* blkFooter = (uint32*) (KERNEL_HEAP_START +  initAllocatedSpace - 2*sizeof(int));
	if (*daBeg != 1 || *daEnd != 1 || (*blkHeader != initAllocatedSpace - 2*sizeof(int))|| (*blkFooter != initAllocatedSpace - 2*sizeof(int)))
	{
		panic("Content of header/footer and/or DA begin/end are not set correctly");
	}
	if (LIST_SIZE(&freeBlocksList) != 1 || (uint32)LIST_FIRST(&freeBlocksList) != KERNEL_HEAP_START + 2*sizeof(int))
	{
		panic("free block is not added correctly");
	}

	cprintf("Congratulations!! test initialize_dynamic_allocator completed successfully.\n");
}

#define numOfAllocs 7
#define allocCntPerSize 200
#define sizeOfMetaData 8

//NOTE: these sizes include the size of MetaData within it
uint32 allocSizes[numOfAllocs] = {4*kilo, 20*sizeof(char) + sizeOfMetaData, 1*kilo, 3*sizeof(int) + sizeOfMetaData, 2*kilo, 2*sizeOfMetaData, 7*kilo} ;
short* startVAs[numOfAllocs*allocCntPerSize+1] ;
short* midVAs[numOfAllocs*allocCntPerSize+1] ;
short* endVAs[numOfAllocs*allocCntPerSize+1] ;

int test_initial_alloc(int ALLOC_STRATEGY)
{
#if USE_KHEAP
	panic("test_initial_alloc: the kernel heap should be disabled. make sure USE_KHEAP = 0");
	return 0;
#endif

	int eval = 0;
	bool is_correct = 1;
	int initAllocatedSpace = 3*Mega;
	initialize_dynamic_allocator(KERNEL_HEAP_START, initAllocatedSpace);

	void * va ;
	//====================================================================//
	/*INITIAL ALLOC Scenario 1: Try to allocate a block with a size greater than the size of any existing free block*/
	cprintf("	1: Try to allocate large block [not fit in any space]\n\n") ;

	is_correct = 1;
	va = alloc_block(3*initAllocatedSpace, ALLOC_STRATEGY);

	//Check returned va
	if(va != NULL)
	{
		is_correct = 0;
		cprintf("alloc_block_xx #1: should not be allocated.\n");
	}
	va = alloc_block(initAllocatedSpace, ALLOC_STRATEGY);

	//Check returned va
	if(va != NULL)
	{
		is_correct = 0;
		cprintf("alloc_block_xx #2: should not be allocated.\n");
	}

	if (is_correct)
	{
		eval += 5;
	}
	//====================================================================//
	/*INITIAL ALLOC Scenario 2: Try to allocate set of blocks with different sizes*/
	cprintf("	2: Try to allocate set of blocks with different sizes [all should fit]\n\n") ;
	is_correct = 1;

	int totalSizes = 0;
	for (int i = 0; i < numOfAllocs; ++i)
	{
		totalSizes += allocSizes[i] * allocCntPerSize ;
	}
	int remainSize = initAllocatedSpace - totalSizes - 2*sizeof(int) ; //exclude size of "DA Begin & End" blocks
	//cprintf("\n********* Remaining size = %d\n", remainSize);
	if (remainSize <= 0)
	{
		is_correct = 0;
		cprintf("alloc_block_xx test is not configured correctly. Consider updating the initial allocated space OR the required allocations\n");
	}
	int idx = 0;
	void* curVA = (void*) KERNEL_HEAP_START + sizeof(int) ; //just after the "DA Begin" block
	uint32 actualSize;
	for (int i = 0; i < numOfAllocs; ++i)
	{
		for (int j = 0; j < allocCntPerSize; ++j)
		{
			actualSize = allocSizes[i] - sizeOfMetaData;
			va = startVAs[idx] = alloc_block(actualSize, ALLOC_STRATEGY);
			midVAs[idx] = va + actualSize/2 ;
			endVAs[idx] = va + actualSize - sizeof(short);
			//Check returned va
			if(va == NULL || (va != (curVA + sizeOfMetaData/2)))
			{
				if (is_correct)
				{
					is_correct = 0;
					cprintf("alloc_block_xx #3.%d: WRONG ALLOC - alloc_block_xx return wrong address. Expected %x, Actual %x\n", idx, curVA + sizeOfMetaData ,va);
				}
			}
			curVA += allocSizes[i] ;
			*(startVAs[idx]) = idx ;
			*(midVAs[idx]) = idx ;
			*(endVAs[idx]) = idx ;
			idx++;
		}
		//if (is_correct == 0)
		//break;
	}
	if (is_correct)
	{
		eval += 20;
	}
	//====================================================================//
	/*INITIAL ALLOC Scenario 3: Try to allocate a block with a size equal to the size of the first existing free block*/
	cprintf("	3: Try to allocate a block with equal to the first existing free block\n\n") ;
	is_correct = 1;

	actualSize = remainSize - sizeOfMetaData;
	va = startVAs[idx] = alloc_block(actualSize, ALLOC_STRATEGY);
	midVAs[idx] = va + actualSize/2 ;
	endVAs[idx] = va + actualSize - sizeof(short);
	//Check returned va
	if(va == NULL || (va != (curVA + sizeOfMetaData/2)) || LIST_SIZE(&freeBlocksList) != 0)
	{
		is_correct = 0;
		cprintf("alloc_block_xx #4: WRONG ALLOC.\n");
	}
	*(startVAs[idx]) = idx ;
	*(midVAs[idx]) = idx ;
	*(endVAs[idx]) = idx ;
	if (is_correct)
	{
		eval += 5;
	}
	//====================================================================//
	/*INITIAL ALLOC Scenario 4: Check stored data inside each allocated block*/
	cprintf("	4: Check stored data inside each allocated block\n\n") ;
	is_correct = 1;

	for (int i = 0; i < idx; ++i)
	{
		if (*(startVAs[i]) != i || *(midVAs[i]) != i ||	*(endVAs[i]) != i)
		{
			is_correct = 0;
			cprintf("alloc_block_xx #4.%d: WRONG! content of the block is not correct. Expected %d\n",i, i);
			break;
		}
	}
	if (is_correct)
	{
		eval += 10;
	}
	return eval;
}

void test_alloc_block_FF()
{
#if USE_KHEAP
	panic("test_alloc_block_FF: the kernel heap should be disabled. make sure USE_KHEAP = 0");
	return;
#endif

	int eval = 0;
	bool is_correct;
	void* va = NULL;
	uint32 actualSize = 0;

	cprintf("=======================================================\n") ;
	cprintf("FIRST: Tests depend on the Allocate Function ONLY [40%]\n") ;
	cprintf("=======================================================\n") ;
	eval = test_initial_alloc(DA_FF);

	cprintf("====================================================\n") ;
	cprintf("SECOND: Tests depend on BOTH Allocate and Free [60%] \n") ;
	cprintf("====================================================\n") ;

	//Free set of blocks with different sizes (first block of each size)
	for (int i = 0; i < numOfAllocs; ++i)
	{
		free_block(startVAs[i*allocCntPerSize]);
	}
	is_correct = 1;
	//Check number of freed blocks
	if(LIST_SIZE(&freeBlocksList) != numOfAllocs)
	{
		is_correct = 0;
		cprintf("alloc_block_FF #5: WRONG FREE. unexpected number of freed blocks\n");
	}
	if (is_correct)
	{
		eval += 10;
	}
	//====================================================================//
	/*FF ALLOC Scenario 1: Try to allocate a block with a size greater than the size of any existing free block*/
	cprintf("	1: Try to allocate large block [not fit in any space]\n\n") ;
	is_correct = 1;

	uint32 maxSize = 0 ;
	for (int i = 0; i < numOfAllocs; ++i)
	{
		if (allocSizes[i] > maxSize)
			maxSize = allocSizes[i] ;
	}
	va = alloc_block(maxSize, DA_FF);

	//Check returned va
	if(va != NULL)
	{
		is_correct = 0;
		cprintf("alloc_block_FF #5: WRONG FF ALLOC - alloc_block_FF find a block instead no existing free blocks with the given size.\n");
	}

	if (is_correct)
	{
		eval += 5;
	}
	//====================================================================//
	/*FF ALLOC Scenario 2: Try to allocate blocks with sizes smaller than existing free blocks*/
	cprintf("	2: Try to allocate set of blocks with different sizes smaller than existing free blocks\n\n") ;
	is_correct = 1;

#define numOfFFTests 3
	uint32 startVA = KERNEL_HEAP_START + sizeof(int); //just after the DA Begin block
	uint32 testSizes[numOfFFTests] = {1*kilo + kilo/2, 3*kilo, kilo/2} ;
	uint32 startOf1st7KB = (uint32)startVAs[6*allocCntPerSize];
	uint32 expectedVAs[numOfFFTests] = {startVA + sizeOfMetaData/2, startOf1st7KB, startVA + testSizes[0] + sizeOfMetaData/2};
	short* tstStartVAs[numOfFFTests+1] ;
	short* tstMidVAs[numOfFFTests+1] ;
	short* tstEndVAs[numOfFFTests+1] ;
	for (int i = 0; i < numOfFFTests; ++i)
	{
		actualSize = testSizes[i] - sizeOfMetaData;
		va = tstStartVAs[i] = alloc_block(actualSize, DA_FF);
		tstMidVAs[i] = va + actualSize/2 ;
		tstEndVAs[i] = va + actualSize - sizeof(short);
		//Check returned va
		if(tstStartVAs[i] == NULL || (tstStartVAs[i] != (short*)expectedVAs[i]))
		{
			is_correct = 0;
			cprintf("alloc_block_FF #6.%d: WRONG FF ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", i, expectedVAs[i] ,tstStartVAs[i]);
			//break;
		}
		*(tstStartVAs[i]) = 353;
		*(tstMidVAs[i]) = 353;
		*(tstEndVAs[i]) = 353;
	}
	if (is_correct)
	{
		eval += 15;
	}
	//====================================================================//
	/*FF ALLOC Scenario 3: Try to allocate a block with a size equal to the size of the first existing free block*/
	cprintf("	3: Try to allocate a block with equal to the first existing free block\n\n") ;
	is_correct = 1;

	actualSize = 2*kilo - sizeOfMetaData;
	va = tstStartVAs[numOfFFTests] = alloc_block(actualSize, DA_FF);
	tstMidVAs[numOfFFTests] = va + actualSize/2 ;
	tstEndVAs[numOfFFTests] = va + actualSize - sizeof(short);
	//Check returned va
	void* expected = (void*)(startVA + testSizes[0] + testSizes[2] + sizeOfMetaData/2) ;
	if(va == NULL || (va != expected))
	{
		is_correct = 0;
		cprintf("alloc_block_FF #7: WRONG FF ALLOC - alloc_block_FF return wrong address.expected %x, actual %x\n", expected, va);
	}
	if(LIST_SIZE(&freeBlocksList) != numOfAllocs - 1)
	{
		is_correct = 0;
		cprintf("alloc_block_FF #7: WRONG FF ALLOC - unexpected number of free blocks. expected %d, actual %d\n", numOfAllocs-1, LIST_SIZE(&freeBlocksList));
	}

	*(tstStartVAs[numOfFFTests]) = 353 ;
	*(tstMidVAs[numOfFFTests]) = 353 ;
	*(tstEndVAs[numOfFFTests]) = 353 ;

	if (is_correct)
	{
		eval += 15;
	}
	//====================================================================//
	/*FF ALLOC Scenario 4: Check stored data inside each allocated block*/
	cprintf("	4: Check stored data inside each allocated block\n\n") ;
	is_correct = 1;

	for (int i = 0; i <= numOfFFTests; ++i)
	{
		//cprintf("startVA = %x, mid = %x, last = %x\n", tstStartVAs[i], tstMidVAs[i], tstEndVAs[i]);
		if (*(tstStartVAs[i]) != 353 || *(tstMidVAs[i]) != 353 || *(tstEndVAs[i]) != 353)
		{
			is_correct = 0;
			cprintf("alloc_block_FF #8.%d: WRONG! content of the block is not correct. Expected=%d, val1=%d, val2=%d, val3=%d\n",i, 353, *(tstStartVAs[i]), *(tstMidVAs[i]), *(tstEndVAs[i]));
			break;
		}
	}

	if (is_correct)
	{
		eval += 15;
	}
	cprintf("test alloc_block_FF completed. Evaluation = %d%\n", eval);
}

void test_alloc_block_BF()
{
#if USE_KHEAP
	panic("test_alloc_block_BF: the kernel heap should be disabled. make sure USE_KHEAP = 0");
	return;
#endif

	panic("Test is under construction! will be announced later isA");
}

void test_alloc_block_NF()
{

	//====================================================================//
	/*NF ALLOC Scenario 1: Try to allocate a block with a size greater than the size of any existing free block*/

	//====================================================================//
	/*NF ALLOC Scenario 2: Try to allocate a block with a size equal to the size of the one existing free blocks (STARTING from 0)*/

	//====================================================================//
	/*NF ALLOC Scenario 3: Try to allocate a block with a size equal to the size of the one existing free blocks (The first one fit after the last allocated VA)*/

	//====================================================================//
	/*NF ALLOC Scenario 4: Try to allocate a block with a size smaller than the size of any existing free block (The first one fit after the last allocated VA)*/

	//====================================================================//
	/*NF ALLOC Scenario 5: Try to allocate a block with a size smaller than the size of any existing free block (One from the updated blocks before in the free list)*/

	//====================================================================//
	/*NF ALLOC Scenario 6: Try to allocate a block with a size smaller than ALL the NEXT existing blocks .. Shall start search from the start of the list*/

	//====================================================================//
	/*NF ALLOC Scenario 7: Try to allocate a block with a size smaller than the existing blocks .. To try to update head not to remove it*/

	//cprintf("Congratulations!! test alloc_block_NF completed successfully.\n");

}

void test_free_block_FF()
{

#if USE_KHEAP
	panic("test_free_block: the kernel heap should be disabled. make sure USE_KHEAP = 0");
	return;
#endif

	cprintf("===========================================================\n") ;
	cprintf("NOTE: THIS TEST IS DEPEND ON BOTH ALLOCATE & FREE FUNCTIONS\n") ;
	cprintf("===========================================================\n") ;

	int eval = 0;
	bool is_correct;
	int initAllocatedSpace = 3*Mega;
	initialize_dynamic_allocator(KERNEL_HEAP_START, initAllocatedSpace);

	void * va ;
	//====================================================================//
	/* Try to allocate set of blocks with different sizes*/
	cprintf("	1: Try to allocate set of blocks with different sizes to fill-up the allocated space\n\n") ;

	int totalSizes = 0;
	for (int i = 0; i < numOfAllocs; ++i)
	{
		totalSizes += allocSizes[i] * allocCntPerSize ;
	}
	int remainSize = initAllocatedSpace - totalSizes - 2*sizeof(int) ; //exclude size of "DA Begin & End" blocks
	if (remainSize <= 0)
		panic("test_free_block is not configured correctly. Consider updating the initial allocated space OR the required allocations");

	int idx = 0;
	void* curVA = (void*) KERNEL_HEAP_START + sizeof(int) ; //just after the "DA Begin" block
	uint32 actualSize;
	for (int i = 0; i < numOfAllocs; ++i)
	{
		for (int j = 0; j < allocCntPerSize; ++j)
		{
			actualSize = allocSizes[i] - sizeOfMetaData;
			va = startVAs[idx] = alloc_block(actualSize, DA_FF);
			midVAs[idx] = va + actualSize/2 ;
			endVAs[idx] = va + actualSize - sizeof(short);
			//Check returned va
			if(va == NULL || (va != (curVA + sizeOfMetaData/2)))
				panic("test_free_block #1.%d: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x", idx, curVA + sizeOfMetaData ,va);
			curVA += allocSizes[i] ;
			*(startVAs[idx]) = idx ;
			*(midVAs[idx]) = idx ;
			*(endVAs[idx]) = idx ;
			idx++;
		}
	}

	//====================================================================//
	/* Try to allocate a block with a size equal to the size of the first existing free block*/
	actualSize = remainSize - sizeOfMetaData;
	va = startVAs[idx] = alloc_block(actualSize, DA_FF);
	midVAs[idx] = va + actualSize/2 ;
	endVAs[idx] = va + actualSize - sizeof(short);
	//Check returned va
	if(va == NULL || (va != (curVA + sizeOfMetaData/2)))
		panic("test_free_block #2: WRONG ALLOC - alloc_block_FF return wrong address.");
	*(startVAs[idx]) = idx ;
	*(midVAs[idx]) = idx ;
	*(endVAs[idx]) = idx ;

	//====================================================================//
	/* Check stored data inside each allocated block*/
	cprintf("	2: Check stored data inside each allocated block\n\n") ;
	is_correct = 1;

	for (int i = 0; i < idx; ++i)
	{
		if (*(startVAs[i]) != i || *(midVAs[i]) != i ||	*(endVAs[i]) != i)
		{
			is_correct = 0;
			cprintf("test_free_block #3.%d: WRONG! content of the block is not correct. Expected %d\n",i, i);
			break;
		}
	}

	//====================================================================//
	/* free_block Scenario 1: Free some allocated blocks [no coalesce]*/
	cprintf("	3: Free some allocated block [no coalesce]\n\n") ;
	uint32 block_size, expected_size, *blk_header, *blk_footer;
	is_correct = 1;

	//Free set of blocks with different sizes (first block of each size)
	for (int i = 0; i < numOfAllocs; ++i)
	{
		free_block(startVAs[i*allocCntPerSize]);
		uint32 block_size = get_block_size(startVAs[i*allocCntPerSize]) ;
		if (block_size != allocSizes[i])
		{
			if (is_correct)
			{
				is_correct = 0;
				cprintf("test_free_block #4.%d: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",i, allocSizes[i],block_size);
			}
		}
		int8 block_status = is_free_block(startVAs[i*allocCntPerSize]) ;
		if (block_status != 1)
		{
			if (is_correct)
			{
				is_correct = 0;
				cprintf("test_free_block #5.%d: WRONG FREE! block status not marked as free.\n", i);
			}
		}
	}
	uint32 expectedNumOfFreeBlks = numOfAllocs;
	is_correct = 1;
	if (LIST_SIZE(&freeBlocksList) != expectedNumOfFreeBlks)
	{
		is_correct = 0;
		cprintf("test_free_block #5.oo: WRONG number of freed blocks in the freeBlockList.\n");
	}
	if (is_correct)
	{
		eval += 10;
	}

	is_correct = 1;
	//Free last block
	free_block(startVAs[numOfAllocs*allocCntPerSize]);
	block_size = get_block_size(startVAs[numOfAllocs*allocCntPerSize]) ;
	if (block_size != remainSize)
	{
		is_correct = 0;
		cprintf("test_free_block #6.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",remainSize,block_size);
	}
	int8 block_status = is_free_block(startVAs[numOfAllocs*allocCntPerSize]) ;
	if (block_status != 1)
	{
		is_correct = 0;
		cprintf("test_free_block #6.2: WRONG FREE! WRONG FREE! block status not marked as free.\n");
	}

	//Reallocate last block
	actualSize = remainSize - sizeOfMetaData;
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	if(va == NULL || (va != (curVA + sizeOfMetaData/2)))
	{
		is_correct = 0;
		cprintf("test_free_block #6.3: WRONG ALLOC - alloc_block_FF return wrong address.\n");
	}

	//Free block before last
	free_block(startVAs[numOfAllocs*allocCntPerSize - 1]);
	block_size = get_block_size(startVAs[numOfAllocs*allocCntPerSize - 1]) ;
	if (block_size != allocSizes[numOfAllocs-1])
	{
		is_correct = 0;
		cprintf("test_free_block #6.4: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",allocSizes[numOfAllocs-1],block_size);
	}
	block_status = is_free_block(startVAs[numOfAllocs*allocCntPerSize-1]) ;
	if (block_status != 1)
	{
		is_correct = 0;
		cprintf("test_free_block #6.5: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
	}

	//Reallocate first block
	actualSize = allocSizes[0] - sizeOfMetaData;
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	if(va == NULL || (va != (void*)(KERNEL_HEAP_START + sizeof(int) + sizeOfMetaData/2)))
	{
		is_correct = 0;
		cprintf("test_free_block #7.1: WRONG ALLOC - alloc_block_FF return wrong address.\n");
	}

	//Free 2nd block
	free_block(startVAs[1]);
	block_size = get_block_size(startVAs[1]) ;
	if (block_size != allocSizes[0])
	{
		is_correct = 0;
		cprintf("test_free_block #7.2: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",allocSizes[0],block_size);
	}
	block_status = is_free_block(startVAs[1]) ;
	if (block_status != 1)
	{
		is_correct = 0;
		cprintf("test_free_block #7.3: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
	}
	is_correct = 1;
	expectedNumOfFreeBlks++ ;
	if (LIST_SIZE(&freeBlocksList) != expectedNumOfFreeBlks)
	{
		is_correct = 0;
		cprintf("test_free_block #7.4: WRONG number of freed blocks in the freeBlockList.\n");
	}
	if (is_correct)
	{
		eval += 10;
	}

	//====================================================================//
	/*free_block Scenario 2: Merge with previous ONLY (AT the tail)*/
	cprintf("	4: Free some allocated blocks [Merge with previous ONLY]\n\n") ;
	cprintf("		4.1: at the tail\n\n") ;
	is_correct = 1;
	//Free last block (coalesce with previous)
	uint32 blockIndex = numOfAllocs*allocCntPerSize;
	free_block(startVAs[blockIndex]);
	block_size = get_block_size(startVAs[blockIndex-1]) ;
	expected_size = remainSize + allocSizes[numOfAllocs-1];
	if (block_size != expected_size)
	{
		is_correct = 0;
		cprintf("test_free_block #8.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",remainSize + allocSizes[numOfAllocs-1],block_size);
	}
	block_status = is_free_block(startVAs[blockIndex-1]) ;
	if (block_status != 1)
	{
		is_correct = 0;
		cprintf("test_free_block #8.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
	}

	blk_header = (uint32*)((uint32)startVAs[blockIndex-1] - sizeof(int));
	blk_footer = (uint32*)((uint32)startVAs[blockIndex-1]+expected_size-2*sizeof(int));
	if (*(blk_header) != *(blk_footer))
	{
		is_correct = 0;
		cprintf("test_free_block #8.3: WRONG FREE! make sure to set block header and footer with the same values.\n");
	}

	//====================================================================//
	/*free_block Scenario 3: Merge with previous ONLY (between 2 blocks)*/
	cprintf("		4.2: between 2 blocks\n\n") ;
	blockIndex = 2*allocCntPerSize+1 ;
	free_block(startVAs[blockIndex]);
	block_size = get_block_size(startVAs[blockIndex-1]) ;
	expected_size = allocSizes[2]+allocSizes[2];
	if (block_size != expected_size)
	{
		is_correct = 0;
		cprintf	("test_free_block #9.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",allocSizes[2] + allocSizes[2],block_size);
	}
	block_status = is_free_block(startVAs[blockIndex-1]) ;
	if (block_status != 1)
	{
		is_correct = 0;
		cprintf("test_free_block #9.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
	}

	blk_header = (uint32*)((uint32)startVAs[blockIndex-1] - sizeof(int));
	blk_footer = (uint32*)((uint32)startVAs[blockIndex-1]+expected_size-2*sizeof(int));
	if (*(blk_header) != *(blk_footer))
	{
		is_correct = 0;
		cprintf("test_free_block #9.3: WRONG FREE! make sure to set block header and footer with the same values.\n");
	}
	is_correct = 1;
	if (LIST_SIZE(&freeBlocksList) != expectedNumOfFreeBlks)
	{
		is_correct = 0;
		cprintf("test_free_block #9.4: WRONG number of freed blocks in the freeBlockList.\n");
	}
	if (is_correct)
	{
		eval += 15;
	}

	//====================================================================//
	/*free_block Scenario 4: Merge with next ONLY (AT the head)*/
	cprintf("	5: Free some allocated blocks [Merge with next ONLY]\n\n") ;
	cprintf("		5.1: at the head\n\n") ;
	is_correct = 1;
	blockIndex = 0 ;
	free_block(startVAs[blockIndex]);
	block_size = get_block_size(startVAs[blockIndex]) ;
	expected_size = allocSizes[0]+allocSizes[0];
	if (block_size != expected_size)
	{
		is_correct = 0;
		cprintf("test_free_block #10.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",allocSizes[0] + allocSizes[0],block_size);
	}
	block_status = is_free_block(startVAs[blockIndex]) ;
	if (block_status != 1)
	{
		is_correct = 0;
		cprintf("test_free_block #10.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
	}
	blk_header = (uint32*)((uint32)startVAs[blockIndex] - sizeof(int));
	blk_footer = (uint32*)((uint32)startVAs[blockIndex]+expected_size-2*sizeof(int));
	if (*(blk_header) != *(blk_footer))
	{
		is_correct = 0;
		cprintf("test_free_block #10.3: WRONG FREE! make sure to set block header and footer with the same values.\n");
	}


	//====================================================================//
	/*free_block Scenario 5: Merge with next ONLY (between 2 blocks)*/
	cprintf("		5.2: between 2 blocks\n\n") ;
	blockIndex = 1*allocCntPerSize - 1 ;
	free_block(startVAs[blockIndex]);
	block_size = get_block_size(startVAs[blockIndex]) ;
	expected_size = allocSizes[0]+allocSizes[1];
	if (block_size != expected_size)
	{
		is_correct = 0;
		cprintf("test_free_block #11.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",allocSizes[0] + allocSizes[1],block_size);
	}
	block_status = is_free_block(startVAs[blockIndex]) ;
	if (block_status != 1)
	{
		is_correct = 0;
		cprintf("test_free_block #11.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
	}
	blk_header = (uint32*)((uint32)startVAs[blockIndex] - sizeof(int));
	blk_footer = (uint32*)((uint32)startVAs[blockIndex]+expected_size-2*sizeof(int));
	if (*(blk_header) != *(blk_footer))
	{
		is_correct = 0;
		cprintf("test_free_block #11.3: WRONG FREE! make sure to set block header and footer with the same values.\n");
	}

	is_correct = 1;
	if (LIST_SIZE(&freeBlocksList) != expectedNumOfFreeBlks)
	{
		is_correct = 0;
		cprintf("test_free_block #11.4: WRONG number of freed blocks in the freeBlockList.\n");
	}
	if (is_correct)
	{
		eval += 15;
	}

	//====================================================================//
	/*free_block Scenario 6: Merge with prev & next */
	cprintf("	6: Free some allocated blocks [Merge with previous & next]\n\n") ;
	is_correct = 1;
	blockIndex = 4*allocCntPerSize - 2 ;
	free_block(startVAs[blockIndex]);	//no merge
	expectedNumOfFreeBlks++;

	blockIndex = 4*allocCntPerSize - 1 ;
	free_block(startVAs[blockIndex]);	//merge with prev & next
	expectedNumOfFreeBlks--;

	block_size = get_block_size(startVAs[blockIndex-1]) ;
	expected_size = allocSizes[3]+allocSizes[3]+allocSizes[4];
	if (block_size != expected_size)
	{
		is_correct = 0;
		cprintf("test_free_block #12.1: WRONG FREE! block size after free is not correct. Expected %d, Actual %d\n",allocSizes[3]+allocSizes[3]+allocSizes[4],block_size);
	}
	block_status = is_free_block(startVAs[blockIndex-1]) ;
	if (block_status != 1)
	{
		is_correct = 0;
		cprintf("test_free_block #12.2: WRONG FREE! block status (is_free) not equal 1 after freeing.\n");
	}
	blk_header = (uint32*)((uint32)startVAs[blockIndex-1] - sizeof(int));
	blk_footer = (uint32*)((uint32)startVAs[blockIndex-1]+expected_size-2*sizeof(int));
	if (*(blk_header) != *(blk_footer))
	{
		is_correct = 0;
		cprintf("test_free_block #12.3: WRONG FREE! make sure to set block header and footer with the same values.\n");
	}

	is_correct = 1;
	if (LIST_SIZE(&freeBlocksList) != expectedNumOfFreeBlks)
	{
		is_correct = 0;
		cprintf("test_free_block #12.4: WRONG number of freed blocks in the freeBlockList. expected = %d, actual = %d\n", expectedNumOfFreeBlks, LIST_SIZE(&freeBlocksList));
	}
	if (is_correct)
	{
		eval += 20;
	}

	//====================================================================//
	/*Allocate After Free Scenarios */
	cprintf("	7: Allocate After Free [should be placed in coalesced blocks]\n\n") ;

	cprintf("		7.1: in block coalesces with NEXT\n\n") ;
	is_correct = 1;
	actualSize = 5*kilo - sizeOfMetaData;
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	void* expected = (void*)(KERNEL_HEAP_START + sizeof(int) + sizeOfMetaData/2);
	if(va == NULL || (va != expected))
	{
		is_correct = 0;
		cprintf("test_free_block #13.1: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
	}
	actualSize = 3*kilo - sizeOfMetaData;
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected = (void*)(KERNEL_HEAP_START + sizeof(int) + 5*kilo + sizeOfMetaData/2);
	if(va == NULL || (va != expected))
	{
		is_correct = 0;
		cprintf("test_free_block #13.2: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
	}
	actualSize = 4*kilo + 10;
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected = startVAs[1*allocCntPerSize - 1];
	if(va == NULL || (va != expected))
	{
		is_correct = 0;
		cprintf("test_free_block #13.3: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
	}
	if (is_correct)
	{
		eval += 10;
	}

	cprintf("		7.2: in block coalesces with PREV & NEXT\n\n") ;
	is_correct = 1;
	actualSize = 2*kilo + 1;
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected = startVAs[4*allocCntPerSize - 2];
	if(va == NULL || (va != expected))
	{
		is_correct = 0;
		cprintf("test_free_block #13.4: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
	}
	if (is_correct)
	{
		eval += 10;
	}

	cprintf("		7.3: in block coalesces with PREV\n\n") ;
	is_correct = 1;
	actualSize = 2*kilo - sizeOfMetaData;
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected = startVAs[2*allocCntPerSize];
	if(va == NULL || (va != expected))
	{
		is_correct = 0;
		cprintf("test_free_block #13.5: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
	}
	actualSize = 8*kilo - sizeOfMetaData;
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected = startVAs[numOfAllocs*allocCntPerSize-1];
	if(va == NULL || (va != expected))
	{
		is_correct = 0;
		cprintf("test_free_block #13.6: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x\n", expected, va);
	}
	if (is_correct)
	{
		eval += 10;
	}

	cprintf("test free_block with FIRST FIT completed. Evaluation = %d%\n", eval);

}

void test_free_block_BF()
{
#if USE_KHEAP
	panic("test_free_block: the kernel heap should be disabled. make sure USE_KHEAP = 0");
	return;
#endif

	cprintf("===========================================================\n") ;
	cprintf("NOTE: THIS TEST IS DEPEND ON BOTH ALLOCATE & FREE FUNCTIONS\n") ;
	cprintf("===========================================================\n") ;

	panic("Test is under construction! will be announced later isA");

}

void test_free_block_NF()
{
	panic("not implemented");
}

void test_realloc_block_FF()
{
#if USE_KHEAP
	panic("test_free_block: the kernel heap should be disabled. make sure USE_KHEAP = 0");
	return;
#endif

	cprintf("===================================================\n");
	cprintf("*****NOTE: THIS IS A PARTIAL TEST FOR REALLOC******\n") ;
	cprintf("You need to pick-up the missing tests and test them\n") ;
	cprintf("===================================================\n");

	//TODO: [PROJECT'24.MS1 - #09] [3] DYNAMIC ALLOCATOR - test_realloc_block_FF()
	//CHECK MISSING CASES AND TRY TO TEST THEM !

	panic("Test is under construction! will be announced later isA");

}


void test_realloc_block_FF_COMPLETE()
{
#if USE_KHEAP
	panic("test_free_block: the kernel heap should be disabled. make sure USE_KHEAP = 0");
	return;
#endif

	panic("this is unseen test");

}


/********************Helper Functions***************************/
