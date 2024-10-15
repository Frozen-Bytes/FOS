/* See COPYRIGHT for copyright information. */

#ifndef FOS_KERN_PICIRQ_H
#define FOS_KERN_PICIRQ_H
#ifndef FOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#define MAX_IRQS	16	// Number of IRQs

// I/O Addresses of the two 8259A programmable interrupt controllers
#define IO_PIC1		0x20	// IO base address for master PIC (IRQs 0-7)
#define IO_PIC2		0xA0	// IO base address for slave PIC (IRQs 8-15)
#define PIC1_CMD	IO_PIC1
#define PIC1_DATA	(IO_PIC1+1)
#define PIC2_CMD	IO_PIC2
#define PIC2_DATA	(IO_PIC2+1)
#define PIC_EOI		0x20		/* End-of-interrupt command code */

#define IRQ_SLAVE	2	// IRQ at which slave connects to master
#define IRQ_OFFSET	32	// IRQ 0 corresponds to int IRQ_OFFSET
//IRQs
#define IRQ0_Clock  32		// Clock IRQ
#define IRQ1_KB 	33		// KB IRQ


#ifndef __ASSEMBLER__

#include <inc/types.h>
#include <inc/x86.h>

extern uint16 irq_init_mask_8259A;
void pic_init(void);
void irq_setmask_8259A(uint16 mask);

/*Ref: OSDev Wiki*/
void pic_sendEOI(uint8 irq);
void irq_set_mask(uint8 IRQline);
void irq_clear_mask(uint8 IRQline);
int irq_get_mask(uint8 IRQline);
/***********************************/

#endif // !__ASSEMBLER__

#endif // !JOS_KERN_PICIRQ_H
