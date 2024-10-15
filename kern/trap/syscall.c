/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/semaphore.h>

#include <kern/proc/user_environment.h>
#include "trap.h"
#include "syscall.h"
#include <kern/cons/console.h>
#include <kern/conc/channel.h>
#include <kern/cpu/sched.h>
#include <kern/cpu/cpu.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/mem/memory_manager.h>
#include <kern/mem/shared_memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/tests/test_working_set.h>

extern uint8 bypassInstrLength ;
struct Env* cur_env ;
/*******************************/
/* STRING I/O SYSTEM CALLS */
/*******************************/

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void sys_cputs(const char *s, uint32 len, uint8 printProgName)
{
	//2024 - better to use locks instead (to support multiprocessors)
	pushcli();	//disable interrupts
	{
		// Check that the user has permission to read memory [s, s+len).
		// Destroy the environment if not.

		// LAB 3: Your code here.

		// Print the string supplied by the user.
		if (printProgName)
			cprintf("[%s %d] ",cur_env->prog_name, cur_env->env_id);
		cprintf("%.*s",len, s);
	}
	popcli();	//enable interrupts
}


// Print a char to the system console.
static void sys_cputc(const char c)
{
	// Print the char supplied by the user.
	cprintf("%c",c);
}


// Read a character from the system console.
// Returns the character.
static int
sys_cgetc(void)
{
	int c;
	int IEN = read_eflags() & FL_IF;

	if (IEN) /*Interrupt-Enabled I/O*/
	{
		// The cons_getc2() primitive doesn't wait for a character
		while ((c = cons_getc2()) == 0)
		{
			//should sleep (i.e. blocked) until a IRQ1 (KB) interrupt occur
			if (KBD_INT_BLK_METHOD == LCK_SLEEP)
			{
				acquire_spinlock(&KBDlock);
				{
					sleep(&KBDchannel, &KBDlock);
				}
				release_spinlock(&KBDlock);
			}
			else if (KBD_INT_BLK_METHOD == LCK_SEMAPHORE)
			{
				wait_ksemaphore(&KBDsem);
			}
		}
	}
	else	/*Programmed I/O*/
	{
		//cprintf("\n(((((((Programmed I/O))))))\n");
		// The cons_getc() primitive doesn't wait for a character,
		// but the sys_cgetc() system call does.
		while ((c = cons_getc()) == 0)
		{
			//cprintf("do nothing\n");
			/* do nothing */;
		}
	}
	//cprintf("\nCHAR %d is READ from KB, IEN = %d\n", c, read_eflags() & FL_IF);

	return c;
}

//Lock the console so that no other processes can read from KB or output to the monitor
void sys_lock_cons(void)
{
	cons_lock();
}
//Unlock the console so that other processes can read from KB or output to the monitor
void sys_unlock_cons(void)
{
	cons_unlock();
}

/*******************************/
/* MEMORY SYSTEM CALLS */
/*******************************/
// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.
//
// Return 0 on success, < 0 on error.  Errors are:
//	E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	E_INVAL if va >= UTOP, or va is not page-aligned.
//	E_INVAL if perm is inappropriate (see above).
//	E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int __sys_allocate_page(void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	int r;
	struct Env *e = cur_env;

	//if ((r = envid2env(envid, &e, 1)) < 0)
	//return r;

	struct FrameInfo *ptr_frame_info ;
	r = allocate_frame(&ptr_frame_info) ;
	if (r == E_NO_MEM)
		return r ;

	//check virtual address to be paged_aligned and < USER_TOP
	if ((uint32)va >= USER_TOP || (uint32)va % PAGE_SIZE != 0)
		return E_INVAL;

	//check permissions to be appropriate
	if ((perm & (~PERM_AVAILABLE & ~PERM_WRITEABLE)) != (PERM_USER))
		return E_INVAL;


	uint32 physical_address = to_physical_address(ptr_frame_info) ;

#if USE_KHEAP
	{
		//FIX: we should implement a better solution for this, but for now
		//		we are using an unsed VA in the invalid area of kernel at 0xef800000 (the current USER_LIMIT)
		//		to do temp initialization of a frame.
		map_frame(e->env_page_directory, ptr_frame_info, USER_LIMIT, PERM_WRITEABLE);
		memset((void*)USER_LIMIT, 0, PAGE_SIZE);

		// Temporarily increase the references to prevent unmap_frame from removing the frame
		// we just got from allocate_frame, we will use it for the new page
		ptr_frame_info->references += 1;
		unmap_frame(e->env_page_directory, USER_LIMIT);

		//return it to the original status
		ptr_frame_info->references -= 1;
	}
