/* See COPYRIGHT for copyright information. */

#ifndef FOS_KERN_TRAP_H
#define FOS_KERN_TRAP_H
#ifndef FOS_KERNEL
# error "This is a FOS kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>
#include <kern/trap/fault_handler.h>

//2024: removed. Pplaced inside CPU struct
//static struct Taskstate ts;

/* The kernel's interrupt descriptor table */
extern struct Gatedesc idt[];

void idt_init(void);
void ts_init(void);

void print_regs(struct PushRegs *regs);
void print_trapframe(struct Trapframe *tf);
void fault_handler(struct Trapframe *);
void backtrace(struct Trapframe *);
extern void trapret();						//assembly code in trapEntry.S to pop the trapframe
void irq_install_handler(int irq, void (*handler)(struct Trapframe *tf));
void irq_uninstall_handler(int irq);

#endif /* FOS_KERN_TRAP_H */
