#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>
#include <kern/cpu/cpu.h>
#include <kern/cpu/picirq.h>


uint32 isSchedMethodRR(){if(scheduler_method == SCH_RR) return 1; return 0;}
uint32 isSchedMethodMLFQ(){if(scheduler_method == SCH_MLFQ) return 1; return 0;}
uint32 isSchedMethodBSD(){if(scheduler_method == SCH_BSD) return 1; return 0;}

//===================================================================================//
//============================ SCHEDULER FUNCTIONS ==================================//
//===================================================================================//
static struct Env* (*sched_next[])(void) = {
[SCH_RR]    fos_scheduler_RR,
[SCH_MLFQ]  fos_scheduler_MLFQ,
[SCH_BSD]   fos_scheduler_BSD,
};

//===================================
// [1] Default Scheduler Initializer:
//===================================
void sched_init()
{
	old_pf_counter = 0;

	sched_init_RR(INIT_QUANTUM_IN_MS);

	init_queue(&ProcessQueues.env_new_queue);
	init_queue(&ProcessQueues.env_exit_queue);

	mycpu()->scheduler_status = SCH_STOPPED;

	/*2024: initialize lock to protect these Qs in MULTI-CORE case only*/
	init_spinlock(&ProcessQueues.qlock, "process queues lock");
}

//=========================
// [2] Main FOS Scheduler:
//=========================

void
fos_scheduler(void)
{
	//ensure that the scheduler is invoked while interrupt is disabled
	if (read_eflags() & FL_IF)
		panic("fos_scheduler: called while the interrupt is enabled!");

	//cprintf("inside scheduler - timer cnt = %d\n", kclock_read_cnt0());
	struct Env *p;
	struct cpu *c = mycpu();
	c->proc = 0;

	chk1();
	c->scheduler_status = SCH_STARTED;

	//This variable should be set to the next environment to be run (if any)
	struct Env* next_env = NULL;

	//2024: should be outer loop as long as there's any BLOCKED processes.
	//Ref: xv6-x86 OS
	int is_any_blocked = 0;
	do
	{
		// Enable interrupts on this processor for a while to allow BLOCKED process to resume
		// The most recent process to run may have had interrupts turned off; enable them
		// to avoid a deadlock if all processes are waiting.
		sti();

		// Check ready queue(s) looking for process to run.
		//cprintf("\n[FOS_SCHEDULER] acquire: lock status before acquire = %d\n", qlock.locked);
		acquire_spinlock(&(ProcessQueues.qlock));  //lock: to protect ready & blocked Qs in multi-CPU
		//cprintf("ACQUIRED\n");
		do
		{
			//Get next env according to the current scheduler
			next_env = sched_next[scheduler_method]() ;

			//temporarily set the curenv by the next env JUST for checking the scheduler
			//Then: reset it again
			struct Env* old_curenv = get_cpu_proc();
			set_cpu_proc(next_env) ;
			chk2(next_env) ;
			set_cpu_proc(old_curenv) ;

			//sched_print_all();

			if(next_env != NULL)
			{
				//cprintf("\nScheduler select program '%s' [%d]... clock counter = %d\n", next_env->prog_name, next_env->env_id, kclock_read_cnt0());

				/*2024: Replaced by context_switch()*/
				//env_run(next_env);

				// Switch to chosen process. It is the process's job to release qlock
				// and then reacquire it before jumping back to us.
				set_cpu_proc(next_env);
				switchuvm(next_env);

				//Change its status to RUNNING
				next_env->env_status = ENV_RUNNING;

				//Context switch to it
				context_switch(&(c->scheduler), next_env->context);

				//ensure that the scheduler is invoked while interrupt is disabled
				if (read_eflags() & FL_IF)
					panic("fos_scheduler: invoked while the interrupt is enabled!");

				//Stop the clock now till finding a next proc (if any).
				//This is to avoid clock interrupt inside the scheduler after sti() of the outer loop
				kclock_stop();
				//cprintf("\n[IEN = %d] clock is stopped! returned to scheduler after context_switch. curenv = %d\n", (read_eflags() & FL_IF) == 0? 0:1, curenv == NULL? 0 : curenv->env_id);

				// Process is done running for now. It should have changed its p->status before coming back.
				//If no process on CPU, switch to the kernel
				assert(get_cpu_proc() == c->proc);
				int status = c->proc->env_status ;
				assert(status != ENV_RUNNING);
				if (status == ENV_READY)
				{
					//OK... will be placed to the correct ready Q in the next iteration
				}
				else
				{
					//					cprintf("scheduler: process %d is BLOCKED/EXITED\n", c->proc->env_id);
					switchkvm();
					set_cpu_proc(NULL);
				}
			}
		} while(next_env);

		//2024 - check if there's any blocked process?
		is_any_blocked = 0;
		for (int i = 0; i < NENV; ++i)
		{
			if (envs[i].env_status == ENV_BLOCKED)
			{
				is_any_blocked = 1;
				break;
			}
		}
		release_spinlock(&ProcessQueues.qlock);  //release lock: to protect ready & blocked Qs in multi-CPU
		//cprintf("\n[FOS_SCHEDULER] release: lock status after = %d\n", qlock.locked);

	} while (is_any_blocked > 0);

	/*2015*///No more envs... curenv doesn't exist any more! return back to command prompt
	{
		//cprintf("[sched] no envs - nothing more to do!\n");
		get_into_prompt();
	}

}

