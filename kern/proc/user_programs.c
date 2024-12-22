/*
 * user_programs.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */
#include <kern/proc/user_environment.h>
#include <inc/string.h>
#include <inc/assert.h>



//User Programs Table
//The input for any PTR_START_OF macro must be the ".c" filename of the user program
struct UserProgramInfo userPrograms[] = {
		{ "fos_helloWorld", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_helloWorld)},
		{ "fos_add", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_add)},
		{ "fos_alloc", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_alloc)},
		{ "fos_input", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_input)},
		{ "fos_game", "Created by FOS team, fos@nowhere.com", PTR_START_OF(game)},
		{ "fos_static_data_section", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_static_data_section)},
		{ "fos_data_on_stack", "Created by FOS team, fos@nowhere.com", PTR_START_OF(fos_data_on_stack)},

		{ "cnc", "Concurrent program test", PTR_START_OF(concurrent_start)},
		/*TESTING 2024*/
		//[1] LOCKS
		{ "tst_chan_all", "Tests sleep & wakeup ALL on a channel", PTR_START_OF(tst_chan_all_master)},
		{ "tstChanAllSlave", "Slave program of tst_chan_all", PTR_START_OF(tst_chan_all_slave)},
		{ "tst_chan_one", "Tests sleep & wakeup ONE on a channel", PTR_START_OF(tst_chan_one_master)},
		{ "tstChanOneSlave", "Slave program of tst_chan_one", PTR_START_OF(tst_chan_one_slave)},
		{ "mergesort", "mergesort a fixed size array of 800000", PTR_START_OF(mergesort_static)},
		{ "tst_protection", "Tests the protection of kernel shared DS (e.g. kernel heap)", PTR_START_OF(tst_protection)},
		{ "protection_slave1", "Slave program of tst_protection", PTR_START_OF(tst_protection_slave1)},

		//[2] REPLACEMENT
		{ "tpr1", "Tests page replacement (allocation of Memory and PageFile)", PTR_START_OF(tst_page_replacement_alloc)},
		{ "tpr2", "tests page replacement (handling new stack and modified pages)", PTR_START_OF(tst_page_replacement_stack)},
		{ "tnclock1", "Tests page replacement (nth clock algorithm - NORMAL version)", PTR_START_OF(tst_page_replacement_nthclock_1)},
		{ "tnclock2", "Tests page replacement (nth clock algorithm - MODIFIED version)", PTR_START_OF(tst_page_replacement_nthclock_2)},

		/*TESTING 2023*/
		//[1] READY MADE TESTS
		{ "tst_syscalls_1", "Tests correct handling of 3 system calls", PTR_START_OF(tst_syscalls_1)},
		{ "tst_syscalls_2", "Tests correct validation of syscalls params", PTR_START_OF(tst_syscalls_2)},
		{ "tsc2_slave1", "Slave program for tst_syscalls_2", PTR_START_OF(tst_syscalls_2_slave1)},
		{ "tsc2_slave2", "Slave program for tst_syscalls_2", PTR_START_OF(tst_syscalls_2_slave2)},
		{ "tsc2_slave3", "Slave program for tst_syscalls_2", PTR_START_OF(tst_syscalls_2_slave3)},

		{ "fib_memomize", "", PTR_START_OF(fib_memomize)},


		{ "fib_loop", "", PTR_START_OF(fib_loop)},

		{ "matops", "Matrix Operations on two square matrices with NO memory leakage", PTR_START_OF(matrix_operations)},


		/*TESTING 2020*/
		//PAGE FAULT HANDLER TESTS [PLACEMENT + REPLACEMENT]
		{"tpplru1","LRU Approx: tests page placement in case the active list is not FULL",PTR_START_OF(tst_placement_1)},
		{"tpplru2","LRU Approx: tests page placement in case the active list is FULL, and the second active list is NOT FULL",PTR_START_OF(tst_placement_2)},
		{"tpplru3","LRU Approx: tests page faults on pages already exist in the second active list (ACCESS)",PTR_START_OF(tst_placement_3)},
		//USER DYNAMIC ALLOCATION USING LARGE SIZES
		{ "tm1", "tests malloc (1): start address & allocated frames", PTR_START_OF(tst_malloc_1)},
		{ "tm2", "tests malloc (2): writing & reading values in allocated spaces", PTR_START_OF(tst_malloc_2)},
		{ "tm3", "tests malloc (3): check memory allocation and WS after accessing", PTR_START_OF(tst_malloc_3)},
		//USER DYNAMIC DEALLOCATION USING LARGE SIZES
		{ "tf1", "tests free (1): freeing tables, WS and page file [placement case]", PTR_START_OF(tst_free_1)},
		{ "tf1_slave1", "tests free (1) slave1: try accessing values in freed spaces", PTR_START_OF(tst_free_1_slave1)},
		{ "tf1_slave2", "tests free (1) slave2: try accessing values in freed spaces that is not accessed before", PTR_START_OF(tst_free_1_slave2)},
		{ "tf2", "tests free (2): try accessing values in freed spaces", PTR_START_OF(tst_free_2)},
		//FIRST FIT for LARGE SIZES ALLOCATIONS
		{ "tff1", "tests first fit (1): always find suitable space", PTR_START_OF(tst_first_fit_1)},
		{ "tff2", "tests first fit (2): no suitable space", PTR_START_OF(tst_first_fit_2)},

		/*TESTING 2017*/
		//[1] READY MADE TESTS
		{ "tpp", "Tests the Page placement", PTR_START_OF(tst_placement)},
		{ "tia", "tests handling of invalid memory access", PTR_START_OF(tst_invalid_access)},
		{ "tia_slave1", "tia: access kernel", PTR_START_OF(tst_invalid_access_slave1)},
		{ "tia_slave2", "tia: write on read only user page", PTR_START_OF(tst_invalid_access_slave2)},
		{ "tia_slave3", "tia: access an unmarked (non-reserved) user heap page", PTR_START_OF(tst_invalid_access_slave3)},
		{ "tia_slave4", "tia: access a non-exist page in page file, stack and heap", PTR_START_OF(tst_invalid_access_slave4)},
		{ "dummy_process", "[Slave program] contains nested loops with random bounds to consume time", PTR_START_OF(dummy_process)},
		{ "tsem1", "Tests the Semaphores only [critical section & dependency]", PTR_START_OF(tst_semaphore_1master)},
		{ "sem1Slave", "[Slave program] of tst_semaphore_1master", PTR_START_OF(tst_semaphore_1slave)},
		{ "tsem2", "Tests the Semaphores only [multiprograms enter the same CS]", PTR_START_OF(tst_semaphore_2master)},
		{ "sem2Slave", "[Slave program] of tst_semaphore_2master", PTR_START_OF(tst_semaphore_2slave)},
		{ "tff3", "tests first fit (3): malloc, smalloc & sget", PTR_START_OF(tst_first_fit_3)},

		{ "tshr1", "Tests the shared variables [create]", PTR_START_OF(tst_sharing_1)},
		{ "tshr2", "Tests the shared variables [create, get and perms]", PTR_START_OF(tst_sharing_2master)},
		{ "shr2Slave1", "[Slave program1] of tst_sharing_2master", PTR_START_OF(tst_sharing_2slave1)},
		{ "shr2Slave2", "[Slave program2] of tst_sharing_2master", PTR_START_OF(tst_sharing_2slave2)},
		{ "tshr3", "Tests the shared variables [Special cases of create]", PTR_START_OF(tst_sharing_3)},

		//[2] PROGRAMS
		{ "fact", "Factorial Recursive", PTR_START_OF(fos_factorial)},
		{ "fib", "Fibonacci Recursive", PTR_START_OF(fos_fibonacci)},
		{ "qs1", "Quicksort with NO memory leakage", PTR_START_OF(quicksort_noleakage)},
		{ "qs2", "Quicksort that cause memory leakage", PTR_START_OF(quicksort_leakage)},
		{ "ms1", "Mergesort with NO memory leakage", PTR_START_OF(mergesort_noleakage)},
		{ "ms2", "Mergesort that cause memory leakage", PTR_START_OF(mergesort_leakage)},

		{ "arrop", "Apply set of array operations: scenario program to test shared objects", PTR_START_OF(arrayOperations_Master)},
		{ "slave_qs", "SlaveOperation: quicksort", PTR_START_OF(arrayOperations_quicksort)},
		{ "slave_ms", "SlaveOperation: mergesort", PTR_START_OF(arrayOperations_mergesort)},
		{ "slave_stats", "SlaveOperation: stats", PTR_START_OF(arrayOperations_stats)},

		{ "tair", "", PTR_START_OF(tst_air)},
		{ "taircl", "", PTR_START_OF(tst_air_clerk)},
		{ "taircu", "", PTR_START_OF(tst_air_customer)},

		{ "midterm", "Midterm 2017: Example on shared resource and dependency", PTR_START_OF(MidTermEx_Master)},
		{ "midterm_a", "Midterm 2017 Example: Process A", PTR_START_OF(MidTermEx_ProcessA)},
		{ "midterm_b", "Midterm 2017 Example: Process B", PTR_START_OF(MidTermEx_ProcessB)},

		//[3] BONUSES
		{ "tshr4", "Tests the free of shared variables after createSharedObject only", PTR_START_OF(tst_sharing_4)},
		{ "tshr5", "Tests the free of shared variables after both createSharedObject and getSharedObject", PTR_START_OF(tst_sharing_5_master)},
		{ "tshr5slave", "Slave program to be used with tshr5", PTR_START_OF(tst_sharing_5_slave)},
		{ "tshr5slaveB1", "Slave program to be used with tshr5", PTR_START_OF(tst_sharing_5_slaveB1)},
		{ "tshr5slaveB2", "Slave program to be used with tshr5", PTR_START_OF(tst_sharing_5_slaveB2)},
		{ "tf3", "tests free (3): freeing buffers, tables, WS and page file [REplacement case]", PTR_START_OF(tst_free_3)},
};

///=========================================================

// To be used as extern in other files
struct UserProgramInfo* ptr_UserPrograms = &userPrograms[0];

// Number of user programs in the program table
int NUM_USER_PROGS = (sizeof(userPrograms)/sizeof(userPrograms[0]));

struct UserProgramInfo* get_user_program_info(char* user_program_name)
{
	int i;
	for (i = 0; i < NUM_USER_PROGS; i++) {
		if (strcmp(user_program_name, userPrograms[i].name) == 0)
			break;
	}
	if(i==NUM_USER_PROGS)
	{
		cprintf("Unknown user program '%s'\n", user_program_name);
		return 0;
	}

	return &userPrograms[i];
}

struct UserProgramInfo* get_user_program_info_by_env(struct Env* e)
{
	int i;
	for (i = 0; i < NUM_USER_PROGS; i++) {
		if ( strcmp( e->prog_name , userPrograms[i].name) ==0)
			break;
	}
	if(i==NUM_USER_PROGS)
	{
		cprintf("Unknown user program \n");
		return 0;
	}

	return &userPrograms[i];
}
