#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>
#include <kern/cpu/cpu.h>

//void on_clock_update_WS_time_stamps();
extern void cleanup_buffers(struct Env* e);
//================

//=================================================================================//
//============================== QUEUE FUNCTIONS ==================================//
//=================================================================================//

//================================
// [1] Initialize the given queue:
//================================
void init_queue(struct Env_Queue* queue)
{
	if(queue != NULL)
	{
		LIST_INIT(queue);
	}
}

//================================
// [2] Get queue size:
//================================
int queue_size(struct Env_Queue* queue)
{
	if(queue != NULL)
	{
		return LIST_SIZE(queue);
	}
	else
	{
		return 0;
	}
}

//====================================
// [3] Enqueue env in the given queue:
//====================================
void enqueue(struct Env_Queue* queue, struct Env* env)
{
	assert(queue != NULL)	;
	if(env != NULL)
	{
		LIST_INSERT_HEAD(queue, env);
	}
}

//======================================
// [4] Dequeue env from the given queue:
//======================================
struct Env* dequeue(struct Env_Queue* queue)
{
	if (queue == NULL) return NULL;
	struct Env* envItem = LIST_LAST(queue);
	if (envItem != NULL)
	{
		LIST_REMOVE(queue, envItem);
	}
	return envItem;
}

//====================================
// [5] Remove env from the given queue:
//====================================
void remove_from_queue(struct Env_Queue* queue, struct Env* e)
{
	assert(queue != NULL)	;

	if (e != NULL)
	{
		LIST_REMOVE(queue, e);
	}
}

//========================================
// [6] Search by envID in the given queue:
//========================================
struct Env* find_env_in_queue(struct Env_Queue* queue, uint32 envID)
{
	if (queue == NULL) return NULL;

	struct Env * ptr_env=NULL;
	LIST_FOREACH(ptr_env, queue)
	{
		if(ptr_env->env_id == envID)
		{
			return ptr_env;
		}
	}
	return NULL;
}

//=====================================================================================//
//============================== SCHED Q'S FUNCTIONS ==================================//
//=====================================================================================//

//========================================
// [1] Delete all ready queues:
//========================================
void sched_delete_ready_queues()
{
#if USE_KHEAP
	acquire_spinlock(&ProcessQueues.qlock);
	{
		if (ProcessQueues.env_ready_queues != NULL)
			kfree(ProcessQueues.env_ready_queues);
		if (quantums != NULL)
			kfree(quantums);
	}
	release_spinlock(&ProcessQueues.qlock);

#endif
}

//=================================================
// [2] Insert the given Env in the 1st Ready Queue:
//=================================================
void sched_insert_ready0(struct Env* env)
{
	/*To protect process Qs (or info of current process) in multi-CPU*/
	if(!holding_spinlock(&ProcessQueues.qlock))
		panic("sched: q.lock is not held by this CPU while it's expected to be.");
	/*********************************************************************/

	assert(env != NULL);
	{
		//cprintf("\nInserting %d into ready queue 0\n", env->env_id);
		env->env_status = ENV_READY ;
		enqueue(&(ProcessQueues.env_ready_queues[0]), env);
	}
}

//============================================================
// [2] Insert the given Env in the priority-based Ready Queue:
//============================================================
void sched_insert_ready(struct Env* env)
{
	/*To protect process Qs (or info of current process) in multi-CPU*/
	if(!holding_spinlock(&ProcessQueues.qlock))
		panic("sched: q.lock is not held by this CPU while it's expected to be.");
	/*********************************************************************/

	assert(env != NULL);
	{
		//cprintf("\nInserting %d into ready queue 0\n", env->env_id);
		env->env_status = ENV_READY ;
		enqueue(&(ProcessQueues.env_ready_queues[env->priority]), env);
	}
}

//=================================================
// [3] Remove the given Env from the Ready Queue(s):
//=================================================
void sched_remove_ready(struct Env* env)
{
	/*To protect process Qs (or info of current process) in multi-CPU*/
	if(!holding_spinlock(&ProcessQueues.qlock))
		panic("sched: q.lock is not held by this CPU while it's expected to be.");
	/*********************************************************************/

	assert(env != NULL && env->env_status == ENV_READY);
	{
		for (int i = 0 ; i < num_of_ready_queues ; i++)
		{
			struct Env * ptr_env = find_env_in_queue(&(ProcessQueues.env_ready_queues[i]), env->env_id);
			if (ptr_env != NULL)
			{
				LIST_REMOVE(&(ProcessQueues.env_ready_queues[i]), env);
				env->env_status = ENV_UNKNOWN;
				return ;
			}
		}
	}
}

