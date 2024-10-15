/* See COPYRIGHT for copyright information. */

#ifndef FOS_KERN_ENV_H
#define FOS_KERN_ENV_H

#include <inc/environment_definitions.h>
#include <kern/proc/user_programs.h>
#include <inc/types.h>
#include <kern/cpu/sched_helpers.h>
#include <kern/conc/spinlock.h>


//========================================================
extern struct UserProgramInfo* ptr_UserPrograms;
extern struct Env *envs;		// All environments
//extern struct Env *curenv;	// Current environment

///===================================================================================
struct Env* get_cpu_proc(void);			// get the the current running process on the CPU
void set_cpu_proc(struct Env* p);		// set the process to be run on CPU
int envid2env(int32  envid, struct Env **env_store, bool checkperm);

///===================================================================================
/*Initialize the envs array and add its elements to the list*/
void env_init(void);
/*Create new environment, initialize it, load the EXE into its memory and adjust its address space*/
struct Env* env_create(char* user_program_name, unsigned int page_WS_size, unsigned int LRU_second_list_size, unsigned int percent_WS_pages_to_remove);
/*Free (delete) the environment by freeing its allocated memory and other resources (if any)*/
void env_free(struct Env *e);

///===================================================================================
/*2024*/
void yield(void);					//giveup the CPU
void sched(void);					//enter the fos_scheduler while keeping the interrupt status of the process and restoring it after return
void switchuvm(struct Env *proc);	//switch to the user virtual memory (TSS & Process Directory)
void switchkvm(void);				//switch to the kernel virtual memory (Kern Directory)
void env_start(void);				//called only at the very first scheduling by scheduler()
void env_exit(void);				//add the running env to the EXIT queue, then reinvoke the scheduler

///===================================================================================

#endif // !FOS_KERN_ENV_H
