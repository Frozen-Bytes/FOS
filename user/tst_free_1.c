/* *********************************************************** */
/* MAKE SURE PAGE_WS_MAX_SIZE = 3000 */
/* *********************************************************** */
#include <inc/lib.h>

struct MyStruct
{
	char a;
	short b;
	int c;
};
int inRange(int val, int min, int max)
{
	return (val >= min && val <= max) ? 1 : 0;
}
void _main(void)
{
	/*********************** NOTE ****************************
	 * WE COMPARE THE DIFF IN FREE FRAMES BY "AT LEAST" RULE
	 * INSTEAD OF "EQUAL" RULE SINCE IT'S POSSIBLE THAT SOME
	 * PAGES ARE ALLOCATED IN DYNAMIC ALLOCATOR DUE TO sbrk()
	 * (e.g. DURING THE DYNAMIC CREATION OF WS ELEMENT in FH).
	 *********************************************************/

#if USE_KHEAP
	//cprintf("1\n");
	//Initial test to ensure it works on "PLACEMENT" not "REPLACEMENT"
	{
		if (LIST_SIZE(&(myEnv->page_WS_list)) >= myEnv->page_WS_max_size)
			panic("Please increase the WS size");
	}
	/*=================================================*/
#else
	panic("not handled!");
#endif
	uint32 pagealloc_start = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE; //UHS + 32MB + 4KB

	int eval = 0;
	bool is_correct = 1;

	int Mega = 1024*1024;
	int kilo = 1024;
	char minByte = 1<<7;
	char maxByte = 0x7F;
	short minShort = 1<<15 ;
	short maxShort = 0x7FFF;
	int minInt = 1<<31 ;
	int maxInt = 0x7FFFFFFF;

	char *byteArr, *byteArr2 ;
	short *shortArr, *shortArr2 ;
	int *intArr;
	struct MyStruct *structArr ;
	int lastIndexOfByte, lastIndexOfByte2, lastIndexOfShort, lastIndexOfShort2, lastIndexOfInt, lastIndexOfStruct;
	bool found;
	int start_freeFrames = sys_calculate_free_frames() ;
	int freeFrames, usedDiskPages, chk;
	int expectedNumOfFrames, actualNumOfFrames;
	cprintf("\n%~[1] Allocate spaces of different sizes in PAGE ALLOCATOR and write some data to them\n");
	void* ptr_allocations[20] = {0};
	{
		//cprintf("3\n");
		//2 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[0] = malloc(2*Mega-kilo);
			if ((uint32) ptr_allocations[0] != (pagealloc_start)) {is_correct = 0; cprintf("1 Wrong start address for the allocated space... \n");}
			expectedNumOfFrames = 1 /*table*/ ;
			actualNumOfFrames = freeFrames - sys_calculate_free_frames();
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{is_correct = 0; cprintf("1 Wrong allocation: unexpected number of pages that are allocated in memory! Expected = [%d, %d], Actual = %d\n", expectedNumOfFrames, expectedNumOfFrames+2, actualNumOfFrames);}
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) { is_correct = 0; cprintf("1 Extra or less pages are allocated in PageFile\n");}

			freeFrames = sys_calculate_free_frames() ;
			lastIndexOfByte = (2*Mega-kilo)/sizeof(char) - 1;
			byteArr = (char *) ptr_allocations[0];
			byteArr[0] = minByte ;
			byteArr[lastIndexOfByte] = maxByte ;
			expectedNumOfFrames = 2 /*+1 table already created in malloc due to marking the allocated pages*/ ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{ is_correct = 0; cprintf("1 Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);}

			uint32 expectedVAs[2] = { ROUNDDOWN((uint32)(&(byteArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(byteArr[lastIndexOfByte])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 2, 0, 2);
			if (found != 1) { is_correct = 0; cprintf("1 malloc: page is not added to WS\n");}
		}
		//2 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[1] = malloc(2*Mega-kilo);
			if ((uint32) ptr_allocations[1] != (pagealloc_start + 2*Mega)) { is_correct = 0; cprintf("2 Wrong start address for the allocated space... \n");}
			expectedNumOfFrames = 0 /*table exists*/ ;
			actualNumOfFrames = freeFrames - sys_calculate_free_frames();
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{is_correct = 0; cprintf("2 Wrong allocation: unexpected number of pages that are allocated in memory! Expected = [%d, %d], Actual = %d\n", expectedNumOfFrames, expectedNumOfFrames+2, actualNumOfFrames);}
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) { is_correct = 0; cprintf("2 Extra or less pages are allocated in PageFile\n");}

			freeFrames = sys_calculate_free_frames() ;
			shortArr = (short *) ptr_allocations[1];
			lastIndexOfShort = (2*Mega-kilo)/sizeof(short) - 1;
			shortArr[0] = minShort;
			shortArr[lastIndexOfShort] = maxShort;
			expectedNumOfFrames = 2 ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{ is_correct = 0; cprintf("2 Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);}
			uint32 expectedVAs[2] = { ROUNDDOWN((uint32)(&(shortArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(shortArr[lastIndexOfShort])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 2, 0, 2);
			if (found != 1) { is_correct = 0; cprintf("2 malloc: page is not added to WS\n");}
		}

		//3 KB
		{
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[2] = malloc(3*kilo);
			if ((uint32) ptr_allocations[2] != (pagealloc_start + 4*Mega)) { is_correct = 0; cprintf("3 Wrong start address for the allocated space... \n");}
			expectedNumOfFrames = 1 /*table*/ ;
			actualNumOfFrames = freeFrames - sys_calculate_free_frames();
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{is_correct = 0; cprintf("3 Wrong allocation: unexpected number of pages that are allocated in memory! Expected = [%d, %d], Actual = %d\n", expectedNumOfFrames, expectedNumOfFrames+2, actualNumOfFrames);}
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) { is_correct = 0; cprintf("3 Extra or less pages are allocated in PageFile\n");}

			freeFrames = sys_calculate_free_frames() ;
			intArr = (int *) ptr_allocations[2];
			lastIndexOfInt = (2*kilo)/sizeof(int) - 1;
			intArr[0] = minInt;
			intArr[lastIndexOfInt] = maxInt;
			expectedNumOfFrames = 1 ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{ is_correct = 0; cprintf("3 Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);}
			uint32 expectedVAs[2] = { ROUNDDOWN((uint32)(&(intArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(intArr[lastIndexOfInt])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 2, 0, 2);
			if (found != 1) { is_correct = 0; cprintf("3 malloc: page is not added to WS\n");}
		}

		//3 KB
		{
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[3] = malloc(3*kilo);
			if ((uint32) ptr_allocations[3] != (pagealloc_start + 4*Mega + 4*kilo)) { is_correct = 0; cprintf("4 Wrong start address for the allocated space... \n");}
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) { is_correct = 0; cprintf("4 Extra or less pages are allocated in PageFile\n");}
		}
		//7 KB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[4] = malloc(7*kilo);
			if ((uint32) ptr_allocations[4] != (pagealloc_start + 4*Mega + 8*kilo)) { is_correct = 0; cprintf("5 Wrong start address for the allocated space... \n");}
			expectedNumOfFrames = 0 /*no table*/ ;
			actualNumOfFrames = freeFrames - sys_calculate_free_frames();
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{is_correct = 0; cprintf("5 Wrong allocation: unexpected number of pages that are allocated in memory! Expected = [%d, %d], Actual = %d\n", expectedNumOfFrames, expectedNumOfFrames+2, actualNumOfFrames);}
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) { is_correct = 0; cprintf("5 Extra or less pages are allocated in PageFile\n");}

			freeFrames = sys_calculate_free_frames() ;
			structArr = (struct MyStruct *) ptr_allocations[4];
			lastIndexOfStruct = (7*kilo)/sizeof(struct MyStruct) - 1;
			structArr[0].a = minByte; structArr[0].b = minShort; structArr[0].c = minInt;
			structArr[lastIndexOfStruct].a = maxByte; structArr[lastIndexOfStruct].b = maxShort; structArr[lastIndexOfStruct].c = maxInt;
			expectedNumOfFrames = 2 ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{ is_correct = 0; cprintf("5 Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);}
			uint32 expectedVAs[2] = { ROUNDDOWN((uint32)(&(structArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(structArr[lastIndexOfStruct])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 2, 0, 2);
			if (found != 1) { is_correct = 0; cprintf("5 malloc: page is not added to WS\n");}
		}
		//3 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[5] = malloc(3*Mega-kilo);
			if ((uint32) ptr_allocations[5] != (pagealloc_start + 4*Mega + 16*kilo)) { is_correct = 0; cprintf("6 Wrong start address for the allocated space... \n");}
			expectedNumOfFrames = 0 /*no table*/ ;
			actualNumOfFrames = freeFrames - sys_calculate_free_frames();
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{is_correct = 0; cprintf("6 Wrong allocation: unexpected number of pages that are allocated in memory! Expected = [%d, %d], Actual = %d\n", expectedNumOfFrames, expectedNumOfFrames+2, actualNumOfFrames);}
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) { is_correct = 0; cprintf("6 Extra or less pages are allocated in PageFile\n");}
		}
		//6 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[6] = malloc(6*Mega-kilo);
			if ((uint32) ptr_allocations[6] != (pagealloc_start + 7*Mega + 16*kilo)) { is_correct = 0; cprintf("7 Wrong start address for the allocated space... \n");}
			expectedNumOfFrames = 2 /*table*/ ;
			actualNumOfFrames = freeFrames - sys_calculate_free_frames();
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{is_correct = 0; cprintf("7 Wrong allocation: unexpected number of pages that are allocated in memory! Expected = [%d, %d], Actual = %d\n", expectedNumOfFrames, expectedNumOfFrames+2, actualNumOfFrames);}
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) { is_correct = 0; cprintf("7 Extra or less pages are allocated in PageFile\n");}

			freeFrames = sys_calculate_free_frames() ;
			lastIndexOfByte2 = (6*Mega-kilo)/sizeof(char) - 1;
			byteArr2 = (char *) ptr_allocations[6];
			byteArr2[0] = minByte ;
			byteArr2[lastIndexOfByte2 / 2] = maxByte / 2;
			byteArr2[lastIndexOfByte2] = maxByte ;
			expectedNumOfFrames = 3 ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{ is_correct = 0; cprintf("7 Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);}
			uint32 expectedVAs[3] = { ROUNDDOWN((uint32)(&(byteArr2[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(byteArr2[lastIndexOfByte2/2])), PAGE_SIZE), ROUNDDOWN((uint32)(&(byteArr2[lastIndexOfByte2])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 3, 0, 2);
			if (found != 1) { is_correct = 0; cprintf("7 malloc: page is not added to WS\n");}
		}
		//14 KB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			ptr_allocations[7] = malloc(14*kilo);
			if ((uint32) ptr_allocations[7] != (pagealloc_start + 13*Mega + 16*kilo)) { is_correct = 0; cprintf("8 Wrong start address for the allocated space... \n");}
			expectedNumOfFrames = 0 /*table*/ ;
			actualNumOfFrames = freeFrames - sys_calculate_free_frames();
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{is_correct = 0; cprintf("8 Wrong allocation: unexpected number of pages that are allocated in memory! Expected = [%d, %d], Actual = %d\n", expectedNumOfFrames, expectedNumOfFrames+2, actualNumOfFrames);}
			if ((sys_pf_calculate_allocated_pages() - usedDiskPages) != 0) { is_correct = 0; cprintf("8 Extra or less pages are allocated in PageFile\n");}

			freeFrames = sys_calculate_free_frames() ;
			shortArr2 = (short *) ptr_allocations[7];
			lastIndexOfShort2 = (14*kilo)/sizeof(short) - 1;
			shortArr2[0] = minShort;
			shortArr2[lastIndexOfShort2 / 2] = maxShort / 2;
			shortArr2[lastIndexOfShort2] = maxShort;
			expectedNumOfFrames = 3 ;
			actualNumOfFrames = (freeFrames - sys_calculate_free_frames()) ;
			if (!inRange(actualNumOfFrames, expectedNumOfFrames, expectedNumOfFrames + 2 /*Block Alloc: max of 1 page & 1 table*/))
			{ is_correct = 0; cprintf("8 Wrong fault handler: pages are not loaded successfully into memory/WS. Expected diff in frames at least = %d, actual = %d\n", expectedNumOfFrames, actualNumOfFrames);}
			uint32 expectedVAs[3] = { ROUNDDOWN((uint32)(&(shortArr2[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(shortArr2[lastIndexOfShort2/2])), PAGE_SIZE), ROUNDDOWN((uint32)(&(shortArr2[lastIndexOfShort2])), PAGE_SIZE)} ;
			found = sys_check_WS_list(expectedVAs, 3, 0, 2);
			if (found != 1) { is_correct = 0; cprintf("8 malloc: page is not added to WS\n");}
		}
	}
	uint32 pagealloc_end = pagealloc_start + 13*Mega + 32*kilo ;


	is_correct = 1;
	//FREE ALL
	cprintf("\n%~[2] Free all the allocated spaces from PAGE ALLOCATOR \[70%]\n");
	{
		//Free 1st 2 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			free(ptr_allocations[0]);

			if ((usedDiskPages - sys_pf_calculate_allocated_pages()) != 0) { is_correct = 0; cprintf("9 Wrong free: Extra or less pages are removed from PageFile\n");}
			if ((sys_calculate_free_frames() - freeFrames) != 2 ) { is_correct = 0; cprintf("9 Wrong free: WS pages in memory and/or page tables are not freed correctly\n");}
			uint32 notExpectedVAs[2] = { ROUNDDOWN((uint32)(&(byteArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(byteArr[lastIndexOfByte])), PAGE_SIZE)} ;
			chk = sys_check_WS_list(notExpectedVAs, 2, 0, 3);
			if (chk != 1) { is_correct = 0; cprintf("9 free: page is not removed from WS\n");}
		}
		if (is_correct)
		{
			eval += 10;
		}

		is_correct = 1;
		//Free 2nd 2 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			free(ptr_allocations[1]);
			if ((usedDiskPages - sys_pf_calculate_allocated_pages()) != 0)
			{ is_correct = 0; cprintf("10 Wrong free: Extra or less pages are removed from PageFile\n");}
			if ((sys_calculate_free_frames() - freeFrames) != 2 /*we don't remove free tables anymore*/)
			{ is_correct = 0; cprintf("10 Wrong free: WS pages in memory and/or page tables are not freed correctly\n");}
			uint32 notExpectedVAs[2] = { ROUNDDOWN((uint32)(&(shortArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(shortArr[lastIndexOfShort])), PAGE_SIZE)} ;
			chk = sys_check_WS_list(notExpectedVAs, 2, 0, 3);
			if (chk != 1) { is_correct = 0; cprintf("10 free: page is not removed from WS\n");}
		}
		if (is_correct)
		{
			eval += 10;
		}

		is_correct = 1;
		//Free 6 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			free(ptr_allocations[6]);
			if ((usedDiskPages - sys_pf_calculate_allocated_pages()) != 0)
			{ is_correct = 0; cprintf("11 Wrong free: Extra or less pages are removed from PageFile\n");}
			if ((sys_calculate_free_frames() - freeFrames) != 3 /*+ 1*/)
			{ is_correct = 0; cprintf("11 Wrong free: WS pages in memory and/or page tables are not freed correctly\n");}
			uint32 notExpectedVAs[3] = { ROUNDDOWN((uint32)(&(byteArr2[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(byteArr2[lastIndexOfByte2/2])), PAGE_SIZE), ROUNDDOWN((uint32)(&(byteArr2[lastIndexOfByte2])), PAGE_SIZE)} ;
			chk = sys_check_WS_list(notExpectedVAs, 3, 0, 3);
			if (chk != 1) { is_correct = 0; cprintf("11 free: page is not removed from WS\n");}
		}
		if (is_correct)
		{
			eval += 10;
		}

		is_correct = 1;
		//Free 7 KB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			free(ptr_allocations[4]);
			if ((usedDiskPages - sys_pf_calculate_allocated_pages()) != 0)
			{ is_correct = 0; cprintf("12 Wrong free: Extra or less pages are removed from PageFile\n");}
			if ((sys_calculate_free_frames() - freeFrames) != 2)
			{ is_correct = 0; cprintf("12 Wrong free: WS pages in memory and/or page tables are not freed correctly\n");}
			uint32 notExpectedVAs[2] = { ROUNDDOWN((uint32)(&(structArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(structArr[lastIndexOfStruct])), PAGE_SIZE)} ;
			chk = sys_check_WS_list(notExpectedVAs, 2, 0, 3);
			if (chk != 1) { is_correct = 0; cprintf("12 free: page is not removed from WS\n");}
		}
		if (is_correct)
		{
			eval += 10;
		}

		is_correct = 1;
		//Free 3 MB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			free(ptr_allocations[5]);
			if ((usedDiskPages - sys_pf_calculate_allocated_pages()) != 0)
			{ is_correct = 0; cprintf("13 Wrong free: Extra or less pages are removed from PageFile\n");}
			if ((sys_calculate_free_frames() - freeFrames) != 0)
			{ is_correct = 0; cprintf("13 Wrong free: unexpected number of free frames after calling free\n");}
		}
		if (is_correct)
		{
			eval += 5;
		}

		is_correct = 1;
		//Free 1st 3 KB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			free(ptr_allocations[2]);
			if ((usedDiskPages - sys_pf_calculate_allocated_pages()) != 0)
			{ is_correct = 0; cprintf("14 Wrong free: Extra or less pages are removed from PageFile\n");}
			if ((sys_calculate_free_frames() - freeFrames) != 1 /*+ 1*/)
			{ is_correct = 0; cprintf("14 Wrong free: WS pages in memory and/or page tables are not freed correctly\n");}
			uint32 notExpectedVAs[2] = { ROUNDDOWN((uint32)(&(intArr[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(intArr[lastIndexOfInt])), PAGE_SIZE)} ;
			chk = sys_check_WS_list(notExpectedVAs, 2, 0, 3);
			if (chk != 1) { is_correct = 0; cprintf("14 free: page is not removed from WS\n");}
		}
		if (is_correct)
		{
			eval += 10;
		}

		is_correct = 1;
		//Free 2nd 3 KB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			free(ptr_allocations[3]);
			if ((usedDiskPages - sys_pf_calculate_allocated_pages()) != 0)
			{ is_correct = 0; cprintf("15 Wrong free: Extra or less pages are removed from PageFile\n");}
			if ((sys_calculate_free_frames() - freeFrames) != 0)
			{ is_correct = 0; cprintf("15 Wrong free: WS pages in memory and/or page tables are not freed correctly\n");}
		}
		if (is_correct)
		{
			eval += 5;
		}

		is_correct = 1;
		//Free last 14 KB
		{
			freeFrames = sys_calculate_free_frames() ;
			usedDiskPages = sys_pf_calculate_allocated_pages() ;
			free(ptr_allocations[7]);
			if ((usedDiskPages - sys_pf_calculate_allocated_pages()) != 0)
			{ is_correct = 0; cprintf("16 Wrong free: Extra or less pages are removed from PageFile\n");}
			if ((sys_calculate_free_frames() - freeFrames) != 3 /*+ 1*/)
			{ is_correct = 0; cprintf("16 Wrong free: WS pages in memory and/or page tables are not freed correctly\n");}
			uint32 notExpectedVAs[3] = { ROUNDDOWN((uint32)(&(shortArr2[0])), PAGE_SIZE), ROUNDDOWN((uint32)(&(shortArr2[lastIndexOfShort2/2])), PAGE_SIZE), ROUNDDOWN((uint32)(&(shortArr2[lastIndexOfShort2])), PAGE_SIZE)} ;
			chk = sys_check_WS_list(notExpectedVAs, 3, 0, 3);
			if (chk != 1) { is_correct = 0; cprintf("16 free: page is not removed from WS\n");}
		}
		if (is_correct)
		{
			eval += 10;
		}

		is_correct = 1;
	}

	is_correct = 1;

	//Test accessing a freed area (processes should be killed by the validation of the fault handler)
	cprintf("\n%~[3] Test accessing a freed area (processes should be killed by the validation of the fault handler) [30%]\n");
	{
		rsttst();
		int ID1 = sys_create_env("tf1_slave1", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
		sys_run_env(ID1);

		//wait until a slave finishes the allocation & freeing operations
		while (gettst() != 1) ;

		int ID2 = sys_create_env("tf1_slave2", (myEnv->page_WS_max_size), (myEnv->SecondListSize),(myEnv->percentage_of_WS_pages_to_be_removed));
		sys_run_env(ID2);

		//wait until a slave finishes the allocation & freeing operations
		while (gettst() != 1) ;

		//signal them to start accessing the freed area
		inctst();

		//sleep for a while to allow each slave to try access its freed location
		env_sleep(10000);

		if (gettst() > 3)
		{ is_correct = 0; cprintf("\n17 Free: access to freed space is done while it's NOT expected to be!! (processes should be killed by the validation of the fault handler)\n");}
	}
	if (is_correct)
	{
		eval += 30;
	}
	cprintf("%~\ntest free [1] [PAGE ALLOCATOR] completed. Eval = %d\n\n", eval);

	return;
}