//=================================================
// [4] Insert the given Env in NEW Queue:
//=================================================
void sched_insert_new(struct Env* env)
{
	/*To protect process Qs (or info of current process) in multi-CPU*/
	if(!holding_spinlock(&ProcessQueues.qlock))
		panic("sched: q.lock is not held by this CPU while it's expected to be.");
	/*********************************************************************/

	assert(env != NULL);
	{
		env->env_status = ENV_NEW ;
		enqueue(&ProcessQueues.env_new_queue, env);
	}
}

//=================================================
// [5] Remove the given Env from NEW Queue:
//=================================================
void sched_remove_new(struct Env* env)
{
	/*To protect process Qs (or info of current process) in multi-CPU*/
	if(!holding_spinlock(&ProcessQueues.qlock))
		panic("sched: q.lock is not held by this CPU while it's expected to be.");
	/*********************************************************************/

	assert(env != NULL && env->env_status == ENV_NEW);
	{
		LIST_REMOVE(&ProcessQueues.env_new_queue, env) ;
		env->env_status = ENV_UNKNOWN;
	}
}

//=================================================
// [6] Insert the given Env in EXIT Queue:
//=================================================
void sched_insert_exit(struct Env* env)
{
	/*To protect process Qs (or info of current process) in multi-CPU*/
	if(!holding_spinlock(&ProcessQueues.qlock))
		panic("sched: q.lock is not held by this CPU while it's expected to be.");
	/*********************************************************************/

	assert(env != NULL);
	{
		if(isBufferingEnabled()) {cleanup_buffers(env);}
		env->env_status = ENV_EXIT ;
		enqueue(&ProcessQueues.env_exit_queue, env);
	}
}
//=================================================
// [7] Remove the given Env from EXIT Queue:
//=================================================
void sched_remove_exit(struct Env* env)
{
	/*To protect process Qs (or info of current process) in multi-CPU*/
	if(!holding_spinlock(&ProcessQueues.qlock))
		panic("sched: q.lock is not held by this CPU while it's expected to be.");
	/*********************************************************************/

	assert(env != NULL && env->env_status == ENV_EXIT);
	{
		LIST_REMOVE(&ProcessQueues.env_exit_queue, env) ;
		env->env_status = ENV_UNKNOWN;
	}
}

//=================================================
// [8] Sched the given Env in NEW Queue:
//=================================================
void sched_new_env(struct Env* e)
{
	  //cprintf("\n[SCHED_NEW_ENV] acquire: lock status before acquire = %d\n", qlock.locked);
	acquire_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs

	//add the given env to the scheduler NEW queue
	assert (e!=NULL);
	{
		sched_insert_new(e);
	}

	release_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
	  //cprintf("\n[SCHED_NEW_ENV] release: lock status after = %d\n", qlock.locked);
}

//=================================================
// [9] Run the given EnvID:
//=================================================
void sched_run_env(uint32 envId)
{
	  //cprintf("\n[SCHED_RUN_ENV] acquire: lock status before acquire = %d\n", qlock.locked);
	acquire_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
	struct Env* ptr_env=NULL;
	LIST_FOREACH(ptr_env, &ProcessQueues.env_new_queue)
	{
		if(ptr_env->env_id == envId)
		{
			sched_remove_new(ptr_env);
			sched_insert_ready(ptr_env);

			/*2015*///if scheduler not run yet, then invoke it!
			if (mycpu()->scheduler_status == SCH_STOPPED)
			{
				release_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
				  //cprintf("\n[SCHED_RUN_ENV] release#1: lock status after = %d\n", qlock.locked);
				fos_scheduler();
			}
			else
			{
				//can be invoked from a running environment via sys_run_env(), so just release the lock and resume
			}
			break;
		}
	}
	release_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
	  //cprintf("\n[SCHED_RUN_ENV] release#2: lock status after = %d\n", qlock.locked);
}

