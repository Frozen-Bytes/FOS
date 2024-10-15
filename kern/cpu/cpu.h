/*
 * cpu.h
 *
 *  Created on: Sep 4, 2024
 *      Author: HP
 */

#ifndef KERN_CPU_CPU_H_
#define KERN_CPU_CPU_H_
#include <inc/mmu.h>
#include <inc/memlayout.h>
// Per-CPU state
struct cpu {
  unsigned char apicid;			// Local APIC ID
  struct Context *scheduler;   	// context_switch() here to enter scheduler
  char* stack;					//Bottom of sched kernel stack for this CPU
								//Its first bottom page is ALWAYS used as a GUARD PAGE (i.e. unmapped)
  struct Taskstate ts;         	// Used by x86 to find stack for interrupt
  struct Segdesc gdt[NSEGS];   	// x86 global descriptor table
  volatile uint32 started;     	// Has the CPU started?
  int ncli;                    	// Depth of pushcli nesting (for locking).
  int intena;                  	// Were interrupts enabled before pushcli? (for locking)
  struct Env *proc;           	// The process running on this cpu or null
  int scheduler_status ;		// Status of the scheduler at this CPU
};

struct cpu CPUS[NCPUS] ;
struct cpu* mycpu();
void cpu_init(int cpuIndx);
void popcli(void);				//enable interrupt on current CPU
void pushcli(void);				//disable interrupt on current CPU
extern void context_switch(struct Context ** old, struct Context * new); //switch CPU regs & stack between two contexts


#endif /* KERN_CPU_CPU_H_ */