#else
	{
		memset(STATIC_KERNEL_VIRTUAL_ADDRESS(physical_address), 0, PAGE_SIZE);
	}
#endif
	r = map_frame(e->env_page_directory, ptr_frame_info, (uint32)va, perm) ;
	if (r == E_NO_MEM)
	{
		decrement_references(ptr_frame_info);
		return r;
	}
	return 0 ;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int __sys_map_frame(int32 srcenvid, void *srcva, int32 dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	panic("sys_map_frame not implemented");
	return 0;

}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int __sys_unmap_frame(int32 envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	panic("sys_page_unmap not implemented");
	return 0;
}

uint32 sys_calculate_required_frames(uint32 start_virtual_address, uint32 size)
{
	return calculate_required_frames(cur_env->env_page_directory, start_virtual_address, size);
}

uint32 sys_calculate_free_frames()
{
	struct freeFramesCounters counters = calculate_available_frames();
	//	cprintf("Free Frames = %d : Buffered = %d, Not Buffered = %d\n", counters.freeBuffered + counters.freeNotBuffered, counters.freeBuffered ,counters.freeNotBuffered);
	return counters.freeBuffered + counters.freeNotBuffered;
}
uint32 sys_calculate_modified_frames()
{
	struct freeFramesCounters counters = calculate_available_frames();
	//	cprintf("================ Modified Frames = %d\n", counters.modified) ;
	return counters.modified;
}

uint32 sys_calculate_notmod_frames()
{
	struct freeFramesCounters counters = calculate_available_frames();
	//	cprintf("================ Not Modified Frames = %d\n", counters.freeBuffered) ;
	return counters.freeBuffered;
}

int sys_calculate_pages_tobe_removed_ready_exit(uint32 WS_or_MEMORY_flag)
{
	return calc_no_pages_tobe_removed_from_ready_exit_queues(WS_or_MEMORY_flag);
}

void sys_scarce_memory(void)
{
	scarce_memory();
}

void sys_clearFFL()
{
	int size;
	acquire_spinlock(&MemFrameLists.mfllock);
	{
		size = LIST_SIZE(&MemFrameLists.free_frame_list) ;
		struct FrameInfo* ptr_tmp_FI ;
		for (int i = 0; i < size ; i++)
		{
			allocate_frame(&ptr_tmp_FI) ;
		}
	}
	release_spinlock(&MemFrameLists.mfllock);
}

/*******************************/
/* PAGE FILE SYSTEM CALLS */
/*******************************/
int sys_pf_calculate_allocated_pages(void)
{
	return pf_calculate_allocated_pages(cur_env);
}

/*******************************/
/* USER HEAP SYSTEM CALLS */
/*******************************/
void sys_free_user_mem(uint32 virtual_address, uint32 size)
{
	if(isBufferingEnabled())
	{
		__free_user_mem_with_buffering(cur_env, virtual_address, size);
	}
	else
	{
		free_user_mem(cur_env, virtual_address, size);
	}
	return;
}

void sys_allocate_user_mem(uint32 virtual_address, uint32 size)
{
	//TODO: [PROJECT'24.MS1 - #03] [2] SYSTEM CALLS - Params Validation

	allocate_user_mem(cur_env, virtual_address, size);
	return;
}

void sys_allocate_chunk(uint32 virtual_address, uint32 size, uint32 perms)
{
	//TODO: [PROJECT'24.MS1 - #03] [2] SYSTEM CALLS - Params Validation

	allocate_chunk(cur_env->env_page_directory, virtual_address, size, perms);
	return;
}

//2014
void sys_move_user_mem(uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	move_user_mem(cur_env, src_virtual_address, dst_virtual_address, size);
	return;
}

//2015
uint32 sys_get_heap_strategy()
{
	return _UHeapPlacementStrategy ;
}
void sys_set_uheap_strategy(uint32 heapStrategy)
{
	_UHeapPlacementStrategy = heapStrategy;
}