//=================================================
// [10] Exit the given EnvID:
//=================================================
void sched_exit_env(uint32 envId)
{
	bool lock_already_held = holding_spinlock(&ProcessQueues.qlock);
	  //cprintf("\n[SCHED_EXIT_ENV] acquire: lock status before acquire = %d\n", qlock.locked);
	if (!lock_already_held)
	{
		acquire_spinlock(&ProcessQueues.qlock);
	}
	struct Env* ptr_env=NULL;
	int found = 0;
	if (!found)
	{
		LIST_FOREACH(ptr_env, &ProcessQueues.env_new_queue)
		{
			if(ptr_env->env_id == envId)
			{
				sched_remove_new(ptr_env);
				found = 1;
				//			return;
			}
		}
	}
	if (!found)
	{
		for (int i = 0 ; i < num_of_ready_queues ; i++)
		{
			if (!LIST_EMPTY(&(ProcessQueues.env_ready_queues[i])))
			{
				ptr_env=NULL;
				LIST_FOREACH(ptr_env, &(ProcessQueues.env_ready_queues[i]))
				{
					if(ptr_env->env_id == envId)
					{
						LIST_REMOVE(&(ProcessQueues.env_ready_queues[i]), ptr_env);
						found = 1;
						break;
					}
				}
			}
			if (found) break;
		}
	}
	struct Env* cur_env = get_cpu_proc();
	assert(cur_env != NULL);
	if (!found)
	{
		if (cur_env->env_id == envId)
		{
			ptr_env = cur_env;
			found = 1;
		}
	}

	if (found)
	{
		sched_insert_exit(ptr_env);

		//If it's the curenv, then reinvoke the scheduler as there's no meaning to return back
		//to an exited env. Status already set to EXIT in the sched_insert_exit()
		//It's the fos_scheduler task to release the lock on the Qs after context_switch to it from
		//this process
		if (cur_env->env_id == envId)
		{
			//2024: Replaced by sched() which call context switch
			//fos_scheduler();
			sched();
		}
	}
	if (!lock_already_held)
	{
		release_spinlock(&ProcessQueues.qlock);
	}
	//cprintf("\n[SCHED_EXIT_ENV] release: lock status after = %d\n", qlock.locked);
}


/*2015*/
//=================================================
// [11] KILL the given EnvID:
//=================================================
void sched_kill_env(uint32 envId)
{
	acquire_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
	struct Env* ptr_env=NULL;
	int found = 0;
	if (!found)
	{
		LIST_FOREACH(ptr_env, &ProcessQueues.env_new_queue)
		{
			if(ptr_env->env_id == envId)
			{
				cprintf("killing[%d] %s from the NEW queue...", ptr_env->env_id, ptr_env->prog_name);
				sched_remove_new(ptr_env);
				found = 1;
				break;
			}
		}
	}
	if (!found)
	{
		for (int i = 0 ; i < num_of_ready_queues ; i++)
		{
			if (!LIST_EMPTY(&(ProcessQueues.env_ready_queues[i])))
			{
				ptr_env=NULL;
				LIST_FOREACH(ptr_env, &(ProcessQueues.env_ready_queues[i]))
				{
					if(ptr_env->env_id == envId)
					{
						cprintf("killing[%d] %s from the READY queue #%d...", ptr_env->env_id, ptr_env->prog_name, i);
						LIST_REMOVE(&(ProcessQueues.env_ready_queues[i]), ptr_env);
						found = 1;
						break;
					}
				}
			}
			if (found)
				break;
		}
	}
	if (!found)
	{
		ptr_env=NULL;
		LIST_FOREACH(ptr_env, &ProcessQueues.env_exit_queue)
		{
			if(ptr_env->env_id == envId)
			{
				cprintf("killing[%d] %s from the EXIT queue...", ptr_env->env_id, ptr_env->prog_name);
				sched_remove_exit(ptr_env);
				found = 1;
				break;
			}
		}
	}
	release_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs

	if (found)
	{
		env_free(ptr_env);
		cprintf("DONE\n");
	}
	else
	{
		struct Env* cur_env = get_cpu_proc();
		assert(cur_env != NULL);

		if (cur_env->env_id == envId)
		{
			ptr_env = cur_env;
			assert(ptr_env->env_status == ENV_RUNNING);
			cprintf("killing a RUNNABLE environment [%d] %s...", ptr_env->env_id, ptr_env->prog_name);
			env_free(ptr_env);
			cprintf("DONE\n");
			found = 1;
			//If it's the curenv, then reset it and reinvoke the scheduler as there's no meaning to
			//return back to a killed env. Status already set to EXIT in the env_free()
			//It's the fos_scheduler task to release the lock on the Qs after context_switch to it from
			//this process
			/*2024: replaced by sched() to apply context_switch*/
			//lcr3(phys_page_directory);
			//switchkvm();
			//fos_scheduler();
			sched();
		}
	}

}