//=============================
// [3] Initialize RR Scheduler:
//=============================
void sched_init_RR(uint8 quantum)
{

	// Create 1 ready queue for the RR
	num_of_ready_queues = 1;
#if USE_KHEAP
	sched_delete_ready_queues();
	ProcessQueues.env_ready_queues = kmalloc(sizeof(struct Env_Queue));
	//cprintf("sizeof(struct Env_Queue) = %x\n", sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
	//cprintf("num_of_ready_queues * sizeof(uint8) = %x\n", num_of_ready_queues * sizeof(uint8));

#endif
	quantums[0] = quantum;
	kclock_set_quantum(quantums[0]);
	init_queue(&(ProcessQueues.env_ready_queues[0]));
	//=========================================
	//DON'T CHANGE THESE LINES=================
	uint16 cnt0 = kclock_read_cnt0_latch() ; //read after write to ensure it's set to the desired value
	cprintf("*	RR scheduler with initial clock = %d\n", cnt0);
	mycpu()->scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_RR;
	//=========================================
	//=========================================
}

//===============================
// [4] Initialize MLFQ Scheduler:
//===============================
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel)
{
	//=========================================
	//DON'T CHANGE THESE LINES=================
	sched_delete_ready_queues();
	//=========================================
	//=========================================
	//[PROJECT] MLFQ Scheduler - sched_init_MLFQ
	//Your code is here
	//Comment the following line
	panic("Not implemented yet");


	//=========================================
	//DON'T CHANGE THESE LINES=================
	uint16 cnt0 = kclock_read_cnt0_latch() ; //read after write to ensure it's set to the desired value
	cprintf("*	MLFQ scheduler with initial clock = %d\n", cnt0);
	mycpu()->scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_MLFQ;
	//=========================================
	//=========================================

}

//===============================
// [5] Initialize BSD Scheduler:
//===============================
void sched_init_BSD(uint8 numOfLevels, uint8 quantum)
{
	//[PROJECT] BSD Scheduler - sched_init_BSD
	//Your code is here
	//Comment the following line
	panic("Not implemented yet");


	//=========================================
	//DON'T CHANGE THESE LINES=================
	uint16 cnt0 = kclock_read_cnt0_latch() ; //read after write to ensure it's set to the desired value
	cprintf("*	BSD scheduler with initial clock = %d\n", cnt0);
	mycpu()->scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_BSD;
	//=========================================
	//=========================================
}


//=========================
// [6] RR Scheduler:
//=========================
struct Env* fos_scheduler_RR()
{
	// Implement simple round-robin scheduling.
	// Pick next environment from the ready queue,
	// and switch to such environment if found.
	// It's OK to choose the previously running env if no other env
	// is runnable.
	/*To protect process Qs (or info of current process) in multi-CPU************************/
	if(!holding_spinlock(&ProcessQueues.qlock))
		panic("fos_scheduler_RR: q.lock is not held by this CPU while it's expected to be.");
	/****************************************************************************************/
	struct Env *next_env = NULL;
	struct Env *cur_env = get_cpu_proc();
	//If the curenv is still exist, then insert it again in the ready queue
	if (cur_env != NULL)
	{
		enqueue(&(ProcessQueues.env_ready_queues[0]), cur_env);
	}

