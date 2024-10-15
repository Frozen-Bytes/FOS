/*
 * cpu.c
 *
 *  Created on: Sep 4, 2024
 *      Author: HP
 */
#include <kern/cpu/cpu.h>
#include <kern/cpu/sched.h>
#include <inc/x86.h>
#include <inc/stdio.h>
#include <inc/assert.h>
#include <inc/string.h>

extern void seg_init(void);
extern void idt_init(void);

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu* mycpu()
{
	return &CPUS[0]; //main CPU

	/*this code is for multi-cpu
	 * ref: xv6 OS (x86 ver)
	 */
//	if(read_eflags()&FL_IF)
//	    panic("mycpu called with interrupts enabled\n");

//	int apicid, i;
//  apicid = lapicid();
//  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
//  // a reverse map, or reserve a register to store &cpus[i].
//  for (i = 0; i < ncpu; ++i) {
//    if (cpus[i].apicid == apicid)
//      return &cpus[i];
//  }
//  panic("unknown apicid\n");
}

// Common CPU setup code.
void cpu_init(int cpuIndx)
{
  struct cpu* c = mycpu();
  c->proc = NULL;
  c->ncli = 0;
  c->intena = read_eflags() & FL_IF ? 1 : 0;


  //c->apicid = ?? ;

  //Initialize the CPU Context to NULL.
  //to be set later to the correct position on the stack during the
  //first switch from scheduler to the first process
  c->scheduler = NULL ;
  c->scheduler_status = SCH_UNINITIALIZED;

  //Initialize its sched stack
  c->stack = (char*)(KERN_STACK_TOP - (cpuIndx+1)*KERNEL_STACK_SIZE);

  //initialize GDT & set it to this CPU
  seg_init();

  //initialize IDT
  idt_init();       // load idt register

  //Initialize the TaskState to ZERO.
  //to be initialized later in init.c
  memset(&(c->ts), 0, sizeof(c->ts)) ;

  //Indicate it's started
  xchg(&(c->started), 1); // tell startothers() we're up

  //scheduler();     // start running processes
}


// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.

void pushcli(void)
{
  int eflags = read_eflags();
  cli();
  struct cpu* c = mycpu();
  if(c->ncli == 0)
    c->intena = eflags & FL_IF;
  c->ncli += 1;
}

void popcli(void)
{
  if(read_eflags()&FL_IF)
    panic("popcli - interruptible");
  struct cpu* c = mycpu();
  if(--c->ncli < 0)
    panic("popcli");
  if(c->ncli == 0 && c->intena)
    sti();
}

