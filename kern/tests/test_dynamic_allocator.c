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

/***********************************************************************************************************************/
#define Mega  (1024*1024)
#define kilo (1024)
#define numOfAllocs 7
#define allocCntPerSize 200
#define sizeOfMetaData 8

//NOTE: these sizes include the size of MetaData within it
uint32 allocSizes[numOfAllocs] = {4*kilo, 20*sizeof(char) + sizeOfMetaData, 1*kilo, 3*sizeof(int) + sizeOfMetaData, 2*kilo, 2*sizeOfMetaData, 7*kilo} ;
short* startVAs[numOfAllocs*allocCntPerSize+1] ;
short* midVAs[numOfAllocs*allocCntPerSize+1] ;
short* endVAs[numOfAllocs*allocCntPerSize+1] ;

int check_block(void* va, void* expectedVA, uint32 expectedSize, uint8 expectedFlag)
{
	//Check returned va
	if(va != expectedVA)
	{
		cprintf("wrong block address. Expected %x, Actual %x\n", expectedVA, va);
		return 0;
	}
	//Check header & footer
	uint32 header = *((uint32*)va-1);
	uint32 footer = *((uint32*)(va + expectedSize - 8));
	uint32 expectedData = expectedSize | expectedFlag ;
	if(header != expectedData || footer != expectedData)
	{
		cprintf("wrong header/footer data. Expected %d, Actual H:%d F:%d\n", expectedData, header, footer);
		return 0;
	}
	return 1;
}
int check_list_size(uint32 expectedListSize)
{
	if (LIST_SIZE(&freeBlocksList) != expectedListSize)
	{
		cprintf("freeBlocksList: wrong size! expected %d, actual %d\n", expectedListSize, LIST_SIZE(&freeBlocksList));
		return 0;
	}
	return 1;
}
/***********************************************************************************************************************/

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
	void* expectedVA;
	uint32 expectedNumFreeBlks;
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
			//Check block
			expectedVA = (curVA + sizeOfMetaData/2);
			if (check_block(va, expectedVA, allocSizes[i], 1) == 0)
			{
				is_correct = 0;
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
		eval += 15;
	}
	if (check_list_size(1))
	{
		eval += 5;
	}
	//====================================================================//
	/*INITIAL ALLOC Scenario 3: Try to allocate a block with a size equal to the size of the first existing free block*/
	cprintf("	3: Try to allocate a block with equal to the first existing free block\n\n") ;
	is_correct = 1;

	actualSize = remainSize - sizeOfMetaData;
	va = startVAs[idx] = alloc_block(actualSize, ALLOC_STRATEGY);
	midVAs[idx] = va + actualSize/2 ;
	endVAs[idx] = va + actualSize - sizeof(short);
	//Check block
	expectedVA = (curVA + sizeOfMetaData/2);

	if (is_correct) is_correct = check_block(va, expectedVA, remainSize, 1) ;
	if (is_correct) is_correct = check_list_size(0);

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
	//Check number of freed blocks
	is_correct = check_list_size(numOfAllocs);
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
	void* expectedVA;
	uint32 expectedNumFreeBlks;
#define numOfFFTests 3
	uint32 startVA = KERNEL_HEAP_START + sizeof(int); //just after the DA Begin block
	uint32 testSizes[numOfFFTests] = {1*kilo + kilo/2, 3*kilo, kilo/2} ;
	uint32 startOf1st7KB = (uint32)startVAs[6*allocCntPerSize];
	uint32 expectedVAs[numOfFFTests] = {startVA + sizeOfMetaData/2, startOf1st7KB, startVA + testSizes[0] + sizeOfMetaData/2};
	short* tstStartVAs[numOfFFTests+2] ;
	short* tstMidVAs[numOfFFTests+2] ;
	short* tstEndVAs[numOfFFTests+2] ;
	for (int i = 0; i < numOfFFTests; ++i)
	{
		actualSize = testSizes[i] - sizeOfMetaData;
		va = tstStartVAs[i] = alloc_block(actualSize, DA_FF);
		tstMidVAs[i] = va + actualSize/2 ;
		tstEndVAs[i] = va + actualSize - sizeof(short);
		//Check block
		cprintf("test#%d\n",i);
		expectedVA = (void*)expectedVAs[i];
		if (check_block(va, expectedVA, testSizes[i], 1) == 0)
		{
			is_correct = 0;
		}
		*(tstStartVAs[i]) = 353;
		*(tstMidVAs[i]) = 353;
		*(tstEndVAs[i]) = 353;
	}
	if (is_correct) is_correct = check_list_size(numOfAllocs);
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
	//Check block
	expectedVA = (void*)(startVA + testSizes[0] + testSizes[2] + sizeOfMetaData/2) ;

	if (is_correct) is_correct = check_block(va, expectedVA, 2*kilo, 1);
	if (is_correct) is_correct = check_list_size(numOfAllocs - 1);

	*(tstStartVAs[numOfFFTests]) = 353 ;
	*(tstMidVAs[numOfFFTests]) = 353 ;
	*(tstEndVAs[numOfFFTests]) = 353 ;

	if (is_correct)
	{
		eval += 10;
	}
	//====================================================================//
	/*FF ALLOC Scenario 4: Try to allocate a block with a bit smaller size [internal fragmentation case]*/
	cprintf("	4: Try to allocate a block with a bit smaller size [internal fragmentation case]\n\n") ;
	is_correct = 1;

	actualSize = allocSizes[1] - sizeOfMetaData - 10;
	va = tstStartVAs[numOfFFTests+1] = alloc_block(actualSize, DA_FF);
	tstMidVAs[numOfFFTests+1] = va + actualSize/2 ;
	tstEndVAs[numOfFFTests+1] = va + actualSize - sizeof(short);
	//Check block
	expectedVA = startVAs[1*allocCntPerSize];

	if (is_correct) is_correct = check_block(va, expectedVA, allocSizes[1], 1);
	if (is_correct) is_correct = check_list_size(numOfAllocs - 2);

	*(tstStartVAs[numOfFFTests+1]) = 353 ;
	*(tstMidVAs[numOfFFTests+1]) = 353 ;
	*(tstEndVAs[numOfFFTests+1]) = 353 ;

	if (is_correct)
	{
		eval += 10;
	}
	//====================================================================//
	/*FF ALLOC Scenario 5: Check stored data inside each allocated block*/
	cprintf("	5: Check stored data inside each allocated block\n\n") ;
	is_correct = 1;

	for (int i = 0; i < numOfFFTests + 2; ++i)
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
		eval += 10;
	}
	cprintf("test alloc_block_FF completed. Evaluation = %d%\n", eval);
}