	//Pick the next environment from the ready queue
	next_env = dequeue(&(ProcessQueues.env_ready_queues[0]));

	//Reset the quantum
	//2017: Reset the value of CNT0 for the next clock interval
	kclock_set_quantum(quantums[0]);
	//uint16 cnt0 = kclock_read_cnt0_latch() ;
	//cprintf("CLOCK INTERRUPT AFTER RESET: Counter0 Value = %d\n", cnt0 );

	return next_env;
}

//=========================
// [6] MLFQ Scheduler:
//=========================
struct Env* fos_scheduler_MLFQ()
{
	//Apply the MLFQ with the specified levels to pick up the next environment
	//Note: the "curenv" (if exist) should be placed in its correct queue
	/*To protect process Qs (or info of current process) in multi-CPU************************/
	if(!holding_spinlock(&ProcessQueues.qlock))
		panic("fos_scheduler_MLFQ: q.lock is not held by this CPU while it's expected to be.");
	/****************************************************************************************/

	//[PROJECT] MLFQ Scheduler - fos_scheduler_MLFQ
	//Your code is here
	//Comment the following line
	panic("Not implemented yet");

}

//=========================
// [7] BSD Scheduler:
//=========================
struct Env* fos_scheduler_BSD()
{
	/*To protect process Qs (or info of current process) in multi-CPU************************/
	if(!holding_spinlock(&ProcessQueues.qlock))
		panic("fos_scheduler_BSD: q.lock is not held by this CPU while it's expected to be.");
	/****************************************************************************************/

	//[PROJECT] BSD Scheduler - fos_scheduler_BSD
	//Your code is here
	//Comment the following line
	panic("Not implemented yet");

}

//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================
void clock_interrupt_handler(struct Trapframe* tf)
{
	if (isSchedMethodBSD())
	{
		//[PROJECT] BSD Scheduler - clock_interrupt_handler
		//Your code is here
		//Comment the following line
		panic("Not implemented yet");
	}



	/********DON'T CHANGE THESE LINES***********/
	ticks++ ;
	struct Env* p = get_cpu_proc();
	if (p == NULL)
	{
	}
	else
	{
		p->nClocks++ ;
		if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
		{
			update_WS_time_stamps();
		}
		//cprintf("\n***************\nClock Handler\n***************\n") ;
		//fos_scheduler();
		yield();
	}
	/*****************************************/
}

//===================================================================
// [9] Update LRU Timestamp of WS Elements
//	  (Automatically Called Every Quantum in case of LRU Time Approx)
//===================================================================
void update_WS_time_stamps()
{
	struct Env *curr_env_ptr = get_cpu_proc();

	if(curr_env_ptr != NULL)
	{
		struct WorkingSetElement* wse ;
		{
			int i ;
#if USE_KHEAP
			LIST_FOREACH(wse, &(curr_env_ptr->page_WS_list))
			{
#else
				for (i = 0 ; i < (curr_env_ptr->page_WS_max_size); i++)
				{
					wse = &(curr_env_ptr->ptr_pageWorkingSet[i]);
					if( wse->empty == 1)
						continue;
#endif
					//update the time if the page was referenced
					uint32 page_va = wse->virtual_address ;
					uint32 perm = pt_get_page_permissions(curr_env_ptr->env_page_directory, page_va) ;
					uint32 oldTimeStamp = wse->time_stamp;

					if (perm & PERM_USED)
					{
						wse->time_stamp = (oldTimeStamp>>2) | 0x80000000;
						pt_set_page_permissions(curr_env_ptr->env_page_directory, page_va, 0 , PERM_USED) ;
					}
					else
					{
						wse->time_stamp = (oldTimeStamp>>2);
					}
				}
			}

			{
				int t ;
				for (t = 0 ; t < __TWS_MAX_SIZE; t++)
				{
					if( curr_env_ptr->__ptr_tws[t].empty != 1)
					{
						//update the time if the page was referenced
						uint32 table_va = curr_env_ptr->__ptr_tws[t].virtual_address;
						uint32 oldTimeStamp = curr_env_ptr->__ptr_tws[t].time_stamp;

						if (pd_is_table_used(curr_env_ptr->env_page_directory, table_va))
						{
							curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2) | 0x80000000;
							pd_set_table_unused(curr_env_ptr->env_page_directory, table_va);
						}
						else
						{
							curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2);
						}
					}
				}
			}
		}
	}

