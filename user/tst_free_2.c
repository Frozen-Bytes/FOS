/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 2000 */
/* *********************************************************** */

#include <inc/lib.h>
#include <user/tst_utilities.h>

#define Mega  (1024*1024)
#define kilo (1024)
#define numOfAllocs 7
#define allocCntPerSize 200
#define sizeOfMetaData 8
/*NOTE: these sizes include the size of MetaData within it &
 * ALL sizes are in increasing order to ensure that they allocated contiguously
 * & no later block can be allocated in a previous unused free block at the end of page
 */
uint32 allocSizes[numOfAllocs] = {3*sizeof(int) + sizeOfMetaData, 2*sizeOfMetaData, 20*sizeof(char) + sizeOfMetaData, kilo/2, 1*kilo, 3*kilo/2, 2*kilo} ;
short* startVAs[numOfAllocs*allocCntPerSize+1] ;
short* midVAs[numOfAllocs*allocCntPerSize+1] ;
short* endVAs[numOfAllocs*allocCntPerSize+1] ;

void _main(void)
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN KERNEL DYNAMIC ALLOCATOR sbrk()
	 * (e.g. DURING THE DYNAMIC CREATION OF WS ELEMENT in FH).
	 *********************************************************/
	sys_set_uheap_strategy(UHP_PLACE_FIRSTFIT);

	//cprintf("1\n");
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

	int eval = 0;
	bool is_correct = 1;
	int targetAllocatedSpace = 3*Mega;

	void * va ;
	int idx = 0;
	bool chk;
	int usedDiskPages = sys_pf_calculate_allocated_pages() ;
	int freeFrames = sys_calculate_free_frames() ;
	uint32 actualSize, block_size, blockIndex;
	int8 block_status;
	void* expectedVA;
	uint32 expectedSize, curTotalSize,roundedTotalSize ;

	void* curVA = (void*) USER_HEAP_START + sizeof(int) /*BEG Block*/ ;
	//====================================================================//
	/*INITIAL ALLOC Scenario 1: Try to allocate set of blocks with different sizes*/
	cprintf("PREREQUISITE#1: Try to allocate set of blocks with different sizes [all should fit]\n\n") ;
	{
		is_correct = 1;
		curTotalSize = sizeof(int);
		for (int i = 0; i < numOfAllocs; ++i)
		{
			for (int j = 0; j < allocCntPerSize; ++j)
			{
				actualSize = allocSizes[i] - sizeOfMetaData;
				va = startVAs[idx] = malloc(actualSize);
				midVAs[idx] = va + actualSize/2 ;
				endVAs[idx] = va + actualSize - sizeof(short);
				//Check returned va
				expectedVA = (curVA + sizeOfMetaData/2);
				expectedSize = allocSizes[i];
				curTotalSize += allocSizes[i] ;
				//============================================================
				//Check if the remaining area doesn't fit the DynAllocBlock,
				//so update the curVA & curTotalSize to skip this area
				roundedTotalSize = ROUNDUP(curTotalSize, PAGE_SIZE);
				int diff = (roundedTotalSize - curTotalSize) ;
				if (diff > 0 && diff < (DYN_ALLOC_MIN_BLOCK_SIZE + sizeOfMetaData))
				{
//					cprintf("%~\n FRAGMENTATION: curVA = %x diff = %d\n", curVA, diff);
//					cprintf("%~\n Allocated block @ %x with size = %d\n", va, get_block_size(va));

					curVA = ROUNDUP(curVA, PAGE_SIZE)- sizeof(int) /*next alloc will start at END Block (after sbrk)*/;
					curTotalSize = roundedTotalSize - sizeof(int) /*exclude END Block*/;
					expectedSize += diff - sizeof(int) /*exclude END Block*/;
				}
				else
				{
					curVA += allocSizes[i] ;
				}
				//============================================================
				if (is_correct)
				{
					if (check_block(va, expectedVA, expectedSize, 1) == 0)
					{
						if (is_correct)
						{
							is_correct = 0;
							panic("alloc_block_xx #PRQ.%d: WRONG ALLOC\n", idx);
						}
					}
				}
				idx++;

			}
			//if (is_correct == 0)
			//break;
		}
	}
	/* Fill the remaining space at the end of the DA*/
	roundedTotalSize = ROUNDUP(curTotalSize, PAGE_SIZE);
	uint32 remainSize = (roundedTotalSize - curTotalSize) - sizeof(int) /*END block*/;
	if (remainSize >= (DYN_ALLOC_MIN_BLOCK_SIZE + sizeOfMetaData))
	{
		cprintf("Filling the remaining size of %d\n\n", remainSize);
		va = startVAs[idx] = alloc_block(remainSize - sizeOfMetaData, DA_FF);
		//Check returned va
		expectedVA = curVA + sizeOfMetaData/2;
		if (check_block(va, expectedVA, remainSize, 1) == 0)
		{
			is_correct = 0;
			panic("alloc_block_xx #PRQ.oo: WRONG ALLOC\n", idx);
		}
	}

	freeFrames = sys_calculate_free_frames() ;

	//====================================================================//
	/*Free set of blocks with different sizes (first block of each size)*/
	cprintf("1: Free set of blocks with different sizes (first block of each size)\n\n") ;
	{
		is_correct = 1;
		for (int i = 0; i < numOfAllocs; ++i)
		{
			free(startVAs[i*allocCntPerSize]);
			if (check_block(startVAs[i*allocCntPerSize], startVAs[i*allocCntPerSize], allocSizes[i], 0) == 0)
			{
				is_correct = 0;
				cprintf("test_free_2 #1.1: WRONG FREE!\n");
			}
		}

		//Free block before last
		free(startVAs[numOfAllocs*allocCntPerSize - 1]);

		if (check_block(startVAs[numOfAllocs*allocCntPerSize - 1], startVAs[numOfAllocs*allocCntPerSize - 1], allocSizes[numOfAllocs-1], 0) == 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #1.2: WRONG FREE!\n");
		}

		//Reallocate first block
		actualSize = allocSizes[0] - sizeOfMetaData;
		va = malloc(actualSize);
		//Check returned va
		expectedVA = (void*)(USER_HEAP_START + sizeof(int) + sizeOfMetaData/2);
		if (check_block(va, expectedVA, allocSizes[0], 1) == 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #1.3: WRONG ALLOCATE AFTER FREE!\n");
		}

		//Free 2nd block
		free(startVAs[1]);
		if (check_block(startVAs[1],startVAs[1], allocSizes[0], 0) == 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #1.4: WRONG FREE!\n");
		}

		if (is_correct)
		{
			eval += 10;
		}
	}

	//====================================================================//
	/*free Scenario 2: Merge with previous ONLY (AT the tail)*/
	cprintf("2: Free some allocated blocks [Merge with previous ONLY]\n\n") ;
	{
		cprintf("	2.1: at the tail\n\n") ;
		is_correct = 1;
		//Free last block (coalesce with previous)
		blockIndex = numOfAllocs*allocCntPerSize;
		free(startVAs[blockIndex]);
		expectedSize = allocSizes[numOfAllocs-1] + remainSize;
		if (check_block(startVAs[blockIndex-1],startVAs[blockIndex-1], expectedSize, 0) == 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #2.1: WRONG FREE!\n");
		}

		//====================================================================//
		/*free Scenario 3: Merge with previous ONLY (between 2 blocks)*/
		cprintf("	2.2: between 2 blocks\n\n") ;
		blockIndex = 2*allocCntPerSize+1 ;
		free(startVAs[blockIndex]);
		expectedSize = allocSizes[2]+allocSizes[2];
		if (check_block(startVAs[blockIndex-1],startVAs[blockIndex-1], expectedSize, 0) == 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #2.2: WRONG FREE!\n");
		}
		if (is_correct)
		{
			eval += 10;
		}
	}


	//====================================================================//
	/*free Scenario 4: Merge with next ONLY (AT the head)*/
	cprintf("3: Free some allocated blocks [Merge with next ONLY]\n\n") ;
	{
		cprintf("	3.1: at the head\n\n") ;
		is_correct = 1;
		blockIndex = 0 ;
		free(startVAs[blockIndex]);
		expectedSize = allocSizes[0]+allocSizes[0];
		if (check_block(startVAs[blockIndex],startVAs[blockIndex], expectedSize, 0) == 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #3.1: WRONG FREE!\n");
		}

		//====================================================================//
		/*free Scenario 5: Merge with next ONLY (between 2 blocks)*/
		cprintf("	3.2: between 2 blocks\n\n") ;
		blockIndex = 1*allocCntPerSize - 1 ;
		free(startVAs[blockIndex]);
		block_size = get_block_size(startVAs[blockIndex]) ;
		expectedSize = allocSizes[0]+allocSizes[1];
		if (check_block(startVAs[blockIndex],startVAs[blockIndex], expectedSize, 0) == 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #3.2: WRONG FREE!\n");
		}

		if (is_correct)
		{
			eval += 10;
		}
	}
	//====================================================================//
	/*free Scenario 6: Merge with prev & next */
	cprintf("4: Free some allocated blocks [Merge with previous & next]\n\n") ;
	{
		is_correct = 1;
		blockIndex = 4*allocCntPerSize - 2 ;
		free(startVAs[blockIndex]);

		blockIndex = 4*allocCntPerSize - 1 ;
		free(startVAs[blockIndex]);
		block_size = get_block_size(startVAs[blockIndex-1]) ;
		expectedSize = allocSizes[3]+allocSizes[3]+allocSizes[4];
		if (check_block(startVAs[blockIndex-1],startVAs[blockIndex-1], expectedSize, 0) == 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #4: WRONG FREE!\n");
		}
		if (is_correct)
		{
			eval += 20;
		}
	}

	//====================================================================//
	/*Allocate After Free Scenarios */
	cprintf("5: Allocate After Free [should be placed in coalesced blocks]\n\n") ;
	{
		cprintf("	5.1: in block coalesces with NEXT\n\n") ;
		is_correct = 1;
		cprintf("	5.1.1: a. at head\n\n") ;
		{
			actualSize = 4*sizeof(int);
			expectedSize = ROUNDUP(actualSize + sizeOfMetaData, 2);
			va = malloc(actualSize);
			//Check returned va
			expectedVA = (void*)(USER_HEAP_START + sizeOfMetaData);
			if (check_block(va, expectedVA, expectedSize, 1) == 0)
			{
				is_correct = 0;
				cprintf("test_free_2 #5.1.1: WRONG ALLOCATE AFTER FREE!\n");
			}
		}
		cprintf("	5.1.2: b. after the prev alloc in 5.1.1\n\n") ;
		{
			actualSize = 2*sizeof(int) ;
			va = malloc(actualSize);
			expectedSize = ROUNDUP(actualSize + sizeOfMetaData, 2);
			//Check returned va
			expectedVA = (void*)(USER_HEAP_START + sizeOfMetaData + 4*sizeof(int) + sizeOfMetaData);
			if (check_block(va, expectedVA, expectedSize, 1) == 0)
			{
				is_correct = 0;
				cprintf("test_free_2 #5.1.2: WRONG ALLOCATE AFTER FREE!\n");
			}
		}
		cprintf("	5.1.3: c. between two blocks [INTERNAL FRAGMENTATION CASE]\n\n") ;
		{
			actualSize = 5*sizeof(int); //20B
			expectedSize = allocSizes[0] + allocSizes[1]; //20B + 16B [Internal Fragmentation of 8 Bytes]
			va = malloc(actualSize);
			//Check returned va
			expectedVA = startVAs[1*allocCntPerSize - 1];
			if (check_block(va, expectedVA, expectedSize, 1) == 0)
			{
				is_correct = 0;
				cprintf("test_free_2 #5.1.3: WRONG ALLOCATE AFTER FREE!\n");
			}
		}
		if (is_correct)
		{
			eval += 10;
		}

		cprintf("	5.2: in block coalesces with PREV & NEXT\n\n") ;
		is_correct = 1;
		actualSize = 3*kilo/2;
		expectedSize = ROUNDUP(actualSize + sizeOfMetaData, 2);
		va = malloc(actualSize);
		//Check returned va
		expectedVA = startVAs[4*allocCntPerSize - 2];
		if (check_block(va, expectedVA, expectedSize, 1) == 0)
		{
			is_correct = 0;
			cprintf("test_free_2 #5.2: WRONG ALLOCATE AFTER FREE!\n");
		}
		if (is_correct)
		{
			eval += 10;
		}

		cprintf("	5.3: in block coalesces with PREV\n\n") ;
		cprintf("	5.3.1: a. between two blocks\n\n") ;
		{
			is_correct = 1;
			actualSize = 30*sizeof(char) ;
			expectedSize = ROUNDUP(actualSize + sizeOfMetaData, 2);
			va = malloc(actualSize);
			//Check returned va
			expectedVA = startVAs[2*allocCntPerSize];
			if (check_block(va, expectedVA, expectedSize, 1) == 0)
			{
				is_correct = 0;
				cprintf("test_free_2 #5.3.1: WRONG ALLOCATE AFTER FREE!\n");
			}
		}
		actualSize = 3*kilo/2 - sizeOfMetaData ;
		expectedSize = ROUNDUP(actualSize + sizeOfMetaData, 2);

		//dummy allocation to consume the 1st 1.5 KB free block
		{
			va = malloc(actualSize);
		}
		//dummy allocation to consume the 1st 2 KB free block
		{
			va = malloc(actualSize);
		}

		cprintf("	5.3.2: b. at tail\n\n") ;
		{
			actualSize = 3*kilo/2 ;
			expectedSize = ROUNDUP(actualSize + sizeOfMetaData, 2);

			print_blocks_list(freeBlocksList);

			va = malloc(actualSize);

			//Check returned va
			expectedVA = startVAs[numOfAllocs*allocCntPerSize-1];
			if (check_block(va, expectedVA, expectedSize, 1) == 0)
			{
				is_correct = 0;
				cprintf("test_free_2 #5.3.2: WRONG ALLOCATE AFTER FREE!\n");
			}
		}

		cprintf("	5.3.3: c. after the prev allocated block in 5.3.2\n\n") ;
		{
			actualSize = 3*kilo/2 ;
			expectedSize = ROUNDUP(actualSize + sizeOfMetaData, 2);

			va = malloc(actualSize);

			//Check returned va
			expectedVA = (void*)startVAs[numOfAllocs*allocCntPerSize-1] + 3*kilo/2 + sizeOfMetaData;
			if (check_block(va, expectedVA, expectedSize, 1) == 0)
			{
				is_correct = 0;
				cprintf("test_free_2 #5.3.3: WRONG ALLOCATE AFTER FREE!\n");
			}
		}
		if (is_correct)
		{
			eval += 10;
		}
	}

	//====================================================================//
	/*Check memory allocation*/
	cprintf("6: Check memory allocation [should not be changed due to free]\n\n") ;
	{
		is_correct = 1;
		if ((freeFrames - sys_calculate_free_frames()) != 0)
		{
			cprintf("test_free_2 #6: number of allocated pages in MEMORY is changed due to free() while it's not supposed to!\n");
			is_correct = 0;
		}
		if (is_correct)
		{
			eval += 10;
		}
	}

	uint32 expectedAllocatedSize = curTotalSize;
