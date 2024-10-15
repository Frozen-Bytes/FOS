#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include "trap.h"
#include <kern/cons/console.h>
#include <kern/proc/user_environment.h>
#include "syscall.h"
#include <kern/trap/trap.h>
#include <kern/trap/fault_handler.h>
#include <kern/cmd/command_prompt.h>
#include <kern/cpu/kclock.h>
#include <kern/cpu/sched.h>
#include <kern/cpu/cpu.h>
#include <kern/cpu/picirq.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/mem/memory_manager.h>


extern  void (*PAGE_FAULT)();
extern  void (*SYSCALL_HANDLER)();
extern  void (*DBL_FAULT)();

extern  void (*ALL_FAULTS0)();
extern  void (*ALL_FAULTS1)();
extern  void (*ALL_FAULTS2)();
extern  void (*ALL_FAULTS3)();
extern  void (*ALL_FAULTS4)();
extern  void (*ALL_FAULTS5)();
extern  void (*ALL_FAULTS6)();
extern  void (*ALL_FAULTS7)();
//extern  void (*ALL_FAULTS8)();
//extern  void (*ALL_FAULTS9)();
extern  void (*ALL_FAULTS10)();
extern  void (*ALL_FAULTS11)();
extern  void (*ALL_FAULTS12)();
extern  void (*ALL_FAULTS13)();
//extern  void (*ALL_FAULTS14)();
//extern  void (*ALL_FAULTS15)();
extern  void (*ALL_FAULTS16)();
extern  void (*ALL_FAULTS17)();
extern  void (*ALL_FAULTS18)();
extern  void (*ALL_FAULTS19)();


extern  void (*IRQ0_CLK_HANDLER)();
extern  void (*IRQ1_KBD_HANDLER)();
extern  void (*ALL_FAULTS34)();
extern  void (*ALL_FAULTS35)();
extern  void (*ALL_FAULTS36)();
extern  void (*ALL_FAULTS37)();
extern  void (*ALL_FAULTS38)();
extern  void (*ALL_FAULTS39)();
extern  void (*ALL_FAULTS40)();
extern  void (*ALL_FAULTS41)();
extern  void (*ALL_FAULTS42)();
extern  void (*ALL_FAULTS43)();
extern  void (*ALL_FAULTS44)();
extern  void (*ALL_FAULTS45)();
extern  void (*ALL_FAULTS46)();
extern  void (*ALL_FAULTS47)();



static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
			"Divide error",
			"Debug",
			"Non-Maskable Interrupt",
			"Breakpoint",
			"Overflow",
			"BOUND Range Exceeded",
			"Invalid Opcode",
			"Device Not Available",
			"Double Fault",
			"Coprocessor Segment Overrun",
			"Invalid TSS",
			"Segment Not Present",
			"Stack Fault",
			"General Protection",
			"Page Fault",
			"(unknown trap)",
			"x87 FPU Floating-Point Error",
			"Alignment Check",
			"Machine-Check",
			"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	else if (trapno == IRQ0_Clock)
		return "Clock Interrupt";
	else if (trapno == IRQ1_KB)
		return "Keyboard Interrupt";
	return "(unknown trap)";
}


void ts_init(void)
{
	pushcli();	//disable interrupt - lock: to protect CPU info in multi-CPU

	struct cpu* c = mycpu();

	// Setup a TSS so that we get the right user kernel stack
	// when we trap to the kernel.
	// 2024: for now, temporarily set it to 0
	// since the scheduler will run first then switch to the first process
	c->ts.ts_esp0 = 0;
	c->ts.ts_ss0 = GD_KD;

	// Initialize the TSS field of the gdt.
	c->gdt[GD_TSS >> 3] = SEG16(STS_T32A, (uint32) (&(c->ts)), sizeof(struct Taskstate), 0);
	c->gdt[GD_TSS >> 3].sd_s = 0;

	popcli();	//enable interrupt - lock: to protect CPU info in multi-CPU

	// Load the TSS
	ltr(GD_TSS);
}


/// Interrupt descriptor table.  (Must be built at run time because
/// shifted function addresses can't be represented in relocation records.)
///
struct Gatedesc idt[256] = { { 0 } };