/*******************************/
/* SEMAPHORES SYSTEM CALLS */
/*******************************/
//[PROJECT'24.MS3] ADD SUITABLE CODE HERE


/*******************************/
/* SHARED MEMORY SYSTEM CALLS */
/*******************************/
int sys_createSharedObject(char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	return createSharedObject(cur_env->env_id, shareName, size, isWritable, virtual_address);
}

int sys_getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	return getSizeOfSharedObject(ownerID, shareName);
}

int sys_getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	return getSharedObject(ownerID, shareName, virtual_address);
}

int sys_freeSharedObject(int32 sharedObjectID, void *startVA)
{
	return freeSharedObject(sharedObjectID, startVA);
}

/*********************************/
/* USER ENVIRONMENT SYSTEM CALLS */
/*********************************/
// Returns the current environment's envid.
//2017
static int32 sys_getenvid(void)
{
	return cur_env->env_id;
}

//2017
static int32 sys_getenvindex(void)
{
	//return cur_env->env_id;
	return (cur_env - envs) ;
}

//2017
static int32 sys_getparentenvid(void)
{
	return cur_env->env_parent_id;
}

// Destroy a given environment whatever its state & DON'T place it in EXIT
// if envid=0, destroy the currently running environment --> schedule the next (if any)
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int sys_destroy_env(int32 envid)
{
	int r;
	struct Env *e;
	if (envid == 0)
	{
		e = cur_env ;
	}
	else if ((r = envid2env(envid, &e, 0)) < 0)
	{
		return r;
	}

	if (e == cur_env)
	{
		cprintf("[%08x] exiting gracefully\n", cur_env->env_id);
	}
	else
	{
		cprintf("[%08x] destroying %08x\n", cur_env->env_id, e->env_id);
	}
	//2015
	sched_kill_env(e->env_id);

	return 0;
}

//Just place the current env into the EXIT queue & schedule the next one
static void sys_exit_env()
{
	//2015
	env_exit();

	//2024: if returned here, then it's not the current environment. So, just return
	//env_run_cmd_prmpt();
	//context_switch(&(cur_env->context), mycpu()->scheduler);

}

//New update in 2020
//Create a new env & add it to the NEW queue
int sys_create_env(char* programName, unsigned int page_WS_size,unsigned int LRU_second_list_size, unsigned int percent_WS_pages_to_remove)
{
	//cprintf("\nAttempt to create a new env\n");

	struct Env* env =  env_create(programName, page_WS_size, LRU_second_list_size, percent_WS_pages_to_remove);
	if(env == NULL)
	{
		return E_ENV_CREATION_ERROR;
	}
	//cprintf("\nENV %d is created\n", env->env_id);

	//2015
	sched_new_env(env);

	//cprintf("\nENV %d is scheduled as NEW\n", env->env_id);

	return env->env_id;
}

//Place a new env into the READY queue
void sys_run_env(int32 envId)
{
	sched_run_env(envId);
}


//====================================
/*******************************/
/* ETC... SYSTEM CALLS */
/*******************************/

struct uint64 sys_get_virtual_time()
{
	struct uint64 t = get_virtual_time();
	return t;
}

uint32 sys_rcr2()
{
	return rcr2();
}
void sys_bypassPageFault(uint8 instrLength)
{
	bypassInstrLength = instrLength;
}