//=================================================
// [12] PRINT ALL Envs from all queues:
//=================================================
void sched_print_all()
{
	acquire_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
	struct Env* ptr_env ;
	if (!LIST_EMPTY(&ProcessQueues.env_new_queue))
	{
		cprintf("\nThe processes in NEW queue are:\n");
		LIST_FOREACH(ptr_env, &ProcessQueues.env_new_queue)
		{
			cprintf("	[%d] %s\n", ptr_env->env_id, ptr_env->prog_name);
		}
	}
	else
	{
		cprintf("\nNo processes in NEW queue\n");
	}
	cprintf("================================================\n");
	for (int i = 0 ; i < num_of_ready_queues ; i++)
	{
		if (!LIST_EMPTY(&(ProcessQueues.env_ready_queues[i])))
		{
			cprintf("The processes in READY queue #%d are:\n", i);
			LIST_FOREACH(ptr_env, &(ProcessQueues.env_ready_queues[i]))
			{
				cprintf("	[%d] %s\n", ptr_env->env_id, ptr_env->prog_name);
			}
		}
		else
		{
			cprintf("No processes in READY queue #%d\n", i);
		}
		cprintf("================================================\n");
	}
	if (!LIST_EMPTY(&ProcessQueues.env_exit_queue))
	{
		cprintf("The processes in EXIT queue are:\n");
		LIST_FOREACH(ptr_env, &ProcessQueues.env_exit_queue)
		{
			cprintf("	[%d] %s\n", ptr_env->env_id, ptr_env->prog_name);
		}
	}
	else
	{
		cprintf("No processes in EXIT queue\n");
	}
	release_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
}

//=================================================
// [13] MOVE ALL NEW Envs into READY Q:
//=================================================
void sched_run_all()
{
	acquire_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
	struct Env* ptr_env=NULL;

	/*2023: Changed from LIST_FOREACH into DEQUEUE (based on suggestion from T52 & T73 2023.Term1)
	 * to move the processes in FIFO order instead of LIFO in case of LIST_FOREACH
	 * */
	int q_size = LIST_SIZE(&ProcessQueues.env_new_queue);
	for (int i = 0; i < q_size; ++i)
	{
		ptr_env = dequeue(&ProcessQueues.env_new_queue);
		sched_insert_ready(ptr_env);
	}

	release_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
	/*2015*///if scheduler not run yet, then invoke it!
	if (mycpu()->scheduler_status == SCH_STOPPED)
		fos_scheduler();
	else
		panic("scheduler status is NOT STOPPED while it's expected to be!!");
}