//	for (int i = 0; i < numOfAllocs; ++i)
//	{
//		expectedAllocatedSize += allocCntPerSize * allocSizes[i] ;
//	}
	expectedAllocatedSize = ROUNDUP(expectedAllocatedSize, PAGE_SIZE);
	uint32 expectedAllocNumOfPages = expectedAllocatedSize / PAGE_SIZE; 				/*# pages*/
	uint32 expectedAllocNumOfTables = ROUNDUP(expectedAllocatedSize, PTSIZE) / PTSIZE; 	/*# tables*/

	//====================================================================//
	/*Check WS elements*/
	cprintf("7: Check WS Elements [should not be changed due to free]\n\n") ;
	{
		is_correct = 1;
		uint32* expectedVAs = malloc(expectedAllocNumOfPages*sizeof(int));
		int i = 0;
		for (uint32 va = USER_HEAP_START; va < USER_HEAP_START + expectedAllocatedSize; va+=PAGE_SIZE)
		{
			expectedVAs[i++] = va;
		}
		chk = sys_check_WS_list(expectedVAs, expectedAllocNumOfPages, 0, 2);
		if (chk != 1)
		{
			cprintf("test_free_2 #7: page is either not added to WS or removed from it\n");
			is_correct = 0;
		}
		if (is_correct)
		{
			eval += 10;
		}
	}

	cprintf("%~test free() with FIRST FIT completed [DYNAMIC ALLOCATOR]. Evaluation = %d%\n", eval);

	return;
}

