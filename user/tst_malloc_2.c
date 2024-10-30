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
	void* expectedVA;
	uint32 actualSize, expectedSize, curTotalSize,roundedTotalSize ;
	//====================================================================//
	/*INITIAL ALLOC Scenario 1: Try to allocate set of blocks with different sizes*/
	cprintf("%~\n1: [BLOCK ALLOCATOR] allocate set of blocks with different sizes [all should fit] [30%]\n") ;
	{
		is_correct = 1;
		void* curVA = (void*) USER_HEAP_START + sizeof(int) /*BEG Block*/ ;
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
							cprintf("alloc_block_xx #1.%d: WRONG ALLOC\n", idx);
						}
					}
				}
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
			eval += 30;
		}
	}

	//====================================================================//
	/*INITIAL ALLOC Scenario 2: Check stored data inside each allocated block*/
	cprintf("%~\n2: Check stored data inside each allocated block [30%]\n") ;
	{
		is_correct = 1;

		for (int i = 0; i < idx; ++i)
		{
			if (*(startVAs[i]) != i || *(midVAs[i]) != i ||	*(endVAs[i]) != i)
			{
				is_correct = 0;
				cprintf("alloc_block_xx #2.%d: WRONG! content of the block is not correct. Expected %d\n",i, i);
				break;
			}
		}
		if (is_correct)
		{
			eval += 30;
		}
	}

	/*Check page file*/
	cprintf("%~\n3: Check page file size (nothing should be allocated) [10%]\n") ;
	{
		is_correct = 1;
		if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0)
		{
			cprintf("page(s) are allocated in PageFile while not expected to\n");
			is_correct = 0;
		}
		if (is_correct)
		{
			eval += 10;
		}
	}

	uint32 expectedAllocatedSize = 0;
//	for (int i = 0; i < numOfAllocs; ++i)
//	{
//		expectedAllocatedSize += allocCntPerSize * allocSizes[i] ;
//	}
//	expectedAllocatedSize = ROUNDUP(expectedAllocatedSize, PAGE_SIZE);
	expectedAllocatedSize = ROUNDUP(curTotalSize, PAGE_SIZE);
	uint32 expectedAllocNumOfPages = expectedAllocatedSize / PAGE_SIZE; 				/*# pages*/
	uint32 expectedAllocNumOfTables = ROUNDUP(expectedAllocatedSize, PTSIZE) / PTSIZE; 	/*# tables*/
	uint32 expectedAllocNumOfPagesForWS = ROUNDUP(expectedAllocNumOfPages * (sizeof(struct WorkingSetElement) + sizeOfMetaData), PAGE_SIZE) / PAGE_SIZE; 				/*# pages*/

	/*Check memory allocation*/
	cprintf("%~\n4: Check total allocation in RAM (for pages, tables & WS) [10%]\n") ;
	{
		is_correct = 1;
		uint32 expected = expectedAllocNumOfPages + expectedAllocNumOfTables  + expectedAllocNumOfPagesForWS;
		uint32 actual = (freeFrames - sys_calculate_free_frames()) ;
		if (expected != actual)
		{
			cprintf("number of allocated pages in MEMORY not correct. Expected %d, Actual %d\n", expected, actual);
			is_correct = 0;
		}
		if (is_correct)
		{
			eval += 10;
		}
	}

	/*Check WS elements*/
	cprintf("%~\n5: Check content of WS [20%]\n") ;
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
			cprintf("malloc: page is not added to WS\n");
			is_correct = 0;
		}
		if (is_correct)
		{
			eval += 20;
		}
	}

	cprintf("%~\ntest malloc (2) [DYNAMIC ALLOCATOR] is finished. Evaluation = %d%\n", eval);

	return;
}