//=================================================
// [14] KILL ALL Envs in the System:
//=================================================
void sched_kill_all()
{
	acquire_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
	struct Env* ptr_env ;
	if (!LIST_EMPTY(&ProcessQueues.env_new_queue))
	{
		cprintf("\nKILLING the processes in the NEW queue...\n");
		LIST_FOREACH(ptr_env, &ProcessQueues.env_new_queue)
		{
			cprintf("	killing[%d] %s...", ptr_env->env_id, ptr_env->prog_name);
			sched_remove_new(ptr_env);
			env_free(ptr_env);
			cprintf("DONE\n");
		}
	}
	else
	{
		cprintf("No processes in NEW queue\n");
	}
	cprintf("================================================\n");
	for (int i = 0 ; i < num_of_ready_queues ; i++)
	{
		if (!LIST_EMPTY(&(ProcessQueues.env_ready_queues[i])))
		{
			cprintf("KILLING the processes in the READY queue #%d...\n", i);
			LIST_FOREACH(ptr_env, &(ProcessQueues.env_ready_queues[i]))
			{
				cprintf("	killing[%d] %s...", ptr_env->env_id, ptr_env->prog_name);
				LIST_REMOVE(&(ProcessQueues.env_ready_queues[i]), ptr_env);
				env_free(ptr_env);
				cprintf("DONE\n");
			}
		}
		else
		{
			cprintf("No processes in READY queue #%d\n",i);
		}
		cprintf("================================================\n");
	}

	if (!LIST_EMPTY(&ProcessQueues.env_exit_queue))
	{
		cprintf("KILLING the processes in the EXIT queue...\n");
		LIST_FOREACH(ptr_env, &ProcessQueues.env_exit_queue)
		{
			cprintf("	killing[%d] %s...", ptr_env->env_id, ptr_env->prog_name);
			sched_remove_exit(ptr_env);
			env_free(ptr_env);
			cprintf("DONE\n");
		}
	}
	else
	{
		cprintf("No processes in EXIT queue\n");
	}

	struct Env* cur_env = get_cpu_proc();
	if (cur_env)
	{
		ptr_env = cur_env;
		assert(ptr_env->env_status == ENV_RUNNING);
		cprintf("killing a RUNNABLE environment [%d] %s...", ptr_env->env_id, ptr_env->prog_name);
		env_free(ptr_env);
		cprintf("DONE\n");
		//If it's the curenv, then reset it and reinvoke the scheduler as there's no meaning to
		//return back to a killed env. Status already set to EXIT in the env_free()
		//It's the fos_scheduler task to release the lock on the Qs after context_switch to it from
		//this process
		//reinvoke the scheduler since there're no env to return back to it
		/*2024: replaced by sched() to apply context_switch*/
		sched();
	}
	release_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
	//get into the command prompt since there're no env to return back to it
	//fos_scheduler(); //2024: commented
	get_into_prompt();

}

/*2018*/
//=================================================
// [14] EXIT ALL Ready Envs:
//=================================================
void sched_exit_all_ready_envs()
{
	acquire_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
	struct Env* ptr_env=NULL;
	for (int i = 0 ; i < num_of_ready_queues ; i++)
	{
		if (!LIST_EMPTY(&(ProcessQueues.env_ready_queues[i])))
		{
			ptr_env=NULL;
			LIST_FOREACH(ptr_env, &(ProcessQueues.env_ready_queues[i]))
			{
				LIST_REMOVE(&(ProcessQueues.env_ready_queues[i]), ptr_env);
				sched_insert_exit(ptr_env);
			}
		}
	}
	release_spinlock(&(ProcessQueues.qlock)); 	//CS on Qs
}

/*2023*/
/********* for BSD Priority Scheduler *************/
int64 timer_ticks()
{
	return ticks;
}
int env_get_nice(struct Env* e)
{
	//[PROJECT] BSD Scheduler - env_get_nice
	//Your code is here
	//Comment the following line
	panic("Not implemented yet");
}

void env_set_nice(struct Env* e, int nice_value)
{
	//[PROJECT] BSD Scheduler - env_set_nice
	//Your code is here
	//Comment the following line
	panic("Not implemented yet");
}

int env_get_recent_cpu(struct Env* e)
{
	//[PROJECT] BSD Scheduler - env_get_recent_cpu
	//Your code is here
	//Comment the following line
	panic("Not implemented yet");
}
int get_load_average()
{
	//return 1;
	//[PROJECT] BSD Scheduler - get_load_average
	//Your code is here
	//Comment the following line
	panic("Not implemented yet");
	return 0;
}
/********* for BSD Priority Scheduler *************/
//==================================================================================//

/*2024*/
/********* for Priority RR Scheduler *************/
void env_set_priority(int envID, int priority)
{
	//TODO: [PROJECT'24.MS3 - #06] [3] PRIORITY RR Scheduler - env_set_priority

	//Get the process of the given ID
	struct Env* proc ;
	envid2env(envID, &proc, 0);

	//Your code is here
	//Comment the following line
	panic("Not implemented yet");
}

void sched_set_starv_thresh(uint32 starvThresh)
{
	//TODO: [PROJECT'24.MS3 - #06] [3] PRIORITY RR Scheduler - sched_set_starv_thresh
	//Your code is here
	//Comment the following line
	panic("Not implemented yet");
}