void idt_init(void)
{
	//initialize idt
	SETGATE(idt[T_DBLFLT  ], 0, GD_KT , &DBL_FAULT, 0) ;		//8
	SETGATE(idt[T_PGFLT   ], 0, GD_KT , &PAGE_FAULT, 0) ;		//14
	SETGATE(idt[IRQ0_Clock], 0, GD_KT , &IRQ0_CLK_HANDLER, 3) ;	//32
	SETGATE(idt[IRQ1_KB	  ], 0, GD_KT , &IRQ1_KBD_HANDLER, 3) ;	//33
	SETGATE(idt[T_SYSCALL ], 0, GD_KT , &SYSCALL_HANDLER, 3) ;	//48

	//S/W Exceptions
	SETGATE(idt[T_DIVIDE   ], 0, GD_KT , &ALL_FAULTS0, 3) ;
	SETGATE(idt[T_DEBUG    ], 1, GD_KT , &ALL_FAULTS1, 3) ;
	SETGATE(idt[T_NMI      ], 0, GD_KT , &ALL_FAULTS2, 3) ;
	SETGATE(idt[T_BRKPT    ], 1, GD_KT , &ALL_FAULTS3, 3) ;
	SETGATE(idt[T_OFLOW    ], 1, GD_KT , &ALL_FAULTS4, 3) ;
	SETGATE(idt[T_BOUND    ], 0, GD_KT , &ALL_FAULTS5, 3) ;
	SETGATE(idt[T_ILLOP    ], 0, GD_KT , &ALL_FAULTS6, 3) ;
	SETGATE(idt[T_DEVICE   ], 0, GD_KT , &ALL_FAULTS7, 3) ;
	//SETGATE(idt[T_DBLFLT ], 0, GD_KT , &ALL_FAULTS, 3) ;
	//SETGATE(idt[], 0, GD_KT , &ALL_FAULTS, 3) ;
	SETGATE(idt[T_TSS      ], 0, GD_KT , &ALL_FAULTS10, 3) ;
	SETGATE(idt[T_SEGNP    ], 0, GD_KT , &ALL_FAULTS11, 3) ;
	SETGATE(idt[T_STACK    ], 0, GD_KT , &ALL_FAULTS12, 3) ;
	SETGATE(idt[T_GPFLT    ], 0, GD_KT , &ALL_FAULTS13, 3) ;
	//SETGATE(idt[T_PGFLT    ], 0, GD_KT , &ALL_FAULTS, 3) ;
	//SETGATE(idt[ne T_RES   ], 0, GD_KT , &ALL_FAULTS, 3) ;
	SETGATE(idt[T_FPERR    ], 0, GD_KT , &ALL_FAULTS16, 3) ;
	SETGATE(idt[T_ALIGN    ], 0, GD_KT , &ALL_FAULTS17, 3) ;
	SETGATE(idt[T_MCHK     ], 0, GD_KT , &ALL_FAULTS18, 3) ;
	SETGATE(idt[T_SIMDERR  ], 0, GD_KT , &ALL_FAULTS19, 3) ;

	//IRQs
	SETGATE(idt[34], 0, GD_KT , &ALL_FAULTS34, 3) ;
	SETGATE(idt[35], 0, GD_KT , &ALL_FAULTS35, 3) ;
	SETGATE(idt[36], 0, GD_KT , &ALL_FAULTS36, 3) ;
	SETGATE(idt[37], 0, GD_KT , &ALL_FAULTS37, 3) ;
	SETGATE(idt[38], 0, GD_KT , &ALL_FAULTS38, 3) ;
	SETGATE(idt[39], 0, GD_KT , &ALL_FAULTS39, 3) ;
	SETGATE(idt[40], 0, GD_KT , &ALL_FAULTS40, 3) ;
	SETGATE(idt[41], 0, GD_KT , &ALL_FAULTS41, 3) ;
	SETGATE(idt[42], 0, GD_KT , &ALL_FAULTS42, 3) ;
	SETGATE(idt[43], 0, GD_KT , &ALL_FAULTS43, 3) ;
	SETGATE(idt[44], 0, GD_KT , &ALL_FAULTS44, 3) ;
	SETGATE(idt[45], 0, GD_KT , &ALL_FAULTS45, 3) ;
	SETGATE(idt[46], 0, GD_KT , &ALL_FAULTS46, 3) ;
	SETGATE(idt[47], 0, GD_KT , &ALL_FAULTS47, 3) ;

	// Load the IDT
	//asm volatile("lidt idt_pd");
	lidt(idt, sizeof(idt));

}

void print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s - %d\n", tf->tf_trapno, trapname(tf->tf_trapno), tf->tf_trapno);
	cprintf("  err  0x%08x\n", tf->tf_err);
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	cprintf("  esp  0x%08x\n", tf->tf_esp);
	cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

void print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}


void *irq_handlers[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} ;
void irq_install_handler(int irq, void (*handler)(struct Trapframe *tf))
{
	irq_handlers[irq] = handler;
}
void irq_uninstall_handler(int irq)
{
	irq_handlers[irq] = NULL;
}
void irq_dispatch(struct Trapframe *tf)
{
	void (*handler)(struct Trapframe *tf);
	int IRQNum = tf->tf_trapno - IRQ_OFFSET;
	handler = irq_handlers[IRQNum] ;
	if (handler)
	{
		handler(tf);
	}

	//Send End Of Interrupt CMD to PIC
	pic_sendEOI(IRQNum);
}

