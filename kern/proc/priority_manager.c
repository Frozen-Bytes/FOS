#include <inc/stdio.h>
#include <kern/proc/priority_manager.h>
#include <inc/assert.h>
#include <kern/proc/user_environment.h>
#include "../disk/pagefile_manager.h"
#include "../mem/kheap.h"
#include "../mem/memory_manager.h"

void set_program_priority(struct Env* env, int priority)
{
	//[PROGRAM PRIORITY] set_program_priority
	//[ALREADY IMPLEMENTED]

	if(priority < 1 || priority > 5)
	{
		panic("Please enter valid priority (1->5)\n");
		return;
	}
	if(env == NULL)
		return;
	switch(priority)
	{
		case PRIORITY_LOW:
			half_WS_Size(env, 1);
			break;
		case PRIORITY_BELOWNORMAL:
			half_WS_Size(env, 0);
			break;
		case PRIORITY_NORMAL:
			// Do Nothing
			break;
		case PRIORITY_ABOVENORMAL:
			double_WS_Size(env, 1);
			break;
		case PRIORITY_HIGH:
			double_WS_Size(env, 0);
			break;
	}

	cprintf("The new size of the ws is %d\n", env->page_WS_max_size);
}