void test_alloc_block_BF()
{
#if USE_KHEAP
	panic("test_alloc_block_BF: the kernel heap should be disabled. make sure USE_KHEAP = 0");
	return;
#endif

	int eval = 0;
	bool is_correct;
	void* va = NULL;
	uint32 actualSize = 0;

	cprintf("=======================================================\n") ;
	cprintf("FIRST: Tests depend on the Allocate Function ONLY [40%]\n") ;
	cprintf("=======================================================\n") ;
	eval = test_initial_alloc(DA_BF);

	cprintf("====================================================\n") ;
	cprintf("SECOND: Tests depend on BOTH Allocate and Free [60%] \n") ;
	cprintf("====================================================\n") ;
	void* expectedVA;
	uint32 expectedNumFreeBlks;
	//Free set of blocks with different sizes (first block of each size)
	for (int i = 0; i < numOfAllocs; ++i)
	{
		free_block(startVAs[i*allocCntPerSize]);
	}
	//Check number of freed blocks
	is_correct = check_list_size(numOfAllocs);
	if (is_correct)
	{
		eval += 10;
	}
	//====================================================================//
	/*BF ALLOC Scenario 1: Try to allocate a block with a size greater than the size of any existing free block*/
	cprintf("	1: Try to allocate large block [not fit in any space]\n\n") ;
	is_correct = 1;

	uint32 maxSize = 0 ;
	for (int i = 0; i < numOfAllocs; ++i)
	{
		if (allocSizes[i] > maxSize)
			maxSize = allocSizes[i] ;
	}
	va = alloc_block(maxSize, DA_BF);

	//Check returned va
	if(va != NULL)
	{
		is_correct = 0;
		cprintf("alloc_block_BF #5: WRONG BF ALLOC - alloc_block_BF find a block instead no existing free blocks with the given size.\n");
	}
	if (is_correct)
	{
		eval += 5;
	}
	//====================================================================//
	/*BF ALLOC Scenario 2: Try to allocate blocks with sizes smaller than existing free blocks*/
	cprintf("	2: Try to allocate set of blocks with different sizes smaller than existing free blocks\n\n") ;
	is_correct = 1;

#define numOfBFTests 5
	uint32 testSizes[numOfBFTests] = {
			/*only 1 can fit*/4*kilo + kilo/2,
			/*many can fit*/ 1*kilo + kilo/4,
			/*many can fit*/kilo/2,
			/*many can fit*/kilo/2,
			/*only 1 can fit (@head)*/3*kilo } ;
	uint32 startOf1st1KB = (uint32)startVAs[2*allocCntPerSize];
	uint32 startOf1st2KB = (uint32)startVAs[4*allocCntPerSize];
	uint32 startOf1st7KB = (uint32)startVAs[6*allocCntPerSize];

	uint32 expectedVAs[numOfBFTests] = {startOf1st7KB, startOf1st2KB, startOf1st2KB + testSizes[1],startOf1st1KB, KERNEL_HEAP_START + 2*sizeof(int)};
	short* tstStartVAs[numOfBFTests+2] ;
	short* tstMidVAs[numOfBFTests+2] ;
	short* tstEndVAs[numOfBFTests+2] ;
	for (int i = 0; i < numOfBFTests; ++i)
	{
		actualSize = testSizes[i] - sizeOfMetaData;
		va = tstStartVAs[i] = alloc_block(actualSize, DA_BF);
		tstMidVAs[i] = va + actualSize/2 ;
		tstEndVAs[i] = va + actualSize - sizeof(short);

		//Check block
		cprintf("test#%d\n",i);
		expectedVA = (void*)expectedVAs[i];
		if (check_block(va, expectedVA, testSizes[i], 1) == 0)
		{
			is_correct = 0;
		}
		*(tstStartVAs[i]) = 353;
		*(tstMidVAs[i]) = 353;
		*(tstEndVAs[i]) = 353;
	}

	if (is_correct) is_correct = check_list_size(numOfAllocs);
	if (is_correct)
	{
		eval += 15;
	}
	//====================================================================//
	/*BF ALLOC Scenario 3: Try to allocate a block with a size equal to the size of an existing free block*/
	cprintf("	3: Try to allocate a block with equal to an existing free block\n\n") ;
	is_correct = 1;

	actualSize = kilo/4 - sizeOfMetaData;
	va = tstStartVAs[numOfBFTests] = alloc_block(actualSize, DA_BF);
	tstMidVAs[numOfBFTests] = va + actualSize/2 ;
	tstEndVAs[numOfBFTests] = va + actualSize - sizeof(short);
	//Check returned va
	expectedVA = (void*)(startOf1st2KB + testSizes[1] + testSizes[3]) ;
	if (is_correct) is_correct = check_block(va, expectedVA, kilo/4, 1);
	if (is_correct) is_correct = check_list_size(numOfAllocs-1);

	*(tstStartVAs[numOfBFTests]) = 353 ;
	*(tstMidVAs[numOfBFTests]) = 353 ;
	*(tstEndVAs[numOfBFTests]) = 353 ;

	if (is_correct)
	{
		eval += 10;
	}
	//====================================================================//
	/*FF ALLOC Scenario 4: Try to allocate a block with a bit smaller size [internal fragmentation case]*/
	cprintf("	4: Try to allocate a block with a bit smaller size [internal fragmentation case]\n\n") ;
	is_correct = 1;

	actualSize = allocSizes[5] - sizeOfMetaData - 2;
	va = tstStartVAs[numOfBFTests+1] = alloc_block(actualSize, DA_BF);
	tstMidVAs[numOfBFTests+1] = va + 2 ;
	tstEndVAs[numOfBFTests+1] = va + actualSize - sizeof(short);
	//Check block
	expectedVA = startVAs[5*allocCntPerSize];

	if (is_correct) is_correct = check_block(va, expectedVA, allocSizes[5], 1);
	if (is_correct) is_correct = check_list_size(numOfAllocs - 2);

	*(tstStartVAs[numOfBFTests+1]) = 353 ;
	*(tstMidVAs[numOfBFTests+1]) = 353 ;
	*(tstEndVAs[numOfBFTests+1]) = 353 ;

	if (is_correct)
	{
		eval += 10;
	}
	//====================================================================//
	/*BF ALLOC Scenario 5: Check stored data inside each allocated block*/
	cprintf("	5: Check stored data inside each allocated block\n\n") ;
	is_correct = 1;

	for (int i = 0; i < numOfBFTests+2; ++i)
	{
		//cprintf("startVA = %x, mid = %x, last = %x\n", tstStartVAs[i], tstMidVAs[i], tstEndVAs[i]);
		if (*(tstStartVAs[i]) != 353 || *(tstMidVAs[i]) != 353 || *(tstEndVAs[i]) != 353)
		{
			//cprintf("start VA = %x, mid VA = %x, end VA = %x\n", tstStartVAs[i], tstMidVAs[i], tstEndVAs[i]);
			is_correct = 0;
			cprintf("alloc_block_BF #8.%d: WRONG! content of the block is not correct. Expected=%d, val1=%d, val2=%d, val3=%d\n",i, 353, *(tstStartVAs[i]), *(tstMidVAs[i]), *(tstEndVAs[i]));
			break;
		}
	}

	if (is_correct)
	{
		eval += 10;
	}
	cprintf("test alloc_block_BF completed. Evaluation = %d%\n", eval);
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
	void*expected_va ;

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
			expected_va = curVA + sizeOfMetaData/2;
			if (check_block(va, expected_va, allocSizes[i], 1) == 0)
				//			if(va != (curVA + sizeOfMetaData/2))
				panic("test_free_block #1.%d: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x", idx, expected_va ,va);
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
	expected_va = curVA + sizeOfMetaData/2;
	if (check_block(va, expected_va, remainSize, 1) == 0)
		//			if(va != (curVA + sizeOfMetaData/2))
		panic("test_free_block #2: WRONG ALLOC - alloc_block_FF return wrong address. Expected %x, Actual %x",  expected_va ,va);
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
	uint32 block_size, block_status, expected_size, *blk_header, *blk_footer;
	is_correct = 1;

	//Free set of blocks with different sizes (first block of each size)
	for (int i = 0; i < numOfAllocs; ++i)
	{
		cprintf("test#%d\n",i);
		free_block(startVAs[i*allocCntPerSize]);
		if (check_block(startVAs[i*allocCntPerSize], startVAs[i*allocCntPerSize], allocSizes[i], 0) == 0)
		{
			is_correct = 0;
		}
	}
	uint32 expectedNumOfFreeBlks = numOfAllocs;
	if (is_correct) is_correct = check_list_size(expectedNumOfFreeBlks);
	if (is_correct)
	{
		eval += 10;
	}

	is_correct = 1;
	//Free last block
	free_block(startVAs[numOfAllocs*allocCntPerSize]);
	if (is_correct) is_correct = check_block(startVAs[numOfAllocs*allocCntPerSize], startVAs[numOfAllocs*allocCntPerSize], remainSize, 0);

	//Reallocate last block
	actualSize = remainSize - sizeOfMetaData;
	va = alloc_block(actualSize, DA_FF);
	//Check block
	expected_va = (curVA + sizeOfMetaData/2);
	if (is_correct) is_correct = check_block(va, expected_va, remainSize, 1);

	//Free block before last
	free_block(startVAs[numOfAllocs*allocCntPerSize - 1]);
	if (is_correct) is_correct = check_block(startVAs[numOfAllocs*allocCntPerSize-1], startVAs[numOfAllocs*allocCntPerSize-1], allocSizes[numOfAllocs-1], 0);

	//Reallocate first block
	actualSize = allocSizes[0] - sizeOfMetaData;
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected_va = (void*)(KERNEL_HEAP_START + sizeof(int) + sizeOfMetaData/2);
	if (is_correct) is_correct = check_block(va, expected_va, allocSizes[0], 1);

	//Free 2nd block
	free_block(startVAs[1]);
	if (is_correct) is_correct = check_block(startVAs[1], startVAs[1], allocSizes[0], 0);

	expectedNumOfFreeBlks++ ;
	if (is_correct) is_correct = check_list_size(expectedNumOfFreeBlks);
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
	expected_size = remainSize + allocSizes[numOfAllocs-1];
	if (check_block(startVAs[blockIndex-1], startVAs[blockIndex-1], expected_size, 0) == 0)
	{
		is_correct = 0;
	}
	//====================================================================//
	/*free_block Scenario 3: Merge with previous ONLY (between 2 blocks)*/
	cprintf("		4.2: between 2 blocks\n\n") ;
	blockIndex = 2*allocCntPerSize+1 ;
	free_block(startVAs[blockIndex]);
	expected_size = allocSizes[2]+allocSizes[2];
	if (check_block(startVAs[blockIndex-1], startVAs[blockIndex-1], expected_size, 0) == 0)
	{
		is_correct = 0;
	}
	if (check_list_size(expectedNumOfFreeBlks) == 0)
	{
		is_correct = 0;
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
	expected_size = allocSizes[0]+allocSizes[0];
	if (check_block(startVAs[blockIndex], startVAs[blockIndex], expected_size, 0) == 0)
	{
		is_correct = 0;
	}

	//====================================================================//
	/*free_block Scenario 5: Merge with next ONLY (between 2 blocks)*/
	cprintf("		5.2: between 2 blocks\n\n") ;
	blockIndex = 1*allocCntPerSize - 1 ;
	free_block(startVAs[blockIndex]);
	block_size = get_block_size(startVAs[blockIndex]) ;
	expected_size = allocSizes[0]+allocSizes[1];
	if (check_block(startVAs[blockIndex], startVAs[blockIndex], expected_size, 0) == 0)
	{
		is_correct = 0;
	}

	if (is_correct) is_correct = check_list_size(expectedNumOfFreeBlks);
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
	if (check_block(startVAs[blockIndex-1], startVAs[blockIndex-1], expected_size, 0) == 0)
	{
		is_correct = 0;
	}

	if (is_correct) is_correct = check_list_size(expectedNumOfFreeBlks);

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
	expected_size = ROUNDUP(actualSize + sizeOfMetaData, 2);
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	void* expected = (void*)(KERNEL_HEAP_START + sizeof(int) + sizeOfMetaData/2);
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		is_correct = 0;
		cprintf("test_free_block #7.1: Failed\n");
	}
	actualSize = 3*kilo - sizeOfMetaData;
	expected_size = ROUNDUP(actualSize + sizeOfMetaData, 2);
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected = (void*)(KERNEL_HEAP_START + sizeof(int) + 5*kilo + sizeOfMetaData/2);
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		is_correct = 0;
		cprintf("test_free_block #7.2: Failed\n");
	}

	expectedNumOfFreeBlks--;

	/*INTERNAL FRAGMENTATION CASE*/
	actualSize = 4*kilo + 10 ;
	expected_size = MAX(ROUNDUP(actualSize + sizeOfMetaData, 2), allocSizes[0]+allocSizes[1]) ;
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected = startVAs[1*allocCntPerSize - 1];
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		is_correct = 0;
		cprintf("test_free_block #7.3: Failed INTERNAL FRAGMENTATION CASE\n");
	}
	if (is_correct)
	{
		eval += 10;
	}

	expectedNumOfFreeBlks--;

	cprintf("		7.2: in block coalesces with PREV & NEXT\n\n") ;
	is_correct = 1;
	actualSize = 2*kilo + 1;
	expected_size = ROUNDUP(actualSize + sizeOfMetaData, 2);
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected = startVAs[4*allocCntPerSize - 2];
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		is_correct = 0;
		cprintf("test_free_block #7.4: Failed\n");
	}
	if (is_correct)
	{
		eval += 10;
	}

	cprintf("		7.3: in block coalesces with PREV\n\n") ;
	is_correct = 1;
	actualSize = 2*kilo - sizeOfMetaData;
	expected_size = ROUNDUP(actualSize + sizeOfMetaData, 2);
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected = startVAs[2*allocCntPerSize];
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		is_correct = 0;
		cprintf("test_free_block #7.5: Failed\n");
	}

	expectedNumOfFreeBlks--;

	actualSize = 8*kilo - sizeOfMetaData;
	expected_size = ROUNDUP(actualSize + sizeOfMetaData, 2);
	va = alloc_block(actualSize, DA_FF);
	//Check returned va
	expected = startVAs[numOfAllocs*allocCntPerSize-1];
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		is_correct = 0;
		cprintf("test_free_block #7.6: Failed\n");
	}

	if (is_correct) is_correct = check_list_size(expectedNumOfFreeBlks);

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

	uint32 actualSize, expected_size;
	void* expected_va;
	for (int i = 0; i < numOfAllocs; ++i)
	{
		for (int j = 0; j < allocCntPerSize; ++j)
		{
			actualSize = allocSizes[i] - sizeOfMetaData;
			va = startVAs[idx] = alloc_block(actualSize, DA_BF);
			midVAs[idx] = va + actualSize/2 ;
			endVAs[idx] = va + actualSize - sizeof(short);
			//Check returned va
			expected_va = curVA + sizeOfMetaData/2;
			if (check_block(va, expected_va, allocSizes[i], 1) == 0)
				panic("test_free_block #1.%d: WRONG ALLOC - alloc_block_BF return wrong address. Expected %x, Actual %x", idx, curVA + sizeOfMetaData ,va);
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
	va = startVAs[idx] = alloc_block(actualSize, DA_BF);
	midVAs[idx] = va + actualSize/2 ;
	endVAs[idx] = va + actualSize - sizeof(short);
	//Check returned va
	expected_va = curVA + sizeOfMetaData/2;
	if (check_block(va, expected_va, remainSize, 1) == 0)
		panic("test_free_block #1: WRONG ALLOC - alloc_block_BF return wrong address.");
	*(startVAs[idx]) = idx ;
	*(midVAs[idx]) = idx ;
	*(endVAs[idx]) = idx ;

	//====================================================================//
	/* Check stored data inside each allocated block*/
	cprintf("	2: Check stored data inside each allocated block\n\n") ;

	for (int i = 0; i < idx; ++i)
	{
		if (*(startVAs[i]) != i || *(midVAs[i]) != i ||	*(endVAs[i]) != i)
			panic("test_free_block #2.%d: WRONG! content of the block is not correct. Expected %d",i, i);
	}

	//====================================================================//
	/* free_block Scenario 1: Free some allocated blocks [no coalesce]*/
	cprintf("	3: Free some allocated block [no coalesce]\n\n") ;

	//Free set of blocks with different sizes (first block of each size)
	for (int i = 0; i < numOfAllocs; ++i)
	{
		free_block(startVAs[i*allocCntPerSize]);
		if (check_block(startVAs[i*allocCntPerSize], startVAs[i*allocCntPerSize], allocSizes[i], 0) == 0)
		{
			panic("3.1 Failed");
		}
	}
	uint32 expectedNumOfFreeBlks = numOfAllocs;
	if (check_list_size(expectedNumOfFreeBlks) == 0)
	{
		panic("3.2 Failed");
	}

	//Free last block
	free_block(startVAs[numOfAllocs*allocCntPerSize]);
	if (check_block(startVAs[numOfAllocs*allocCntPerSize], startVAs[numOfAllocs*allocCntPerSize], remainSize, 0) == 0)
		panic("3.3 Failed");

	//Reallocate last block
	actualSize = remainSize - sizeOfMetaData;
	va = alloc_block(actualSize, DA_BF);
	//Check returned va
	expected_va = (curVA + sizeOfMetaData/2);
	if(check_block(va, expected_va, remainSize, 1) == 0)
		panic("3.4 Failed");

	//Free block before last
	free_block(startVAs[numOfAllocs*allocCntPerSize - 1]);
	if (check_block(startVAs[numOfAllocs*allocCntPerSize-1], startVAs[numOfAllocs*allocCntPerSize-1], allocSizes[numOfAllocs-1], 0) == 0)
		panic("3.5 Failed");

	//Reallocate first block
	actualSize = allocSizes[0] - sizeOfMetaData;
	va = alloc_block(actualSize, DA_BF);
	//Check returned va
	expected_va = (void*)(KERNEL_HEAP_START + 2*sizeof(int));
	if(check_block(va, expected_va, allocSizes[0], 1) == 0)
		panic("3.6 Failed");

	//Free 2nd block
	free_block(startVAs[1]);
	if (check_block(startVAs[1], startVAs[1], allocSizes[0], 0) == 0)
		panic("3.7 Failed");

	expectedNumOfFreeBlks++ ;
	if (check_list_size(expectedNumOfFreeBlks) == 0)
	{
		panic("3.8 Failed");
	}

	uint32 block_size, block_status;
	//====================================================================//
	/*free_block Scenario 2: Merge with previous ONLY (AT the tail)*/
	cprintf("	4: Free some allocated blocks [Merge with previous ONLY]\n\n") ;
	cprintf("		4.1: at the tail\n\n") ;
	//Free last block (coalesce with previous)
	uint32 blockIndex = numOfAllocs*allocCntPerSize;
	free_block(startVAs[blockIndex]);
	expected_size = remainSize + allocSizes[numOfAllocs-1];
	if (check_block(startVAs[blockIndex-1], startVAs[blockIndex-1], expected_size, 0) == 0)
	{
		panic("4.1 Failed");
	}
	//====================================================================//
	/*free_block Scenario 3: Merge with previous ONLY (between 2 blocks)*/
	cprintf("		4.2: between 2 blocks\n\n") ;
	blockIndex = 2*allocCntPerSize+1 ;
	free_block(startVAs[blockIndex]);
	expected_size = allocSizes[2]+allocSizes[2];
	if (check_block(startVAs[blockIndex-1], startVAs[blockIndex-1], expected_size, 0) == 0)
	{
		panic("4.2 Failed");
	}
	//====================================================================//
	/*free_block Scenario 4: Merge with next ONLY (AT the head)*/
	cprintf("	5: Free some allocated blocks [Merge with next ONLY]\n\n") ;
	cprintf("		5.1: at the head\n\n") ;
	blockIndex = 0 ;
	free_block(startVAs[blockIndex]);
	expected_size = allocSizes[0]+allocSizes[0];
	if (check_block(startVAs[blockIndex], startVAs[blockIndex], expected_size, 0) == 0)
	{
		panic("5.1 Failed");
	}
	//====================================================================//
	/*free_block Scenario 5: Merge with next ONLY (between 2 blocks)*/
	cprintf("		5.2: between 2 blocks\n\n") ;
	blockIndex = 1*allocCntPerSize - 1 ;
	free_block(startVAs[blockIndex]);
	expected_size = allocSizes[0]+allocSizes[1];
	if (check_block(startVAs[blockIndex], startVAs[blockIndex], expected_size, 0) == 0)
	{
		panic("5.2 Failed");
	}
	//====================================================================//
	/*free_block Scenario 6: Merge with prev & next */
	cprintf("	6: Free some allocated blocks [Merge with previous & next]\n\n") ;
	blockIndex = 4*allocCntPerSize - 2 ;
	free_block(startVAs[blockIndex]);

	blockIndex = 4*allocCntPerSize - 1 ;
	free_block(startVAs[blockIndex]);
	expected_size = allocSizes[3]+allocSizes[3]+allocSizes[4];
	if (check_block(startVAs[blockIndex-1], startVAs[blockIndex-1], expected_size, 0) == 0)
	{
		panic("6.1 Failed");
	}
	if (check_list_size(expectedNumOfFreeBlks) == 0)
	{
		panic("6.2 Failed");
	}
	//====================================================================//
	/*Allocate After Free Scenarios */
	void* expected = NULL;
	{
		//Consume 1st 7KB Block
		actualSize = 7*kilo - sizeOfMetaData ;
		expected_size = ROUNDUP(actualSize + sizeOfMetaData,2) ;
		va = alloc_block(actualSize, DA_BF);
		//Check returned va
		expected = (void*)(startVAs[6*allocCntPerSize]);
		if (check_block(va, expected, expected_size, 1) == 0)
		{
			panic("6.3 Failed");
		}
		expectedNumOfFreeBlks--;
	}

	cprintf("	7: Allocate After Free [should be placed in coalesced blocks]\n\n") ;

	cprintf("		7.1: in block coalesces with PREV\n\n") ;
	actualSize = 2*kilo - sizeOfMetaData;
	expected_size = ROUNDUP(actualSize + sizeOfMetaData,2) ;
	va = alloc_block(actualSize, DA_BF);
	//Check returned va
	expected = startVAs[2*allocCntPerSize];
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		panic("7.1 Failed");
	}

	expectedNumOfFreeBlks--;

	actualSize = 8*kilo;
	expected_size = ROUNDUP(actualSize + sizeOfMetaData,2) ;
	va = alloc_block(actualSize, DA_BF);
	//Check returned va
	expected = startVAs[numOfAllocs*allocCntPerSize-1];
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		panic("7.2 Failed");
	}

	cprintf("		7.2: in block coalesces with PREV & NEXT\n\n") ;
	actualSize = 2*kilo + 1;
	expected_size = ROUNDUP(actualSize + sizeOfMetaData,2) ;
	va = alloc_block(actualSize, DA_BF);
	//Check returned va
	expected = startVAs[4*allocCntPerSize - 2];
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		panic("7.3 Failed");
	}

	cprintf("		7.3: in block coalesces with NEXT [INTERNAL FRAGMENTATION]\n\n") ;
	actualSize = 4*kilo + 10;
	expected_size = allocSizes[0]+allocSizes[1]; //ROUNDUP(actualSize + sizeOfMetaData,2) ;
	va = alloc_block(actualSize, DA_BF);
	//Check returned va
	expected = startVAs[1*allocCntPerSize - 1];
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		panic("7.4 Failed");
	}
	expectedNumOfFreeBlks--;

	actualSize = 5*kilo - sizeOfMetaData;
	expected_size = ROUNDUP(actualSize + sizeOfMetaData,2) ;
	va = alloc_block(actualSize, DA_BF);
	//Check returned va
	expected = (void*)(KERNEL_HEAP_START + sizeOfMetaData);
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		panic("7.5 Failed");
	}

	actualSize = 3*kilo - sizeOfMetaData;
	expected_size = ROUNDUP(actualSize + sizeOfMetaData,2) ;
	va = alloc_block(actualSize, DA_BF);
	//Check returned va
	expected = (void*)(KERNEL_HEAP_START + 5*kilo + sizeOfMetaData);
	if (check_block(va, expected, expected_size, 1) == 0)
	{
		panic("7.6 Failed");
	}
	expectedNumOfFreeBlks--;

	if (check_list_size(expectedNumOfFreeBlks) == 0)
	{
		panic("7.7 Failed");
	}

	cprintf("Congratulations!! test free_block with BEST FIT completed successfully.\n");

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

	//TODO: [PROJECT'24.MS1 - #09] [3] DYNAMIC ALLOCATOR - test_realloc_block_FF()
	//CHECK MISSING CASES AND TRY TO TEST THEM !

	cprintf("===================================================\n");
	cprintf("*****NOTE: THIS IS A PARTIAL TEST FOR REALLOC******\n") ;
	cprintf("You need to pick-up the missing tests and test them\n") ;
	cprintf("===================================================\n");

	int eval = 0;
	bool is_correct;

	int initAllocatedSpace = 3*Mega;
	initialize_dynamic_allocator(KERNEL_HEAP_START, initAllocatedSpace);

	void * va, *expectedVA ;
	//====================================================================//
	//[1] Test calling realloc with VA = NULL. It should call malloc
	//====================================================================//
	/* Try to allocate set of blocks with different sizes*/
	cprintf("1: Test calling realloc with VA = NULL.[10%]\n\n") ;
	is_correct = 1;

	int totalSizes = 0;
	for (int i = 0; i < numOfAllocs; ++i)
	{
		totalSizes += allocSizes[i] * allocCntPerSize ;
	}
	int remainSize = initAllocatedSpace - totalSizes - 2*sizeof(int) ; //exclude size of "DA Begin & End" blocks
	if (remainSize <= 0)
		panic("test_realloc_block_FF is not configured correctly. Consider updating the initial allocated space OR the required allocations");

	int idx = 0;
	void* curVA = (void*) KERNEL_HEAP_START + sizeof(int) /*BEG block*/ ;
	uint32 actualSize, expectedSize;
	for (int i = 0; i < numOfAllocs; ++i)
	{
		for (int j = 0; j < allocCntPerSize; ++j)
		{
			actualSize = allocSizes[i] - sizeOfMetaData;
			expectedSize = ROUNDUP(actualSize + sizeOfMetaData, 2);
			expectedVA = (curVA + sizeOfMetaData/2);
			va = startVAs[idx] = realloc_block_FF(NULL, actualSize);
			midVAs[idx] = va + actualSize/2 ;
			endVAs[idx] = va + actualSize - sizeof(short);
			if (check_block(va, expectedVA, expectedSize, 1) == 0)
			{
				panic("test_realloc_block_FF #1.1.%d: WRONG ALLOC - it return wrong address. Expected %x, Actual %x", idx, curVA + sizeOfMetaData ,va);
			}
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
	expectedSize = ROUNDUP(actualSize + sizeOfMetaData, 2);
	expectedVA = (curVA + sizeOfMetaData/2);

	va = startVAs[idx] = realloc_block_FF(NULL, actualSize);

	midVAs[idx] = va + actualSize/2 ;
	endVAs[idx] = va + actualSize - sizeof(short);
	//Check returned va
	if (check_block(va, expectedVA, expectedSize, 1) == 0)
	{
		panic("test_realloc_block_FF #1.2.0: WRONG ALLOC - it return wrong address.");
	}
	*(startVAs[idx]) = idx ;
	*(midVAs[idx]) = idx ;
	*(endVAs[idx]) = idx ;

	//====================================================================//
	/* Check stored data inside each allocated block*/
	for (int i = 0; i < idx; ++i)
	{
		if (*(startVAs[i]) != i || *(midVAs[i]) != i ||	*(endVAs[i]) != i)
			panic("test_realloc_block_FF #1.3.%d: WRONG! content of the block is not correct. Expected %d",i, i);
	}

	if (is_correct)
	{
		eval += 10;
	}

	//====================================================================//
	//[2] Test krealloc by passing size = 0. It should call free
	//====================================================================//
	cprintf("2: Test calling realloc with SIZE = 0.[10%]\n\n") ;
	is_correct = 1;

	//Free set of blocks with different sizes (first block of each size)
	for (int i = 0; i < numOfAllocs; ++i)
	{
		va = realloc_block_FF(startVAs[i*allocCntPerSize], 0);

		uint32 block_size = get_block_size(startVAs[i*allocCntPerSize]) ;
		expectedSize = allocSizes[i];
		expectedVA = va;
		if (check_block(startVAs[i*allocCntPerSize], startVAs[i*allocCntPerSize], expectedSize, 0) == 0)
		{
			panic("test_realloc_block_FF #2.1.%d: Failed.", i);
		}
		if(va != NULL)
			panic("test_realloc_block_FF #2.2.%d: it should return NULL.", i);
	}

	//test calling it with NULL & ZERO
	va = realloc_block_FF(NULL, 0);
	if(va != NULL)
		panic("test_realloc_block_FF #2.3.0: it should return NULL.");

	//====================================================================//
	/* Check stored data inside each allocated block*/
	for (int i = 0; i < idx; ++i)
	{
		if (i % allocCntPerSize == 0)
			continue;
		if (*(startVAs[i]) != i || *(midVAs[i]) != i ||	*(endVAs[i]) != i)
			panic("test_realloc_block_FF #2.4.%d: WRONG! content of the block is not correct. Expected %d",i, i);
	}

	uint32 expectedNumOfFreeBlks = numOfAllocs;
	if (check_list_size(expectedNumOfFreeBlks) == 0)
	{
		panic("2.5 Failed");
	}

	if (is_correct)
	{
		eval += 10;
	}

	//====================================================================//
	//[3] Test realloc with increased sizes
	//====================================================================//
	cprintf("3: Test calling realloc with increased sizes [50%].\n\n") ;
	int blockIndex, block_size, block_status, old_size, new_size, newBlockIndex;
	//[3.1] reallocate in same place (NO relocate - split)
	cprintf("	3.1: reallocate in same place (NO relocate - split)\n\n") ;
	is_correct = 1;
	{
		blockIndex = 4*allocCntPerSize - 1 ;
		new_size = allocSizes[3] /*12+16 B*/ + allocSizes[4]/2 /*2KB/2*/ - sizeOfMetaData;
		expectedSize = ROUNDUP(new_size + sizeOfMetaData, 2);
		expectedVA = startVAs[blockIndex];

		va = realloc_block_FF(startVAs[blockIndex], new_size);

		//check return address
		if (check_block(va, expectedVA, expectedSize, 1) == 0)
		{
			is_correct = 0;
			cprintf("test_realloc_block_FF #3.1.1: Failed\n");
		}
		//check content of reallocated block
		if (*(startVAs[blockIndex]) != blockIndex || *(midVAs[blockIndex]) != blockIndex ||	*(endVAs[blockIndex]) != blockIndex)
		{
			is_correct = 0;
			cprintf("test_realloc_block_FF #3.1.2: WRONG REALLOC! content of the block is not correct. Expected %d\n", blockIndex);
		}
	}
	if (is_correct)
	{
		eval += 25;
	}

	//[3.2] reallocate in same place (NO relocate - NO split)
	cprintf("	3.2: reallocate in same place (NO relocate - NO split)\n\n") ;
	is_correct = 1;
	{
		blockIndex = 4*allocCntPerSize - 1 ;
		//new_size = allocSizes[3] /*12+16B + 2KB/2*/ + allocSizes[4]/2 /*2KB/2*/ - sizeOfMetaData;
		new_size = allocSizes[3] + allocSizes[4] - sizeOfMetaData;
		expectedSize = ROUNDUP(new_size + sizeOfMetaData, 2);
		expectedVA = startVAs[blockIndex];

		va = realloc_block_FF(startVAs[blockIndex], new_size);

		expectedNumOfFreeBlks--;

		if (check_block(va, expectedVA, expectedSize, 1) == 0)
		{
			is_correct = 0;
			cprintf("test_realloc_block_FF #3.2.1: Failed\n");
		}
		//check content of reallocated block
		if (*(startVAs[blockIndex]) != blockIndex || *(midVAs[blockIndex]) != blockIndex ||	*(endVAs[blockIndex]) != blockIndex)
		{
			is_correct = 0;
			cprintf("test_realloc_block_FF #3.2.2: WRONG REALLOC! content of the block is not correct. Expected %d\n", blockIndex);
		}

		if (is_correct) is_correct = check_list_size(expectedNumOfFreeBlks);
	}
	if (is_correct)
	{
		eval += 25;
	}

	//====================================================================//
	//[4] Test realloc with decreased sizes
	//====================================================================//
	cprintf("4: Test calling realloc with decreased sizes.[30%]\n\n") ;
	//[4.1] next block is full (NO coalesce)
	cprintf("	4.1: next block is full (NO coalesce)\n\n") ;
	is_correct = 1;
	{
		blockIndex = 0*allocCntPerSize + 1; /*4KB*/
		old_size = allocSizes[0] - sizeOfMetaData; /*4KB - sizeOfMetaData*/;
		new_size = old_size - 1*kilo ;
		expectedSize = ROUNDUP(new_size + sizeOfMetaData, 2);
		expectedVA = startVAs[blockIndex];

		va = realloc_block_FF(startVAs[blockIndex], new_size);

		expectedNumOfFreeBlks++;

		if (check_block(va, expectedVA, expectedSize, 1) == 0)
		{
			is_correct = 0;
			cprintf("test_realloc_block_FF #4.1.1: Failed\n");
		}
		//check new free block
		struct BlockElement *newBlkAddr = (struct BlockElement *)(va + new_size + 2*sizeof(int));
		cprintf("\nrealloc Test: newBlkAddr @va %x\n", newBlkAddr);
		expectedSize = 1*kilo ;
		if (check_block(newBlkAddr, newBlkAddr, expectedSize, 0) == 0)
		{
			is_correct = 0;
			cprintf("test_realloc_block_FF #4.1.2: Failed\n");
		}
		//check content of reallocated block
		if (*(startVAs[blockIndex]) != blockIndex || *(midVAs[blockIndex]) != blockIndex)
		{
			is_correct = 0;
			cprintf("test_realloc_block_FF #4.1.3: WRONG REALLOC! content of the block is not correct. Expected %d\n", blockIndex);
		}

		//Check # free blocks
		if (is_correct) is_correct = check_list_size(expectedNumOfFreeBlks);

	}
	if (is_correct)
	{
		eval += 15;
	}
	cprintf("	4.2: next block is full (NO coalesce) [Internal Fragmentation]\n\n") ;

	is_correct = 1;
	{
		blockIndex = 1*allocCntPerSize + 1;
		old_size = allocSizes[1] - sizeOfMetaData;/*20 B*/
		new_size = old_size - 6;
		expectedSize = allocSizes[1]; /*Same block size [Internal Framgmentation]*/
		expectedVA = startVAs[blockIndex];

		va = realloc_block_FF(startVAs[blockIndex], new_size);

		if (check_block(va, expectedVA, expectedSize, 1) == 0)
		{
			is_correct = 0;
			cprintf("test_realloc_block_FF #4.2.1: Failed\n");
		}
		//check content of reallocated block
		if (*(startVAs[blockIndex]) != blockIndex || *(midVAs[blockIndex]) != blockIndex)
		{
			is_correct = 0;
			cprintf("test_realloc_block_FF #4.2.2: WRONG REALLOC! content of the block is not correct. Expected %d\n", blockIndex);
		}

		//Check # free blocks
		if (is_correct) is_correct = check_list_size(expectedNumOfFreeBlks);

	}
	if (is_correct)
	{
		eval += 15;
	}

	cprintf("[PARTIAL] test realloc_block with FIRST FIT completed. Evaluation = %d%\n", eval);

}


void test_realloc_block_FF_COMPLETE()
{
#if USE_KHEAP
	panic("test_free_block: the kernel heap should be disabled. make sure USE_KHEAP = 0");
	return;
#endif

	panic("this is UNSEEN test");

}


/********************Helper Functions***************************/