/**************************************************************************/
/************************* SYSTEM CALLS HANDLER ***************************/
/**************************************************************************/
// Dispatches to the correct kernel function, passing the arguments.
uint32 syscall(uint32 syscallno, uint32 a1, uint32 a2, uint32 a3, uint32 a4, uint32 a5)
{
	cur_env = get_cpu_proc();
	assert(cur_env != NULL);

	//cprintf("syscallno = %d\n", syscallno);
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	switch(syscallno)
	{
	//TODO: [PROJECT'24.MS1 - #02] [2] SYSTEM CALLS - Add suitable code here

	//======================================================================
	case SYS_cputs:
		sys_cputs((const char*)a1,a2,(uint8)a3);
		return 0;
		break;
	case SYS_cgetc:
		return sys_cgetc();
		break;
	case SYS_lock_cons:
		sys_lock_cons();
		return 0;
		break;
	case SYS_unlock_cons:
		sys_unlock_cons();
		return 0;
		break;
	case SYS_calc_req_frames:
		return sys_calculate_required_frames(a1, a2);
		break;
	case SYS_calc_free_frames:
		return sys_calculate_free_frames();
		break;
	case SYS_calc_modified_frames:
		return sys_calculate_modified_frames();
		break;
	case SYS_calc_notmod_frames:
		return sys_calculate_notmod_frames();
		break;

	case SYS_pf_calc_allocated_pages:
		return sys_pf_calculate_allocated_pages();
		break;
	case SYS_calculate_pages_tobe_removed_ready_exit:
		return sys_calculate_pages_tobe_removed_ready_exit(a1);
		break;
	case SYS_scarce_memory:
		sys_scarce_memory();
		return 0;
		break;
	case SYS_allocate_chunk_in_mem:
		sys_allocate_chunk(a1, (uint32)a2, a3);
		return 0;
		break;

		//======================
	case SYS_allocate_page:
		__sys_allocate_page((void*)a1, a2);
		return 0;
		break;
	case SYS_map_frame:
		__sys_map_frame(a1, (void*)a2, a3, (void*)a4, a5);
		return 0;
		break;
	case SYS_unmap_frame:
		__sys_unmap_frame(a1, (void*)a2);
		return 0;
		break;

	case SYS_cputc:
		sys_cputc((const char)a1);
		return 0;
		break;

	case SYS_clearFFL:
		sys_clearFFL((const char)a1);
		return 0;
		break;

	case SYS_create_shared_object:
		return sys_createSharedObject((char*)a1, a2, a3, (void*)a4);
		break;

	case SYS_get_shared_object:
		return sys_getSharedObject((int32)a1, (char*)a2, (void*)a3);
		break;

	case SYS_free_shared_object:
		return sys_freeSharedObject((int32)a1, (void *)a2);
		break;

	case SYS_get_size_of_shared_object:
		return sys_getSizeOfSharedObject((int32)a1, (char*)a2);
		break;

	case SYS_create_env:
		return sys_create_env((char*)a1, (uint32)a2, (uint32)a3, (uint32)a4);
		break;

	case SYS_run_env:
		sys_run_env((int32)a1);
		return 0;
		break;
	case SYS_getenvindex:
		return sys_getenvindex();
		break;
	case SYS_getenvid:
		return sys_getenvid();
		break;
	case SYS_getparentenvid:
		return sys_getparentenvid();
		break;
	case SYS_destroy_env:
		return sys_destroy_env(a1);
		break;
	case SYS_exit_env:
		sys_exit_env();
		return 0;
		break;
	case SYS_get_virtual_time:
	{
		struct uint64 res = sys_get_virtual_time();
		uint32* ptrlow = ((uint32*)a1);
		uint32* ptrhi = ((uint32*)a2);
		*ptrlow = res.low;
		*ptrhi = res.hi;
		return 0;
		break;
	}
	case SYS_move_user_mem:
		sys_move_user_mem(a1, a2, a3);
		return 0;
		break;
	case SYS_rcr2:
		return sys_rcr2();
		break;
	case SYS_bypassPageFault:
		sys_bypassPageFault(a1);
		return 0;

	case SYS_rsttst:
		rsttst();
		return 0;
	case SYS_inctst:
		inctst();
		return 0;
	case SYS_chktst:
		chktst(a1);
		return 0;
	case SYS_gettst:
		return gettst();
	case SYS_testNum:
		tst(a1, a2, a3, (char)a4, a5);
		return 0;

	case SYS_get_heap_strategy:
		return sys_get_heap_strategy();

	case SYS_set_heap_strategy:
		sys_set_uheap_strategy(a1);
		return 0;

	case SYS_check_LRU_lists:
		return sys_check_LRU_lists((uint32*)a1, (uint32*)a2, (int)a3, (int)a4);

	case SYS_check_LRU_lists_free:
		return sys_check_LRU_lists_free((uint32*)a1, (int)a2);

	case SYS_check_WS_list:
		return sys_check_WS_list((uint32*)a1, (int)a2, (uint32)a3, (bool)a4);

	case SYS_utilities:
		sys_utilities((char*)a1, (int)a2);
		return 0;

	case NSYSCALLS:
		return 	-E_INVAL;
		break;
	}
	//panic("syscall not implemented");
	return -E_INVAL;
}