static void trap_dispatch(struct Trapframe *tf)
{
	if(tf->tf_trapno == T_PGFLT)
	{
		//2016: Bypass the faulted instruction [used for some tests in which we need to resume the execution after an intended page fault]
		if (bypassInstrLength != 0)
		{
			tf->tf_eip = (uint32*)((uint32)(tf->tf_eip) + bypassInstrLength);
			/*2024: commented. already will be returned to the trapret() in trapentry.S which return to the user/kernel caller code*/
			//kclock_resume();
			//env_pop_tf(tf);
			return;
		}

		//print_trapframe(tf);
		if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
		{
			//cprintf("===========Table WS before updating time stamp========\n");
			//env_table_ws_print(curenv) ;
			update_WS_time_stamps();
		}
		fault_handler(tf);
	}
	else if (tf->tf_trapno == T_SYSCALL)
	{
		/* If the original status of the interrupt is ENABLED (before getting into kernel),
		 * Then, re-enable the interrupts & resume the clock during the system calls
		 * to allow switching between processes
		 */
		if (tf->tf_eflags & FL_IF)
		{
			sti();
			kclock_resume();
		}
		//cprintf("\nserving system call #%d\n", tf->tf_regs.reg_eax);
		uint32 ret = syscall(tf->tf_regs.reg_eax
				,tf->tf_regs.reg_edx
				,tf->tf_regs.reg_ecx
				,tf->tf_regs.reg_ebx
				,tf->tf_regs.reg_edi
				,tf->tf_regs.reg_esi);

		tf->tf_regs.reg_eax = ret;

		/*If still enabled, Disable the interrupt & stop clock before getting into user again
		 */
		if (read_eflags() & FL_IF)
		{
			cli();
			kclock_stop();
		}
		//cprintf("ret val form syscall = %d\n", ret);
	}
	else if(tf->tf_trapno == T_DBLFLT)
	{
		panic("double fault!!");
	}
	else
	{
		// Unexpected trap: The user process or the kernel has a bug.
		print_trapframe(tf);
		if (tf->tf_cs == GD_KT)
		{
			panic("unhandled trap in kernel");
		}
		else
		{
			//env_destroy(curenv);
			panic("unhandled trap in user program");
		}
	}
}

void trap(struct Trapframe *tf)
{
	//[1] Stop the clock
	/* to avoid counting down on the current process while handling exceptions
	 * This avoid pending clock interrupt after returning from the trap.
	 * NOTE: interrupt is automatically disabled by the interrupt cycle (by marking all traps as "Interrupt Gates").
	 * Resume the clock and Re-enable the interrupt whenever required (e.g. in system calls).
	 */
	kclock_stop();

	//[2] Some validations

	//2024 check if interrupt is enabled during the trap handler, then panic
	uint32 flags = read_eflags();
	if (flags & FL_IF)
	{
		print_trapframe(tf);
		panic("trap(): interrupt is enabled while it's expected to be disabled\n");
	}

	int userTrap = 0;
	struct Env* cur_env = get_cpu_proc(); //the current running Environment (if any)

	if ((tf->tf_cs & 3) == 3)
	{
		assert(cur_env && cur_env->env_status == ENV_RUNNING);	//environment should be exist & run
		//cprintf("curenv->env_tf @ %x, tf param @ %x\n", curenv->env_tf , tf);
		assert(cur_env->env_tf == tf);	//tf should be placed in the kernel stack of this process (@e->env_tf)
		userTrap = 1;
	}
	//	cprintf("\n[%s - %d] trap #%d - %s  @va = %x - eip = %x - IEN = %d\n", userTrap == 1? "USER" : "KERNEL", userTrap == 1? curenv->env_id : 0, tf->tf_trapno, trapname(tf->tf_trapno), tf, tf->tf_eip, (read_eflags() & FL_IF) == 0? 0 : 1);
	//	if (tf->tf_trapno == T_SYSCALL)
	//	{
	//		cprintf("System Call #%d\n", tf->tf_regs.reg_eax);
	//	}
	//[3] Handle the incoming trap/interrupt
	if (tf->tf_trapno >= IRQ_OFFSET && tf->tf_trapno < IRQ_OFFSET + MAX_IRQS)
	{
		irq_dispatch(tf);
	}
	else
	{
		trap_dispatch(tf);
	}

	//cprintf("will be returned to the trapret() \n");
	/*2024: will be returned to the trapret() in trapentry.S which return to the caller*/

	//[4] Make sure that the interrupt is disabled before executing the trapret()
	uint32 IEN = read_eflags() & FL_IF;
	assert(IEN == 0);

	//cprintf("will resume the clock\n");

	//[5] Resume the clock
	kclock_resume();
	//	cprintf("\nclock is resumed with counter = %d.\n", kclock_read_cnt0_latch());
	//	cprintf("[tf] tf @%x - tf.cs = %x - tf.eip = %x - tf.eax = %d\n", tf, tf->tf_cs,tf->tf_eip, tf->tf_regs.reg_eax );
}

